// The frame rate. PGR1 advances its game one fixed tick a frame: the main loop (0xC9710) calls 0xC5480, which runs every
// task's update once with the step 0x15AA84 x 1/60 (0x143784; 0x15AA84 is 1.1 at 60 Hz, set by 0xC9200), and nothing
// catches up. So a frame later than 1/60 s put the whole game in slow motion (50 fps is 83% speed).
//
// Game time follows the clock: the main loop's tick (called at 0xC97AC, in a replay at 0xC9769) runs here as many times
// as ticks are due since the last, at least once and at most four times (the replay's fast forward runs four); a gap over
// 0.15 s (a load) is not caught up. PGR1_FIXED_TIME=1 goes back to one a frame; PGR1_TICK_LOG=1 logs the count each second.
//
// frame_rate (settings file; PGR1_HZ overrides it): above 60, the game told the display runs that fast, as its 30 a
// second split screen mode is told 30. The rate 0x2BA9F0 (a float, read by 79 places, which 0xC5400 copies from the
// display's 0x2BA9F4) is set to it, the step scaled to match, and frames paced at it: every frame drawn by the game, a
// tick each. Car speed and the race clock keep real time (headless, against 60). The display's rate stays 60: a save is
// tagged with it, and only offered and loaded on a display of the same rate (FINDINGS, "Frame rate").
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>

#include "app/app.h"
#include "common/release.h"
#include "core/log.h"
#include "kernel/kernel.h"

namespace pgr1 {
extern uint32_t frame_rate;
}

namespace {
using Clock = std::chrono::steady_clock;
constexpr double kLongestGap = 0.15;

// the rate above 60 the game runs at, read once (the setting applies at the next start), 0 at 60
uint32_t Hz() {
    static const uint32_t hz = [] {
        const char* e = std::getenv("PGR1_HZ");
        uint32_t v = e ? uint32_t(std::atoi(e)) : pgr1::frame_rate;
        v = v > 60 && v <= 240 ? v : 0;
        if (v) {
            LOGR("pgr1: the game runs at %u frames a second", v);
            app::g_pace_hz = v;
            // a frame rate cap below it would leave the game drawing fewer frames than it runs ticks
            app::Settings& s = app::Get();
            if (s.fps_cap && s.fps_cap < v) {
                LOGR("pgr1: the frame rate cap raised from %u to %u", s.fps_cap, v);
                s.fps_cap = v;
            }
        }
        return v;
    }();
    return hz;
}

// the step's factor: USA keeps it in its data, the European discs set it at boot
constexpr uint32_t kStepAt = PGR1_VA(0x15AA84, 0x15A9A4, 0x158284, 0x15A9E4);

// a save tagged with a rate above 60 (made by a build that told the display it ran faster) listed as this display's
// own, as it is: the drivers found (0x190C00 of them, 0x40 bytes each from 0x190C10) carry it as a byte 6 before
void LoadableSaves() {
    const uint32_t count = kernel::R32(pgr1::Va(0x190C00));
    float display;
    std::memcpy(&display, &kernel::G<uint32_t>(pgr1::Va(0x2BA9F4)), 4);
    for (uint32_t i = 0; i < count && i < 64; i++) {
        uint8_t& tag = kernel::G<uint8_t>(pgr1::Va(0x190C10) + i * 0x40 - 6);
        if (tag > 60 && display == 60.0f) tag = 60;
    }
}

void ApplyHz() {
    const uint32_t hz = Hz();
    if (!hz) return;
    static float base = 0;
    float step;
    std::memcpy(&step, &kernel::G<uint32_t>(kStepAt), 4);
    if (!base && step > 1.0f && step < 1.5f) base = step;  // 1.1 at 60 Hz, 1.32 at 50, set at boot by 0xC9200
    if (!base) return;
    const float want = base * 60.0f / float(hz);
    if (step != want) std::memcpy(&kernel::G<uint32_t>(kStepAt), &want, 4);
    // floats: 60.0 at 60 Hz
    const float rate = float(hz);
    float now;
    std::memcpy(&now, &kernel::G<uint32_t>(pgr1::Va(0x2BA9F0)), 4);
    if (now == 60.0f) std::memcpy(&kernel::G<uint32_t>(pgr1::Va(0x2BA9F0)), &rate, 4);
}
constexpr int kMostTicks = 4;

bool FollowClock() {
    static const bool on = [] {
        const char* e = std::getenv("PGR1_FIXED_TIME");
        bool v = !(e && *e == '1');
        LOGR("pgr1: game time %s", v ? "follows the clock" : "one tick a frame, as on the console");
        return v;
    }();
    return on;
}
}  // namespace

