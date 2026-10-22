// Hooks for default.xbe. A strong definition replaces the recompiled function at that address.
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

#include "app/app.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "xdk/hle.h"

namespace pgr1 {
extern bool skip_intro;
extern uint32_t fov_percent;
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
    __imp__sub_000A57F0(ctx, base);
    float aspect = app::FrameAspect(), k = 4.0f / 3.0f / aspect;
    if (out && k < 0.999f) {
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
