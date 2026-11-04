// RECOMP in PGR1's own menus: an entry at the end of Options, and one in a race's pause menu above quit race, each
// opening the PC's settings in sections. What rows cannot do (binding by moving a control, achievements, live
// readouts) stays in the settings window (F1), which a row opens. FINDINGS.md, "Menus", has how both menus work.
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "app/app.h"
#include "common/release.h"
#include "core/input.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xdk/hle.h"

namespace pgr1 {

extern bool skip_intro, hide_hud;
extern uint32_t fov_percent, radio_percent, frame_rate;

namespace {

// the front end's widgets
constexpr uint32_t kItem = PGR1_VA(0x62880, 0x628A0, 0x61B50);       // cdecl: place in the list, label, icon, A's handler
constexpr uint32_t kRow = PGR1_VA(0x64C70, 0x64C90, 0x63E10);        // cdecl: place, label, icon, table, count, the int
constexpr uint32_t kLink = PGR1_VA(0x64CC0, 0x64CE0, 0x63E60);       // cdecl: items, count, 0
constexpr uint32_t kOn = PGR1_VA(0xC0CC0, 0xC0CE0, 0xBE2F0);         // cdecl: item, event (5 A, 7 B), handler
constexpr uint32_t kAlso = PGR1_VA(0xC0B90, 0xC0BB0, 0xBE1C0);       // the same, a second handler
constexpr uint32_t kFocus = PGR1_VA(0xC0E60, 0xC0E80, 0xBE490);      // cdecl: item
constexpr uint32_t kNewPage = PGR1_VA(0xC0760, 0xC0780, 0xBDD80);    // cdecl: the byte that remembers the focus
constexpr uint32_t kTitle = PGR1_VA(0x62A30, 0x62A50, 0x61D00);      // cdecl: first item, title, icon (-1 none)
constexpr uint32_t kBackPage = PGR1_VA(0x61950, 0x61920, 0x60C60);
constexpr uint32_t kLeavePage = PGR1_VA(0x618F0, 0x618C0, 0x60C00);
constexpr uint32_t kOptionsBack = PGR1_VA(0x5B620, 0x5B640, 0x5AB30);
constexpr uint32_t kShowChoice = PGR1_VA(0x5B720, 0x5B740, 0x5AC20);  // a row's value, in its choice's entry
constexpr uint32_t kAButton = PGR1_VA(0x5B180, 0x5B1A0, 0x5A690), kBButton = PGR1_VA(0x5B080, 0x5B0A0, 0x5A590);
// the pause menu's
constexpr uint32_t kMenuDraw = PGR1_VA(0x85FA0, 0x85FC0, 0x84C70), kMenuInput = PGR1_VA(0x86080, 0x860A0, 0x84D50);
constexpr uint32_t kMenuItem = PGR1_VA(0x83BE0, 0x83C00, 0x828C0);
constexpr uint32_t kCanSound = PGR1_VA(0x84B80, 0x84BA0, 0x83860), kStepSound = PGR1_VA(0xB7D50, 0xB7D70, 0xB5370);

constexpr uint32_t kTag = 0x52430000;  // a RECOMP item's +0x58, which the game's widget maker zeroes; low: what it opens
constexpr uint32_t kOpenPage = 0xFF, kOpenWindow = 0xFE;
constexpr int kFirstRowId = 1370, kFirstTextId = 1386, kLastId = 1499;  // ids no language file has and no code reads
constexpr int kSectionCount = 6;

uint32_t Strings() { return Va(0x2E7400); }  // a char pointer per string id

uint32_t Guest(const std::string& text) {
    static std::map<std::string, uint32_t> made;
    auto it = made.find(text);
    if (it != made.end()) return it->second;
    uint32_t va = mem::AllocSystem(uint32_t(text.size() + 1));
    std::memcpy(kernel::HostPtr(va), text.c_str(), text.size() + 1);
    return made[text] = va;
}

// a string id for a text. Written each time: the game fills the table as it loads a language
uint32_t Id(const std::string& text) {
    static std::map<std::string, int> ids;
    static int next = kFirstTextId;
    auto it = ids.find(text);
    int id;
    if (it != ids.end()) id = it->second;
    else if (next >= kLastId - 1 - kSectionCount - 1) return 0;
    else id = ids[text] = next++;
    kernel::G<uint32_t>(Strings() + id * 4) = Guest(text);
    return uint32_t(id);
}

// an item's icon: the sprite numbers Options gives its items, in turn (the game fills these as it starts)
uint32_t Icon(uint32_t i) {
    constexpr uint32_t kIcons[] = {PGR1_VA(0x2E8D80, 0x2E8CA0, 0x2E5200), PGR1_VA(0x2E8DC8, 0x2E8CE8, 0x2E5248), PGR1_VA(0x2E8E00, 0x2E8D20, 0x2E527E),
                                   PGR1_VA(0x2E8D9A, 0x2E8CBA, 0x2E521A), PGR1_VA(0x2E8EC4, 0x2E8DE4, 0x2E5340)};
    return uint32_t(int32_t(kernel::G<int16_t>(kIcons[i % 5])));
}

uint32_t Nearest(const std::vector<uint32_t>& values, uint32_t v) {
    size_t best = 0;
    for (size_t i = 1; i < values.size(); i++)
        if (std::abs(int(values[i]) - int(v)) < std::abs(int(values[best]) - int(v))) best = i;
    return uint32_t(best);
}

using Choices = std::vector<std::string>;
struct Row {
    const char* label;
    Choices (*choices)();
    uint32_t (*get)();
    void (*set)(uint32_t);
};

Choices OnOff() { return {"off", "on"}; }

const std::vector<uint32_t> kRender = {25, 50, 75, 100, 125, 150, 175, 200}, kAniso = {0, 2, 4, 8, 16},
                            kFov = {75, 90, 100, 110, 125, 150}, kRadio = {0, 50, 100, 150, 200, 300, 400}, kRates = {60, 90, 120, 144},
                            kSharpen = {0, 25, 50, 75, 100}, kCaps = {30, 60, 120, 144, 240, 0},
                            kSens = {25, 50, 75, 100, 150, 200, 400, 800},
                            kWheelRange = {180, 270, 360, 540, 720, 900, 1080, 2880},
                            kWheelLock = {180, 270, 360, 450, 540, 720, 900, 1080},
                            kDead = {0, 1, 2, 3, 5, 10, 20}, kPercent = {0, 25, 50, 70, 85, 100};

std::vector<app::Mode> ResolutionList() {
    std::vector<app::Mode> list = app::ListModes();
    if (list.size() > 7) list.resize(7);
    return list;
}

uint32_t NearestCap(uint32_t cap) {
    for (size_t i = 0; i < kCaps.size(); i++)
        if (kCaps[i] == cap) return uint32_t(i);
    return 1;
}

void WheelChanged() { input::WheelSetup(); }

const std::vector<Row> kPicture = {
    {"msaa",
     [] { return Choices{"off", "2x", "4x", "8x"}; },
     [] { return std::min<uint32_t>(app::Get().msaa, 3); },
     [](uint32_t i) { app::Get().msaa = i; }},
    {"smooth alpha", OnOff,
     [] { return uint32_t(app::Get().alpha_coverage); },
     [](uint32_t i) { app::Get().alpha_coverage = i != 0; }},
    {"edge smoothing",
     [] { return Choices{"off", "fxaa", "smaa"}; },
     [] {
         const app::Settings& s = app::Get();
         return s.smaa ? uint32_t(2) : uint32_t(s.fxaa);
     },
     [](uint32_t i) {
         app::Get().fxaa = i == 1;
         app::Get().smaa = i == 2;
     }},
    {"sharpening",
     [] { return Choices{"off", "25%", "50%", "75%", "100%"}; },
     [] { return Nearest(kSharpen, app::Get().sharpen); },
     [](uint32_t i) { app::Get().sharpen = kSharpen[i]; }},
    {"texture filtering",
     [] { return Choices{"game", "2x", "4x", "8x", "16x"}; },
     [] { return Nearest(kAniso, app::Get().aniso); },
     [](uint32_t i) { app::Get().aniso = kAniso[i]; }},
    {"world textures", OnOff,
     [] { return uint32_t(app::Get().world_textures); },
     [](uint32_t i) { app::Get().world_textures = i != 0; }},
    {"sharp text and hud", OnOff,
     [] { return uint32_t(app::Get().sharp_2d); },
     [](uint32_t i) { app::Get().sharp_2d = i != 0; }},
};

const std::vector<Row> kPerformance = {
    {"render scale",
     [] { return Choices{"25%", "50%", "75%", "100%", "125%", "150%", "175%", "200%"}; },
     [] { return Nearest(kRender, app::Get().render_percent); },
     [](uint32_t i) { app::Get().render_percent = kRender[i]; }},
    {"scaling filter",
     [] { return Choices{"bilinear", "sharp"}; },
     [] { return app::Get().scale_filter % 2; },
     [](uint32_t i) { app::Get().scale_filter = i; }},
    {"frame rate cap",
     [] { return Choices{"30", "60", "120", "144", "240", "none"}; },
     [] { return NearestCap(app::Get().fps_cap); },
     [](uint32_t i) { app::Get().fps_cap = kCaps[i]; }},
    {"low latency", OnOff,
     [] { return uint32_t(app::Get().low_latency); },
     [](uint32_t i) { app::Get().low_latency = i != 0; }},
    {"frame counter", OnOff,
     [] { return uint32_t(app::Get().show_fps); },
     [](uint32_t i) { app::Get().show_fps = i != 0; }},
    {"renderer",
     [] { return Choices{"direct3d 12", "direct3d 11"}; },
     [] { return app::Get().renderer % 2; },
     [](uint32_t i) { app::Get().renderer = i; }},
};

const std::vector<Row> kDisplay = {
    {"monitor",
     [] {
         Choices c{"primary"};
         std::vector<std::string> names = app::Monitors();
         for (size_t i = 0; i < names.size() && c.size() < 8; i++) c.push_back("display " + std::to_string(i + 1));
         return c;
     },
     [] { return std::min<uint32_t>(app::Get().monitor, 7); },
     [](uint32_t i) {
         app::Get().monitor = i;
         app::ApplyDisplay();
     }},
    {"mode",
     [] { return Choices{"fullscreen", "borderless", "window"}; },
     [] { return uint32_t(app::Get().display_mode) % 3; },
     [](uint32_t i) {
         app::Get().display_mode = app::DisplayMode(i);
         app::ApplyDisplay();
     }},
    {"resolution",
     [] {
         Choices c{"desktop"};
         for (const app::Mode& m : ResolutionList()) c.push_back(std::to_string(m.width) + "x" + std::to_string(m.height));
         return c;
     },
     [] {
         const app::Settings& s = app::Get();
         std::vector<app::Mode> list = ResolutionList();
         for (size_t i = 0; i < list.size(); i++)
             if (list[i].width == s.width && list[i].height == s.height) return uint32_t(i + 1);
         return uint32_t(0);
     },
     [](uint32_t i) {
         app::Settings& s = app::Get();
         std::vector<app::Mode> list = ResolutionList();
         s.width = i && i <= list.size() ? list[i - 1].width : 0;
         s.height = i && i <= list.size() ? list[i - 1].height : 0;
         app::ApplyDisplay();
     }},
    {"vertical sync", OnOff,
     [] { return uint32_t(app::Get().vsync); },
     [](uint32_t i) { app::Get().vsync = i != 0; }},
    {"aspect ratio",
     [] { return Choices{"from screen", "4:3", "16:9", "fill screen"}; },
     [] { return app::Get().aspect % 4; },
     [](uint32_t i) { app::Get().aspect = i; }},
};

const std::vector<Row> kGame = {
    {"game frame rate", [] { return Choices{"60", "90", "120", "144"}; },
     [] { return Nearest(kRates, frame_rate); },
     [](uint32_t i) {
         frame_rate = kRates[i];
         // a cap below it would hold the game's frames back
         app::Settings& s = app::Get();
         if (s.fps_cap && s.fps_cap < frame_rate) s.fps_cap = frame_rate;
     }},
    {"field of view",
     [] { return Choices{"75%", "90%", "100%", "110%", "125%", "150%"}; },
     [] { return Nearest(kFov, fov_percent); },
     [](uint32_t i) { fov_percent = kFov[i]; }},
    {"race hud",
     [] { return Choices{"shown", "hidden"}; },
     [] { return uint32_t(hide_hud); },
     [](uint32_t i) { hide_hud = i != 0; }},
    {"radio music volume",
     [] { return Choices{"0%", "50%", "100%", "150%", "200%", "300%", "400%"}; },
     [] { return Nearest(kRadio, radio_percent); },
     [](uint32_t i) { radio_percent = kRadio[i]; }},
    {"skip intro", OnOff,
     [] { return uint32_t(skip_intro); },
     [](uint32_t i) { skip_intro = i != 0; }},
    {"pause in background", OnOff,
     [] { return uint32_t(app::Get().pause_unfocused); },
     [](uint32_t i) { app::Get().pause_unfocused = i != 0; }},
};

const std::vector<Row> kControls = {
    {"keyboard", OnOff,
     [] { return uint32_t(app::Get().keyboard); },
     [](uint32_t i) { app::Get().keyboard = i != 0; }},
    {"mouse look", OnOff,
     [] { return uint32_t(app::Get().mouse_look); },
     [](uint32_t i) { app::Get().mouse_look = i != 0; }},
    {"invert mouse", OnOff,
     [] { return uint32_t(app::Get().mouse_invert_y); },
     [](uint32_t i) { app::Get().mouse_invert_y = i != 0; }},
    {"mouse sensitivity",
     [] { return Choices{"25%", "50%", "75%", "100%", "150%", "200%", "400%", "800%"}; },
     [] { return Nearest(kSens, app::Get().mouse_sensitivity); },
     [](uint32_t i) { app::Get().mouse_sensitivity = kSens[i]; }},
    {"default keys",
     [] { return Choices{"keep", "reset"}; },
     [] { return uint32_t(0); },
     [](uint32_t i) {
         if (!i) return;
         app::Settings& s = app::Get();
         for (int c = 0; c < input::kControls && c < int(s.keys.size()); c++) s.keys[c] = app::DefaultKeys(c);
     }},
    {"default controller",
     [] { return Choices{"keep", "reset"}; },
     [] { return uint32_t(0); },
     [](uint32_t i) {
         if (!i) return;
         app::Settings& s = app::Get();
         for (int c = 0; c < input::kControls && c < int(s.pad_map.size()); c++) s.pad_map[c] = uint8_t(c);
     }},
};

const std::vector<Row> kWheel = {
    {"invert steering", OnOff,
     [] { return uint32_t(app::Get().wheel_steer.invert); },
     [](uint32_t i) {
         app::Get().wheel_steer.invert = i != 0;
         WheelChanged();
     }},
    {"wheel rotation",
     [] { return Choices{"180", "270", "360", "540", "720", "900", "1080", "2880"}; },
     [] { return Nearest(kWheelRange, app::Get().wheel_range); },
     [](uint32_t i) {
         app::Get().wheel_range = kWheelRange[i];
         WheelChanged();
     }},
    {"steering lock",
     [] { return Choices{"180", "270", "360", "450", "540", "720", "900", "1080"}; },
     [] { return Nearest(kWheelLock, app::Get().wheel_lock); },
     [](uint32_t i) {
         app::Get().wheel_lock = kWheelLock[i];
         WheelChanged();
     }},
    {"dead zone",
     [] { return Choices{"0%", "1%", "2%", "3%", "5%", "10%", "20%"}; },
     [] { return Nearest(kDead, app::Get().wheel_deadzone); },
     [](uint32_t i) {
         app::Get().wheel_deadzone = kDead[i];
         WheelChanged();
     }},
    {"force feedback",
     [] { return Choices{"off", "25%", "50%", "70%", "85%", "100%"}; },
     [] { return Nearest(kPercent, app::Get().ffb_strength); },
     [](uint32_t i) {
         app::Get().ffb_strength = kPercent[i];
         WheelChanged();
     }},
    {"road and impacts",
     [] { return Choices{"off", "25%", "50%", "70%", "85%", "100%"}; },
     [] { return Nearest(kPercent, app::Get().ffb_road); },
     [](uint32_t i) {
         app::Get().ffb_road = kPercent[i];
         WheelChanged();
     }},
};

struct Section {
    const char* label;
    const std::vector<Row>* rows;
};
const std::array<Section, kSectionCount> kSections = {{
    {"picture", &kPicture},
    {"performance", &kPerformance},
    {"display", &kDisplay},
    {"game", &kGame},
    {"controls", &kControls},
    {"wheel", &kWheel},
}};
const char* const kWindowLabel = "bindings and more";

// In the menus the spinner's own A confirms it. In the pause menu a row acts at once, so there a second press within
// three seconds leaves (QuitPress)
const Row kQuit = {"quit game", [] { return Choices{"no", "yes"}; }, [] { return uint32_t(0); },
                   [](uint32_t i) {
                       if (i) app::Quit();
                   }};
std::chrono::steady_clock::time_point g_quit_armed{};

bool QuitArmed() { return std::chrono::steady_clock::now() < g_quit_armed; }

void QuitPress() {
    if (QuitArmed()) app::Quit();
    else g_quit_armed = std::chrono::steady_clock::now() + std::chrono::seconds(3);
}

void Changed(const Row& row, uint32_t choice) {
    row.set(choice);
    app::Save();
    LOGR("pgr1: RECOMP %s = %u", row.label, choice);
}

void OpenWindow() {
    if (!app::OverlayOpen()) app::ToggleOverlay();
}

// ---- the front end: Options > RECOMP > a section > a row, whose A opens the game's own choice spinner.

// a row's choice table, as the game's: a header of 0x18 bytes (+4 the count, +0x10 1), then 0x18 bytes a choice:
// its text's id, its value, a handler on choosing it, the function that shows it on the row
struct Bound {
    const Row* row;
    uint32_t value;  // the int the spinner sets
};
std::map<uint32_t, Bound> g_tables;

uint32_t Table(const Row& row) {
    static std::map<const Row*, uint32_t> tables;
    Choices choices = row.choices();
    uint32_t& t = tables[&row];
    static std::map<uint32_t, uint32_t> sizes;
    if (!t || sizes[t] < choices.size()) {
        t = mem::AllocSystem(uint32_t(0x18 + 0x18 * std::max<size_t>(choices.size(), 8)));
        sizes[t] = uint32_t(std::max<size_t>(choices.size(), 8));
        g_tables[t] = {&row, mem::AllocSystem(4)};
    }
    std::memset(kernel::HostPtr(t), 0, 0x18 + 0x18 * choices.size());
    kernel::G<uint32_t>(t + 4) = uint32_t(choices.size());
    kernel::G<uint32_t>(t + 0x10) = 1;
    for (size_t i = 0; i < choices.size(); i++) {
        uint32_t e = t + 0x18 + uint32_t(i) * 0x18;
        kernel::G<uint32_t>(e) = Id(choices[i]);
        kernel::G<uint32_t>(e + 4) = uint32_t(i);
        kernel::G<uint32_t>(e + 8) = kAButton;  // comes back to AButton below, with the table
        kernel::G<uint32_t>(e + 0xC) = kShowChoice;
    }
    kernel::G<uint32_t>(g_tables[t].value) = row.get();
    return t;
}

uint32_t FocusByte(int page) {
    static std::array<uint32_t, kSections.size() + 1> bytes{};
    if (!bytes[page]) {
        bytes[page] = mem::AllocSystem(4);
        kernel::G<uint32_t>(bytes[page]) = 0;
    }
    return bytes[page];
}

void Finish(X86Context& ctx, std::vector<uint32_t>& items, int page, const char* title, uint32_t back, size_t leaving) {
    static uint32_t list = mem::AllocSystem(4 * 16);
    for (size_t i = 0; i < items.size(); i++) kernel::G<uint32_t>(list + uint32_t(i) * 4) = items[i];
    kernel::CallGuest(ctx, kLink, {list, uint32_t(items.size()), 0});
    for (size_t i = 0; i < items.size(); i++) {
        kernel::CallGuest(ctx, kOn, {items[i], 7, back});
        // the first items leave the page; on a row this would freeze its spinner, which stays on it
        if (i < leaving) kernel::CallGuest(ctx, kAlso, {items[i], 5, kLeavePage});
        kernel::CallGuest(ctx, kAlso, {items[i], 7, kBackPage});
    }
    uint32_t focus = kernel::G<uint8_t>(FocusByte(page));
    kernel::CallGuest(ctx, kFocus, {items[focus < items.size() ? focus : 0]});
    kernel::CallGuest(ctx, kTitle, {items[0], Guest(title), uint32_t(-1)});
}

void BuildLanding(X86Context& ctx) {
    kernel::CallGuest(ctx, kNewPage, {FocusByte(0)});
    std::vector<uint32_t> items;
    uint32_t n = uint32_t(kSections.size()) + 1;
    for (uint32_t i = 0; i < n; i++) {
        bool window = i == kSections.size();
        uint32_t item = kernel::CallGuest(ctx, kItem, {i, Guest(window ? kWindowLabel : kSections[i].label), Icon(i), kAButton});
        kernel::G<uint32_t>(item + 0x58) = kTag | (window ? kOpenWindow : i);
        items.push_back(item);
    }
    uint32_t t = Table(kQuit);
    items.push_back(kernel::CallGuest(ctx, kRow, {n, Guest(kQuit.label), Icon(n), t, kernel::G<uint32_t>(t + 4), g_tables[t].value}));
    kernel::G<uint32_t>(items.back() + 0x58) = 0;
    Finish(ctx, items, 0, "RECOMP", PGR1_VA(0x5CE80, 0x5CEA0, 0x5C340), n);
}

void BuildSection(X86Context& ctx, uint32_t section) {
    kernel::CallGuest(ctx, kNewPage, {FocusByte(int(section) + 1)});
    std::vector<uint32_t> items;
    const std::vector<Row>& rows = *kSections[section].rows;
    for (uint32_t i = 0; i < rows.size(); i++) {
        uint32_t t = Table(rows[i]);
        items.push_back(kernel::CallGuest(ctx, kRow, {i, Guest(rows[i].label), Icon(i), t,
                                                      kernel::G<uint32_t>(t + 4), g_tables[t].value}));
        kernel::G<uint32_t>(items.back() + 0x58) = 0;  // the row's own: its current choice
    }
    Finish(ctx, items, int(section) + 1, kSections[section].label, kBButton, 0);
    for (uint32_t item : items) kernel::G<uint32_t>(item + 0x6C) = kTag;  // B from here: to the landing page
}

// ---- the pause menu: a table of entries (0x34 bytes: the text's id, a handler (menu, entry, action: 0 start,
// 1 measure, 2 draw, 3 A, 4 left, 5 right), a submenu, ...; id -1 ends it). A submenu is a menu of 0x2C bytes.

constexpr uint32_t kEntrySize = 0x34, kMenuSize = 0x2C;
struct PauseRow {
    const Row* row;
    uint32_t text;  // the row's line, rewritten as its choice changes
    int id;
    Choices choices;
};
std::map<uint32_t, PauseRow> g_pause_rows;
std::map<uint32_t, int> g_pause_window;
uint32_t g_pause_quit = 0;
uint32_t g_top = 0;  // RECOMP's own menu
uint32_t g_root_static = 0, g_root_copy = 0, g_root_count = 0;

std::map<uint32_t, int> g_my_menus;

uint32_t NewMenu(uint32_t entries, uint32_t parent) {
    uint32_t m = mem::AllocSystem(kMenuSize);
    std::memset(kernel::HostPtr(m), 0, kMenuSize);
    kernel::G<uint32_t>(m) = entries;
    kernel::G<uint32_t>(m + 4) = kMenuDraw;
    kernel::G<uint32_t>(m + 8) = kMenuInput;
    kernel::G<uint32_t>(m + 0xC) = parent;
    g_my_menus[m] = 1;
    return m;
}

uint32_t NewEntries(uint32_t count) {
    uint32_t e = mem::AllocSystem((count + 1) * kEntrySize);
    std::memset(kernel::HostPtr(e), 0, (count + 1) * kEntrySize);
    kernel::G<int32_t>(e + count * kEntrySize) = -1;
    return e;
}

void SetEntry(uint32_t e, uint32_t id, uint32_t submenu) {
    kernel::G<uint32_t>(e) = id;
    kernel::G<uint32_t>(e + 4) = kMenuItem;
    kernel::G<uint32_t>(e + 8) = submenu;
}

void PauseLine(const PauseRow& r) {
    uint32_t get = r.row->get();
    std::string line = std::string(r.row->label) + "  < " + (get < r.choices.size() ? r.choices[get] : "") + " >";
    if (r.row == &kQuit) line = QuitArmed() ? "quit game  < press again >" : "quit game";
    if (line.size() > 40) line.resize(40);
    // the menu prints its text through a format
    std::string escaped;
    for (char c : line) {
        escaped += c;
        if (c == '%') escaped += c;
    }
    line = escaped;
    std::memcpy(kernel::HostPtr(r.text), line.c_str(), line.size() + 1);
    kernel::G<uint32_t>(Strings() + r.id * 4) = r.text;
}

}  // namespace

// a copy of the pause menu's entries with RECOMP above quit race (the game's table has no room), its sections and
// their rows. GameInfo::loaded: the image is in memory, the game not started
void SettingsLoaded() {
    uint32_t root = Va(0x159D3C), entries = kernel::R32(root);
    uint32_t count = 0;
    while (kernel::G<int32_t>(entries + count * kEntrySize) != -1) count++;
    uint32_t copy = NewEntries(count + 1);
    for (uint32_t i = 0, to = 0; i < count; i++, to++) {
        uint32_t from = entries + i * kEntrySize;
        if (i == count - 1) to++;  // quit race stays last
        std::memcpy(kernel::HostPtr(copy + to * kEntrySize), kernel::HostPtr(from), kEntrySize);
    }
    uint32_t top_entries = NewEntries(uint32_t(kSections.size()) + 2);
    uint32_t top = NewMenu(top_entries, root);
    g_top = top;
    SetEntry(copy + (count - 1) * kEntrySize, kLastId, top);
    for (uint32_t s = 0; s < kSections.size(); s++) {
        const std::vector<Row>& rows = *kSections[s].rows;
        uint32_t row_entries = NewEntries(uint32_t(rows.size()));
        for (uint32_t i = 0; i < rows.size(); i++) {
            uint32_t e = row_entries + i * kEntrySize;
            SetEntry(e, kFirstRowId + i, 0);
            g_pause_rows[e] = {&rows[i], mem::AllocSystem(64), int(kFirstRowId + i), {}};
        }
        SetEntry(top_entries + s * kEntrySize, kLastId - 1 - s, NewMenu(row_entries, top));
    }
    uint32_t window = top_entries + uint32_t(kSections.size()) * kEntrySize;
    SetEntry(window, kLastId - 1 - uint32_t(kSections.size()), 0);
    g_pause_window[window] = 1;
    g_pause_quit = window + kEntrySize;
    SetEntry(g_pause_quit, kFirstTextId - 1, 0);
    g_pause_rows[g_pause_quit] = {&kQuit, mem::AllocSystem(64), kFirstTextId - 1, {}};
    kernel::G<uint32_t>(root) = copy;
    g_root_static = entries;
    g_root_copy = copy;
    g_root_count = count;
    LOGR("pgr1: RECOMP in the pause menu (%u entries)", count + 1);
}

}  // namespace pgr1

