// PGR1's achievements (FINDINGS.md, "Achievements", has the memory map). Conditions are in RetroAchievements' syntax so
// the set can go there as it is.
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

#include "core/achievements.h"

namespace pgr1 {

namespace {

// The driver profile is one block, 0x2C68E0 to 0x2CDE20, read whole from the save by 0xD0210 and written by 0xD0350:
// loading a driver rewrites every value below in one frame. The saved Kudos total (int, 0x2C6900) is stored by 0x57B00
// only on leaving a results screen (0x6B837, the autosave 0xD1180) and on a load, never on the frame a race is decided:
// unchanged, it marks a frame where the game itself moved a value, not a load.
#define PGR1_NOT_LOADED "d0xX2c6900=0xX2c6900"

// Kudos Challenge: 13 levels (frontend\chap01-13.ini, the table 0x2E4D20). At each level race's end 0x6A1A7 stores the
// level's summary byte 0x2C6C2C + level (0x46110: bit 0 a medal in every race, the level complete and the next opened;
// bit 1 all silver or better; bit 2 all gold)
constexpr uint32_t kLevelSummary = 0x2C6C2C;
constexpr int kLevels = 13;

// the level's summary bit newly set by the game
const char* LevelBit(int level, char bit) {
    static std::deque<std::string> kept;
    char b[96];
    std::snprintf(b, sizeof b, "d0x%c%x=0_0x%c%x=1_" PGR1_NOT_LOADED, bit, kLevelSummary + level, bit, kLevelSummary + level);
    return kept.emplace_back(b).c_str();
}

// any level's gold bit newly set: one alternative group each
const char* AnyLevelGold() {
    static std::string c = PGR1_NOT_LOADED;
    if (c.size() == sizeof PGR1_NOT_LOADED - 1)
        for (int i = 0; i < kLevels; i++) {
            char b[48];
            std::snprintf(b, sizeof b, "Sd0xO%x=0_0xO%x=1", kLevelSummary + i, kLevelSummary + i);
            c += b;
        }
    return c.c_str();
}

// the sum of bits at the addresses (each address once per car it counts for), now at least n and last frame below it
const char* SumReached(const std::vector<uint32_t>& addrs, char bit, int n, const char* also) {
    static std::deque<std::string> kept;
    std::string now, before;
    for (size_t i = 0; i < addrs.size(); i++) {
        char b[32];
        bool last = i + 1 == addrs.size();
        std::snprintf(b, sizeof b, "%s0x%c%x%s", last ? "" : "A:", bit, addrs[i], last ? "" : "_");
        now += b;
        std::snprintf(b, sizeof b, "%sd0x%c%x%s", last ? "" : "A:", bit, addrs[i], last ? "" : "_");
        before += b;
    }
    return kept.emplace_back(now + ">=" + std::to_string(n) + "_" + before + "<" + std::to_string(n) + also).c_str();
}

// Every level gold: the 13 gold bits
std::vector<uint32_t> LevelGolds() {
    std::vector<uint32_t> a;
    for (int i = 0; i < kLevels; i++) a.push_back(kLevelSummary + uint32_t(i));
    return a;
}

// Cars: frontend cars.ini's 26 groups (29 cars; the table 0x2E4E58), group g unlocked when bit 0 of the dword at
// 0x2CD765 + 4 g is set. Groups 0-2 (MINI, MR2, SLK) are owned from the start (the profile reset 0x53A70); the other 23
// hold 26 cars (TT, 355 and 360 two each): the One on One wins (0x6AAF0), Quick and Arcade Race levels (0x425A0) and the
// Kudos totals of extras.ini (0x6BB10). Unlocks come one group at a time; a load jumps by many, hence "the last
// frame one or two below"
std::vector<uint32_t> UnlockableCars() {
    std::vector<uint32_t> a;
    for (int g = 3; g < 26; g++) {
        uint32_t addr = 0x2CD765 + uint32_t(g) * 4;
        a.push_back(addr);
        if (addr == 0x2CD775 || addr == 0x2CD7A5 || addr == 0x2CD7B5) a.push_back(addr);  // two cars in the group
    }
    return a;
}

// the sum of the bits now at least n, and last frame within two of it (one unlock, not a load)
const char* CarsReached(int n) {
    static std::deque<std::string> kept;
    std::string before;
    auto cars = UnlockableCars();
    for (size_t i = 0; i < cars.size(); i++) {
        char b[32];
        bool last = i + 1 == cars.size();
        std::snprintf(b, sizeof b, "%sd0xM%x%s", last ? "" : "A:", cars[i], last ? "" : "_");
        before += b;
    }
    std::string c = SumReached(cars, 'M', n, "");
    return kept.emplace_back(c + "_" + before + ">=" + std::to_string(n - 2)).c_str();
}

// Quick Race (mode 2) and Arcade Race (mode 3) have four levels each; 0x425A0 sets bit 0 of 0x2C6C39 + 4 (mode 3) +
// level when one is completed
const char* ModeComplete(uint32_t first) {
    static std::deque<std::string> kept;
    return kept.emplace_back(SumReached({first, first + 1, first + 2, first + 3}, 'M', 4, "_" PGR1_NOT_LOADED)).c_str();
}

// The saved Kudos total: last frame above 0 (a profile loaded) and below the mark, now at it
#define PGR1_KUDOS(n) "d0xX2c6900>0_d0xX2c6900<" #n "_0xX2c6900>=" #n

// Player 1 racing, not the attract demo or a replay: 0x2E71C8 is 1 while 0x69160 plays a recording (0x15DD04)
#define PGR1_LIVE "0xX2e71c8=0_"
// A race won: when the last lap completes, 0x887A0 sets in one frame (player 1's record 0x2E5B10) race over 0x2E5D49,
// the finishing position 0x2E5D51 (1 first) and finished properly 0x2E5D4D (a challenge timing out sets only the
// first); race setup (0x69060) clears them
#define PGR1_FINISHED PGR1_LIVE "d0xX2e5d49=0_0xX2e5d49=1_0xX2e5d4d=1_"
#define PGR1_WON PGR1_FINISHED "0xX2e5d51=1_"
// The race type 0x2C6982 (0x49970): races against other cars, 3 One on One, 5 Street Race, 9 multiplayer, 10 Quick
// Race (not the solo hot laps, timed runs and Kudos races)
#define PGR1_VERSUS "S0xX2c6982=3S0xX2c6982=5S0xX2c6982=9S0xX2c6982=10"
// Player 1's Kudos in the race, 0x2C6CD9 (reset by 0x69060; inside the profile, so read with the load guard): int16
// counts, contacts +6 (4 a hit, 0x45760), each Kudos type's awards from +8 (two wheels 0x2C6CE9, cone gates 0x2C6CF5;
// the names at 0x2E7400), the combo +0x2E (0x56CD0 counts it, a crash zeroes it)

}  // namespace

std::vector<achievements::Def> AchievementSet() {
    static const char* const kLevel[kLevels][4] = {
        {"level_introduction", "First Steps", "Complete the Introduction level", "2"},
        {"level_amateur", "Amateur Hour", "Complete the Amateur Season", "3"},
        {"level_san_francisco", "Golden Gate", "Complete the San Francisco level", "3"},
        {"level_london", "Mind the Gap", "Complete the London level", "5"},
        {"level_tokyo", "Rising Sun", "Complete the Tokyo level", "5"},
        {"level_new_york", "Empire State", "Complete the New York level", "5"},
        {"level_professional", "Going Pro", "Complete the Professional Season", "10"},
        {"level_world_tour", "Globetrotter", "Complete the World Tour", "10"},
        {"level_challenge_tour", "Challenge Accepted", "Complete the Challenge Tour", "10"},
        {"level_endurance", "Long Haul", "Complete the Endurance Season", "10"},
        {"level_world_class", "World Class", "Complete the World Class Season", "25"},
        {"level_invitational", "By Invitation Only", "Complete the Invitational Season", "25"},
        {"level_bonus", "Kudos King", "Complete the Bonus Level", "25"},
    };
    std::vector<achievements::Def> set;
    for (int i = 0; i < kLevels; i++)
        set.push_back({kLevel[i][0], kLevel[i][1], kLevel[i][2], uint32_t(std::stoul(kLevel[i][3])), LevelBit(i, 'M')});
    set.insert(set.end(), {
        {"golden_touch", "Golden Touch", "Gold in every challenge of a Kudos Challenge level", 5, AnyLevelGold()},
        {"gold_standard", "Gold Standard", "Gold in every challenge of every Kudos Challenge level", 50,
         SumReached(LevelGolds(), 'O', kLevels, "_" PGR1_NOT_LOADED)},
        {"quick_race", "Quick Study", "Complete every Quick Race level", 5, ModeComplete(0x2C6C39)},
        {"arcade_race", "Arcade Ace", "Complete every Arcade Race level", 10, ModeComplete(0x2C6C3D)},
        {"kudos_10000", "Showboat", "Earn 10,000 Kudos in total", 3, PGR1_KUDOS(10000)},
        {"kudos_100000", "Crowd Pleaser", "Earn 100,000 Kudos in total", 10, PGR1_KUDOS(100000)},
        {"kudos_200000", "Legend", "Earn 200,000 Kudos in total", 25, PGR1_KUDOS(200000)},
        {"cars_10", "Collector", "Unlock 10 cars", 5, CarsReached(10)},
        {"cars_all", "Full Garage", "Unlock every car", 25, CarsReached(26)},
    });
    set.insert(set.end(), {
        // in a race
        {"combo_x10", "Combo Artist", "Reach a x10 Kudos combo", 10, PGR1_LIVE "d0x 2c6d07<10_0x 2c6d07>=10_" PGR1_NOT_LOADED},
        {"two_wheels", "Two Wheels Good", "Drive on two wheels for a second in a race", 5,
         PGR1_LIVE "d0x 2c6ce9<60_0x 2c6ce9>=60_" PGR1_NOT_LOADED},
        {"cone_gates", "Cone Master", "Pass 15 cone gates in one race", 5, PGR1_LIVE "d0x 2c6cf5<15_0x 2c6cf5>=15_" PGR1_NOT_LOADED},
        {"clean_win", "Clean Sweep", "Win a race against other cars without touching a wall or a car", 10,
         PGR1_WON "0x 2c6cdf=0" PGR1_VERSUS},
        // the gearbox chosen before the race (0x2C694E, 0 manual, by 0x6CB70), player 1's copy in the race setup
        {"manual_win", "Manual Labour", "Win a race against other cars with the manual gearbox", 5,
         PGR1_WON "0xX2c6e52=0" PGR1_VERSUS},
        // night: 0x68190 stores at race setup whether the race's hour (0x2E7208) is outside the city's daylight
        {"night_win", "Night Owl", "Win a race against other cars at night", 5, PGR1_WON "0xX2e71bc!=0" PGR1_VERSUS},
        {"one_on_one", "Head to Head", "Win a One on One race", 5, PGR1_WON "0xX2c6982=3"},
        // Medal Pursuit (mode 7, car_chal.ini): 0x696A0 counts each new medal in 0x2C6CC1 at the race's result
        {"medal_pursuit", "Against the Clock", "Earn a new medal in Medal Pursuit", 10,
         PGR1_LIVE "0xX2c6f80=7_d0xX2c6cc1<0xX2c6cc1_" PGR1_NOT_LOADED},
        // multiplayer (mode 6) with two or more players at the console (0x2C695E)
        {"split_screen", "Split Decision", "Finish a split screen race", 3, PGR1_FINISHED "0xX2c6f80=6_0xX2c695e>=2"},
    });
    return set;
}

}  // namespace pgr1
