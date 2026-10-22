#include <cstdlib>

#include "app/app.h"

namespace pgr1 {
bool skip_intro = false;  // no intro movie, logos or warning at start (src/game/hooks.cpp)
uint32_t fov_percent = 100;  // the race camera's field of view, percent of the console's (src/game/hooks.cpp)
}

namespace {
// PGR1_SKIP_INTRO=1 and PGR1_FOV=percent set those options without a settings file (the headless harness has none)
void Init() {
    if (const char* e = std::getenv("PGR1_SKIP_INTRO"); e && *e == '1') pgr1::skip_intro = true;
    if (const char* e = std::getenv("PGR1_FOV")) pgr1::fov_percent = uint32_t(std::atoi(e));
}
}  // namespace

// what the runtime asks of the game; strings must be set, hooks may stay null
const app::GameInfo app::kGame = {
    .title = L"Project Gotham Racing",               // window and splash
    .short_title = L"PGR1",               // message boxes
    .name = L"PGR1",                      // window classes, crash dumps, handover event
    .log = "pgr1",                        // log file prefix
    .settings = "pgr1.toml",             // settings file beside the exe
    .splash = L"splash",                  // optional splash.png or splash.bmp beside the exe
    .missing = L"Put the game in the assets folder: its disc image (.iso), or default.xbe with its folders beside it.",
    .images = {{"default.xbe", "PGR1"}},  // each recompiled xbe and its exe, for title launches
    // game settings kept in the settings file
    .options = {{"skip_intro", &pgr1::skip_intro, nullptr, "Skip intro", "The movie, logos and warning; at the next start."},
                {"fov_percent", nullptr, &pgr1::fov_percent, "Race camera field of view (%)",
                 "100 is the console's; 50 to 200, at once."}},
    .load = nullptr,                      // reads more from the settings file's toml::table
    .save = nullptr,                      // writes more into it
    .init = Init,                         // after the settings load, before the window and boot
    .loaded = nullptr,                    // image in memory, before its entry point runs
    .window = nullptr,                    // the game window is up
    .message = nullptr,                   // window messages first; true takes one
    .ui = nullptr,                        // ImGui drawing each frame
    .pad = nullptr,                       // changes a polled pad
    .rumble = nullptr,                    // changes a rumble before it is set
    // optional, after these: .achievements (conditions on guest memory), .same_code (other releases' xbe SHA-1s with
    // the same code), .wheel (what a steering wheel's force feedback follows)
    .bink_frame_hold = false,  // the movie player (0xE9780) waits on BinkWait itself (0xE98D0) and counts its frames
    .hud_aspect = 4.0f / 3.0f,  // no widescreen mode: menus and HUD are laid out 4:3, the 3D widens (src/game/hooks.cpp)
};