using namespace pgr1;

// The Options page: RECOMP after its eight items, in its array of twenty (0x5CE80, the link at 0x5D021)
X86_EXTERN_FUNC(PGR1_ORIG(00064CC0, 00064CE0, 00063E60));
X86_FUNC(PGR1_FN(00064CC0, 00064CE0, 00063E60)) {
    bool options = kernel::R32(ctx.esp) == PGR1_VA(0x5D026, 0x5D046, 0x5C4E6) && kernel::R32(ctx.esp + 8) == 8;
    uint32_t list = kernel::R32(ctx.esp + 4), item = 0;
    if (options) {
        item = kernel::CallGuest(ctx, kItem, {8, Guest("RECOMP"), Icon(0), kAButton});
        kernel::G<uint32_t>(item + 0x58) = kTag | kOpenPage;
        kernel::G<uint32_t>(list + 8 * 4) = item;
        kernel::G<uint32_t>(ctx.esp + 8) = 9;
    }
    PGR1_ORIG(00064CC0, 00064CE0, 00063E60)(ctx, base);
    if (options) {
        kernel::CallGuest(ctx, kOn, {item, 7, kOptionsBack});
        kernel::CallGuest(ctx, kAlso, {item, 5, kLeavePage});
        kernel::CallGuest(ctx, kAlso, {item, 7, kBackPage});
    }
}

