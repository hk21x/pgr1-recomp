// Hooks for default.xbe. A strong definition replaces the recompiled function at that address.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>

#include "app/app.h"
#include "core/input.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "gpu/texture.h"
#include "xdk/d3d8.h"
#include "xdk/hle.h"

namespace pgr1 {
extern bool skip_intro;
extern uint32_t fov_percent;
extern bool hide_hud;
}

// skip_intro: startup (0xC9200) plays FMV\Intro.bik (IntroJ/G/F/S/I by language, 0xC9383) through the movie player
// 0xE9780 (cdecl: the path; its result unused), which loads the Bink sections around the movie (0xDD920 with 1, then 0)
// The logos and warning after it are front end screens: Frontend.ini's SkipIntroScreens (0x2E92A8, read before the
// movie; the intro handler 0x56680 tests it at 0x566B1 and skips to 0x55A20) is set here, after the INI is read
X86_EXTERN_FUNC(__imp__sub_000E9780);
X86_FUNC(sub_000E9780) {
    if (pgr1::skip_intro) {
        std::string path;
        for (uint32_t p = kernel::R32(ctx.esp + 4); p && path.size() < 260; p++) {
            char c = char(kernel::G<uint8_t>(p));
            if (!c) break;
            path += char(std::tolower(static_cast<unsigned char>(c)));
        }
        if (path.find("fmv\\intro") != std::string::npos) {
            kernel::G<uint32_t>(0x2E92A8) = 1;  // SkipIntroScreens
            LOGR("pgr1: intro movie %s and the intro screens skipped", path.c_str());
            ctx.esp += 4;  // ret
            return;
        }
    }
    __imp__sub_000E9780(ctx, base);
}

// Widescreen: PGR1 renders 4:3 only (it never asks XGetVideoFlags). Every 3D projection is built by 0xA57F0 (cdecl:
// out, the view's height/width, near, far, horizontal fov, vertical fov; the only fptan in the game), shown at the
// frame's shape it would stretch, so its x scale (out[0]) narrows by 4:3 over that shape: the same height, a wider
// view. The 2D stays 4:3 in the middle (GameInfo::hud_aspect).
X86_EXTERN_FUNC(__imp__sub_000A57F0);
X86_FUNC(sub_000A57F0) {
    uint32_t out = kernel::R32(ctx.esp + 4);
    // the car select's car is drawn into a square target (1024x1024) and shown through a 2D quad: not widened, or it
    // comes out squashed by the same factor
    uint32_t target_w = 0, target_h = 0;
    gpu::SurfaceSize(d3d::State().rt, target_w, target_h);
    __imp__sub_000A57F0(ctx, base);
    float aspect = app::FrameAspect(), k = 4.0f / 3.0f / aspect;
    if (out && k < 0.999f && target_w > target_h) {
        float m00;
        std::memcpy(&m00, &kernel::G<uint32_t>(out), 4);
        m00 *= k;
        std::memcpy(&kernel::G<uint32_t>(out), &m00, 4);
        static float logged = 0;
        if (logged != aspect) LOGR("pgr1: 3D views widened to %.3f:1", logged = aspect);
    }
}

// fov_percent: the race camera's field of view. Cameras set their projection through 0xA58E0 (cdecl: zoom, ...; the
// horizontal and vertical fov are 1.14 rad over the zoom); the race camera's call is at 0x7863E (zoom 1, 2 in the
// fly-by before the start), the car's shadow camera 0x284BB (12), the menus 0xA6F39 and 0xAA65C (1.8)
X86_EXTERN_FUNC(__imp__sub_000A58E0);
X86_FUNC(sub_000A58E0) {
    uint32_t pct = std::clamp<uint32_t>(pgr1::fov_percent, 50, 200);
    if (pct != 100 && kernel::R32(ctx.esp) == 0x78643) {
        float zoom;
        std::memcpy(&zoom, &kernel::G<uint32_t>(ctx.esp + 4), 4);
        zoom = zoom * 100.0f / float(pct);
        std::memcpy(&kernel::G<uint32_t>(ctx.esp + 4), &zoom, 4);
        static uint32_t logged = 100;
        if (logged != pct) LOGR("pgr1: race camera field of view %u%%", logged = pct);
    }
    __imp__sub_000A58E0(ctx, base);
}

