// The frame rate. PGR1 advances its game one fixed tick a frame: the main loop (0xC9710) calls 0xC5480, which runs every
// task's update once with the step 0x15AA84 x 1/60 (0x143784; 0x15AA84 is 1.1 at 60 Hz, set by 0xC9200), and nothing
// catches up. So a frame later than 1/60 s put the whole game in slow motion (50 fps is 83% speed), and a frame rate
// above 60 would have sped it up.
//
// Game time follows the clock: the main loop's tick (called at 0xC97AC, in a replay at 0xC9769) runs here as many times
// as 1/60 s steps have passed since the last, at least once and at most four times (the replay's fast forward runs four);
// a gap over 0.15 s (a load) is not caught up. PGR1_FIXED_TIME=1 goes back to one a frame; PGR1_TICK_LOG=1 logs the count.
//
// Above 60: the game still draws one frame a tick, 60 a second, and in a race the runtime draws frames in between up to
// the frame rate cap (GameInfo::step_interpolation, gpu/interp.cpp).
#include <algorithm>
#include <chrono>
#include <cstdlib>

#include "app/app.h"
#include "common/release.h"
#include "core/log.h"
#include "kernel/kernel.h"

namespace pgr1 {
bool Racing();  // src/game/hooks.cpp

bool StepInterpolation() {
    uint32_t cap = app::Get().fps_cap;
    return (cap == 0 || cap > 60) && Racing();
}
}  // namespace pgr1

namespace {
using Clock = std::chrono::steady_clock;
constexpr double kStep = 1.0 / 60.0, kLongestGap = 0.15;
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

X86_EXTERN_FUNC(PGR1_ORIG(000C5480, 000C54A0, 000C2A80));
X86_FUNC(PGR1_FN(000C5480, 000C54A0, 000C2A80)) {
    uint32_t ret = kernel::R32(ctx.esp);
    if (!FollowClock() || (ret != PGR1_VA(0xC97B1, 0xC97D1, 0xC6A81) && ret != PGR1_VA(0xC976E, 0xC978E, 0xC6A3E))) {
        PGR1_ORIG(000C5480, 000C54A0, 000C2A80)(ctx, base);
        return;
    }
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
        if (now - since >= std::chrono::seconds(10)) {
            LOGR("pgr1: %d ticks in %d frames, %.1f s", run, frames, std::chrono::duration<double>(now - since).count());
            since = now, frames = 0, run = 0;
        }
    }
    for (int i = 0; i < ticks; i++) {
        if (i) {
            ctx.esp -= 4;  // the call again, to the same return
            kernel::W32(ctx.esp, ret);
        }
        PGR1_ORIG(000C5480, 000C54A0, 000C2A80)(ctx, base);
    }
}