// A on a RECOMP item, or a choice taken in a RECOMP spinner: otherwise the game's Time Attack Options
X86_EXTERN_FUNC(PGR1_ORIG(0005B180, 0005B1A0, 0005A690));
X86_FUNC(PGR1_FN(0005B180, 0005B1A0, 0005A690)) {
    uint32_t w = kernel::R32(ctx.esp + 4);
    auto bound = w ? g_tables.find(kernel::R32(w + 0x64)) : g_tables.end();
    if (bound != g_tables.end()) {
        Changed(*bound->second.row, kernel::R32(bound->second.value));
        ctx.esp += 4;  // ret
        return;
    }
    uint32_t tag = w ? kernel::R32(w + 0x58) : 0;
    if ((tag & 0xFFFF0000) == kTag) {
        uint32_t what = tag & 0xFFFF;
        if (what == kOpenPage) BuildLanding(ctx);
        else if (what == kOpenWindow) OpenWindow();
        else BuildSection(ctx, what);
        ctx.esp += 4;  // ret
        return;
    }
    PGR1_ORIG(0005B180, 0005B1A0, 0005A690)(ctx, base);
}

// B on a section's row: otherwise the game's own page
X86_EXTERN_FUNC(PGR1_ORIG(0005B080, 0005B0A0, 0005A590));
X86_FUNC(PGR1_FN(0005B080, 0005B0A0, 0005A590)) {
    uint32_t w = kernel::R32(ctx.esp + 4);
    if (w && kernel::R32(w + 0x6C) == kTag) {
        BuildLanding(ctx);
        ctx.esp += 4;  // ret
        return;
    }
    PGR1_ORIG(0005B080, 0005B0A0, 0005A590)(ctx, base);
}