// hide_hud: the race HUD (time, targets, Kudos, rev counter, lap) is drawn by the ARCADE_RACER task (the task table's
// entry 0x15BDB0: setup 0x807C0, drawing 0x820F0, 0x80940); its drawing is skipped, the race goes on
namespace {
std::atomic<int64_t> g_hud_drawn{0};  // steady clock ms: the race HUD's drawing ran (racing, not the attract demo)
int64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
}  // namespace

// the race HUD spread out on a wide screen (sub_0007D270 below)
namespace {
bool g_spread = false;               // this HUD is spread
float g_shift_left = 0, g_shift_right = 0;  // guest pixels
int g_side_owner = 0;  // an element with its own side is drawing: what it calls keeps that side
void HudSide(int side) {
    if (g_spread && !g_side_owner)
        d3d::State().squeeze_shift = side < 0 ? g_shift_left : side > 0 ? g_shift_right : 0.0f;
}
// the 16:9 frame's width over the 4:3 one (at most) on a wide screen with one player, else 0
float SpreadRatio() {
    float r = std::min(app::FrameAspect(), 16.0f / 9.0f) / (4.0f / 3.0f);
    return r > 1.001f && kernel::G<int32_t>(0x2C695E) <= 1 ? r : 0.0f;
}
}  // namespace

X86_EXTERN_FUNC(__imp__sub_000820F0);
X86_FUNC(sub_000820F0) {
    g_hud_drawn = NowMs();
    if (pgr1::hide_hud) {
        ctx.esp += 4;  // ret
        return;
    }
    __imp__sub_000820F0(ctx, base);
    d3d::State().squeeze_beyond = false;  // set by the layout below (sub_0007D270) while the HUD is spread out
    d3d::State().squeeze_shift = 0;
}

namespace pgr1 {
// GameInfo::wheel: a steering wheel's forces follow the player's car while its race HUD is drawn (the ARCADE_RACER task
// runs in the player's races, not the attract demo); its speed is the float at 0x2EC9D8, m/s (the top speed challenge
// 0x44C60 times it by 3.6; 26.8 against the speedometer's 57 mph, 32.3 against 71, headless). The gear is not known.
void WheelFeel(input::WheelFeel& feel) {
    feel.racing = NowMs() - g_hud_drawn.load() < 200;
    float speed;
    std::memcpy(&speed, &kernel::G<uint32_t>(0x2EC9D8), 4);
    feel.speed = feel.racing && std::isfinite(speed) ? speed : 0;
}
}  // namespace pgr1

// PGR1_DEBUG_LOG=1: the game's own debug messages in the log. 0x378A0 formats one (vsprintf 0xDDE80) and hands the text
// to 0x37490 (cdecl: the text), which keeps it off screen. The radio's (WMAPlayer, CDataStreamer) need its debug level
// 0x2E94AC at 5 as well.
X86_EXTERN_FUNC(__imp__sub_00037490);
X86_FUNC(sub_00037490) {
    static const bool on = [] {
        const char* e = std::getenv("PGR1_DEBUG_LOG");
        return e && *e == '1';
    }();
    if (on) {
        std::string text;
        for (uint32_t p = kernel::R32(ctx.esp + 4); p && text.size() < 400; p++) {
            char c = char(kernel::G<uint8_t>(p));
            if (!c) break;
            if (c != '\n' && c != '\r') text += c;
        }
        LOGI("pgr1 debug: %s", text.c_str());
    }
    __imp__sub_00037490(ctx, base);
}

// Ultrawide HUD: the race HUD keeps its 4:3 shape (GameInfo::hud_aspect squeezes 2D into the centre) but is laid out at
// 16:9, as PGR2's. 0x7D270 sets the HUD's anchors from the safe area percentage (0x159B40) before the elements draw: x,y
// pairs from 0x192484, 8 bytes apart (top left, bottom left, top right, bottom right, top and bottom centre, centre).
// The game's own coordinates cannot move (its medals and big digits are not drawn left of 0, the Kudos panel not right
// of 640), so the renderer moves each element as it squeezes it (State::squeeze_shift): a left one by its anchor's
// distance from the centre times the 16:9 frame's width over the 4:3 one less 1, a right one likewise, centred ones not.
// The side: the anchor slot text is placed from (0x7D4E0), the anchor an element reads itself, else none. One player
// only: split screen lays out per view.