X86_EXTERN_FUNC(PGR1_ORIG(000C5480, 000C54A0, 000C2A80, 000C5490));
X86_FUNC(PGR1_FN(000C5480, 000C54A0, 000C2A80, 000C5490)) {
    uint32_t ret = kernel::R32(ctx.esp);
    if (!FollowClock() || (ret != PGR1_VA(0xC97B1, 0xC97D1, 0xC6A81, 0xC9801) && ret != PGR1_VA(0xC976E, 0xC978E, 0xC6A3E, 0xC97BE))) {
        PGR1_ORIG(000C5480, 000C54A0, 000C2A80, 000C5490)(ctx, base);
        return;
    }
    ApplyHz();
    LoadableSaves();
    const double kStep = 1.0 / (Hz() ? Hz() : 60);
    static Clock::time_point last{};
    static double owed = 0;  // seconds of game time not yet run
    Clock::time_point now = Clock::now();
    double gap = std::chrono::duration<double>(now - last).count();
    last = now;
    owed = gap > kLongestGap || gap < 0 ? kStep : owed + gap;
    int ticks = std::clamp(int(owed / kStep), 1, kMostTicks);
    owed = std::clamp(owed - ticks * kStep, -kStep, kStep);
    // PGR1_TICK_LOG=1: the ticks and frames of every 10 s in the log
    static const bool log = [] {
        const char* e = std::getenv("PGR1_TICK_LOG");
        return e && *e == '1';
    }();
    if (log) {
        static Clock::time_point since = now;
        static int frames = 0, run = 0;
        frames++;
        run += ticks;
        if (now - since >= std::chrono::seconds(1)) {
            float speed;
            std::memcpy(&speed, &kernel::G<uint32_t>(pgr1::Va(0x2EC9D8)), 4);
            LOGR("pgr1: %d ticks in %d frames, %.1f s, speed %.1f m/s", run, frames, std::chrono::duration<double>(now - since).count(),
                 speed);
            since = now, frames = 0, run = 0;
        }
    }
    for (int i = 0; i < ticks; i++) {
        if (i) {
            ctx.esp -= 4;  // the call again, to the same return
            kernel::W32(ctx.esp, ret);
        }
        PGR1_ORIG(000C5480, 000C54A0, 000C2A80, 000C5490)(ctx, base);
    }
}

// a driver loaded from a save (0xD0210 reads the profile block whole) whose rate tag, the byte 0x2C68E6, is above 60: the
// tag the display's own, or 0xD0490 refuses the driver ("Unable to load driver save game")
X86_EXTERN_FUNC(PGR1_ORIG(000D0210, 000D0230, 000CD490, 000D0270));
X86_FUNC(PGR1_FN(000D0210, 000D0230, 000CD490, 000D0270)) {
    PGR1_ORIG(000D0210, 000D0230, 000CD490, 000D0270)(ctx, base);
    uint8_t& tag = kernel::G<uint8_t>(pgr1::Va(0x2C68E6));
    if (tag > 60) {
        LOGR("pgr1: a driver saved with the display at %u loaded as 60", tag);
        tag = 60;
    }
}