// the pause menu's plain item: RECOMP's section lists and rows use it, each entry told apart by its address
X86_EXTERN_FUNC(PGR1_ORIG(00083BE0, 00083C00, 000828C0));
X86_FUNC(PGR1_FN(00083BE0, 00083C00, 000828C0)) {
    uint32_t entry = kernel::R32(ctx.esp + 8), action = kernel::R32(ctx.esp + 12);
    auto it = g_pause_rows.find(entry);
    if (it != g_pause_rows.end()) {
        PauseRow& r = it->second;
        if (r.choices.empty()) r.choices = r.row->choices();
        if (entry == g_pause_quit && action >= 3 && action <= 5) {
            QuitPress();
            ctx.esp += 4;  // ret
            return;
        }
        // like the section list, not under a section's rows
        if (entry == g_pause_quit && action == 2 && kernel::R32(Va(0x192664)) != g_top) {
            ctx.esp += 4;  // ret
            return;
        }
        if (action >= 3 && action <= 5) {
            uint32_t n = uint32_t(r.choices.size()), now = r.row->get();
            Changed(*r.row, action == 4 ? (now + n - 1) % n : (now + 1) % n);
            if (kernel::CallGuest(ctx, kCanSound, {}) & 0xFF) kernel::CallGuest(ctx, kStepSound, {});
            ctx.esp += 4;  // ret
            return;
        }
        PauseLine(r);
    } else if (g_pause_window.count(entry) && action == 3) {
        OpenWindow();
        ctx.esp += 4;  // ret
        return;
    }
    // the section list's and RECOMP's own texts
    uint32_t id = kernel::R32(entry);
    // a section open: the list would lie under its rows, the game's own menus being two deep (0x192664 the menu open)
    if (action == 2 && id < uint32_t(kLastId) && int(id) >= kLastId - 1 - kSectionCount && it == g_pause_rows.end() &&
        kernel::R32(Va(0x192664)) != g_top) {
        ctx.esp += 4;  // ret
        return;
    }
    if (it == g_pause_rows.end() && int(id) > kLastId - 1 - int(kSections.size()) - 1 && int(id) <= kLastId) {
        int i = kLastId - 1 - int(id);
        const char* text = id == uint32_t(kLastId) ? "recomp"
                           : i < int(kSections.size()) ? kSections[i].label : kWindowLabel;
        kernel::G<uint32_t>(Strings() + id * 4) = Guest(text);
    }
    PGR1_ORIG(00083BE0, 00083C00, 000828C0)(ctx, base);
}