X86_EXTERN_FUNC(__imp__sub_0007D270);
X86_FUNC(sub_0007D270) {
    __imp__sub_0007D270(ctx, base);
    float r = SpreadRatio();
    g_spread = r != 0;
    if (!g_spread) return;
    g_shift_left = (float(kernel::G<int32_t>(0x192484)) - 320.0f) * (r - 1);
    g_shift_right = (float(kernel::G<int32_t>(0x192494)) - 320.0f) * (r - 1);
    gpu::State& state = d3d::State();
    state.squeeze_beyond = true;  // until the HUD is drawn (sub_000820F0)
    state.squeeze_shift = 0;
    static float logged = 0;
    if (logged != r)
        LOGR("pgr1: race HUD laid out at %.3f:1 (columns moved %.0f and %.0f)", 4.0f / 3.0f * (logged = r), g_shift_left,
             g_shift_right);
}

// text: the anchor slot in ecx (0 and 1 left, 2 and 3 right, 4 to 6 centred)
X86_EXTERN_FUNC(__imp__sub_0007D4E0);
X86_FUNC(sub_0007D4E0) {
    uint32_t slot = ctx.ecx;
    HudSide(slot < 2 ? -1 : slot < 4 ? 1 : 0);
    __imp__sub_0007D4E0(ctx, base);
}

// every element the HUD task (0x820F0) calls: its side, from the anchor it reads (0x7F040 and 0x7F510 top left, 0x7E8F0
// bottom left, 0x7EEF0 and 0x813F0 bottom right: the speedometer) or what it is (the Kudos panel, top right, places
// itself), else none until its text says
#define PGR1_HUD_ELEMENT(addr, side)            \
    X86_EXTERN_FUNC(__imp__sub_##addr);         \
    X86_FUNC(sub_##addr) {                      \
        HudSide(side);                          \
        g_side_owner += (side) != 0;            \
        __imp__sub_##addr(ctx, base);           \
        g_side_owner -= (side) != 0;            \
    }
PGR1_HUD_ELEMENT(0007F040, -1)
PGR1_HUD_ELEMENT(0007F510, -1)
PGR1_HUD_ELEMENT(0007E8F0, -1)
PGR1_HUD_ELEMENT(0007EEF0, 1)
PGR1_HUD_ELEMENT(000813F0, 1)
PGR1_HUD_ELEMENT(0007FEC0, 1)  // the Kudos panel and its pop-ups, top right (drawn through 0x7F8E0)
PGR1_HUD_ELEMENT(0007D590, 0)
PGR1_HUD_ELEMENT(0007DCB0, 0)
PGR1_HUD_ELEMENT(0007DE50, 0)
PGR1_HUD_ELEMENT(0007DF40, 0)
PGR1_HUD_ELEMENT(0007DFC0, 0)
PGR1_HUD_ELEMENT(0007E010, 0)
PGR1_HUD_ELEMENT(0007E0B0, 0)
PGR1_HUD_ELEMENT(0007E110, 0)
PGR1_HUD_ELEMENT(0007E160, 0)
PGR1_HUD_ELEMENT(0007E230, 0)
PGR1_HUD_ELEMENT(0007E310, 0)
PGR1_HUD_ELEMENT(0007E3D0, 0)
PGR1_HUD_ELEMENT(0007E4B0, 0)
PGR1_HUD_ELEMENT(0007E980, 0)
PGR1_HUD_ELEMENT(0007EA30, 0)
PGR1_HUD_ELEMENT(0007EAE0, 0)
PGR1_HUD_ELEMENT(0007EF70, 0)
PGR1_HUD_ELEMENT(0007ECE0, 0)
PGR1_HUD_ELEMENT(0007F000, 0)
PGR1_HUD_ELEMENT(0007F130, 0)
PGR1_HUD_ELEMENT(0007F180, 0)
PGR1_HUD_ELEMENT(0007F230, 0)
PGR1_HUD_ELEMENT(0007F290, 0)
PGR1_HUD_ELEMENT(0007F350, 0)
PGR1_HUD_ELEMENT(0007F3B0, 0)
PGR1_HUD_ELEMENT(0007F410, 0)
PGR1_HUD_ELEMENT(0007F470, 0)
PGR1_HUD_ELEMENT(0007F610, 0)
PGR1_HUD_ELEMENT(000808C0, 0)
PGR1_HUD_ELEMENT(00080AB0, 0)
PGR1_HUD_ELEMENT(00081170, 0)
PGR1_HUD_ELEMENT(000812C0, 0)
PGR1_HUD_ELEMENT(00081FB0, 0)