// the root menu's frame: the game hides items (reset ghost car, screen split) by flag in its own table, so the flags
// follow into the copy
X86_EXTERN_FUNC(PGR1_ORIG(00085D20, 00085D40, 000849F0));
X86_FUNC(PGR1_FN(00085D20, 00085D40, 000849F0)) {
    for (uint32_t i = 0, to = 0; i < g_root_count; i++, to++) {
        if (i == g_root_count - 1) to++;
        kernel::G<uint32_t>(g_root_copy + to * kEntrySize + 0x10) = kernel::R32(g_root_static + i * kEntrySize + 0x10);
    }
    // recomp only in a race of one's own: not in a recording played back (0x2E71C8) or multiplayer (mode 0x2C6F80, 6)
    bool hide = kernel::R32(Va(0x2E71C8)) != 0 || kernel::R32(Va(0x2C6F80)) == 6;
    kernel::G<uint32_t>(g_root_copy + (g_root_count - 1) * kEntrySize + 0x10) = hide;
    PGR1_ORIG(00085D20, 00085D40, 000849F0)(ctx, base);
}

// RECOMP's menus start beside the item that opened them, as the game's do, but are longer: moved up to end above
// "Press START" (a menu's +0x24 is that item, whose +0x20 is its height). USA's pause text is larger
X86_EXTERN_FUNC(PGR1_ORIG(00086080, 000860A0, 00084D50));
X86_FUNC(PGR1_FN(00086080, 000860A0, 00084D50)) {
    constexpr float kBottom = 400.0f, kStep = PGR1_RELEASE == 2 ? 32.0f : 23.0f;
    uint32_t menu = kernel::R32(ctx.esp + 4), opener = kernel::R32(menu + 0x24);
    if (g_my_menus.count(menu) && opener) {
        static uint32_t fake = mem::AllocSystem(kEntrySize);
        uint32_t n = 0, entries = kernel::R32(menu);
        while (kernel::G<int32_t>(entries + n * kEntrySize) != -1) n++;
        float y;
        uint32_t bits = kernel::R32(opener + 0x20);
        std::memcpy(&y, &bits, 4);
        y = std::min(y, kBottom - float(n) * kStep);
        std::memcpy(&bits, &y, 4);
        kernel::G<uint32_t>(fake + 0x20) = bits;
        kernel::G<uint32_t>(menu + 0x24) = fake;
        PGR1_ORIG(00086080, 000860A0, 00084D50)(ctx, base);
        kernel::G<uint32_t>(menu + 0x24) = opener;
        return;
    }
    PGR1_ORIG(00086080, 000860A0, 00084D50)(ctx, base);
}
