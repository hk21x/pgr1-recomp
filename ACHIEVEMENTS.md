# PGR1 achievements

31 achievements, 334 points, shown in the port (pop-ups and F1 > Achievements, unlocks kept in `achievements.toml`).
Each is a condition on the game's memory in [RetroAchievements](https://retroachievements.org)' syntax, so the set
could go there as it is if it ever supports the original Xbox. The conditions are in
[src/game/achievement_set.cpp](src/game/achievement_set.cpp), the memory map in [FINDINGS.md](FINDINGS.md).

Rules, as PGR2's: every achievement fires at the moment the game itself awards something, never on loading a
finished profile (the driver profile loads in one frame, and the saved Kudos total, which changes only on a load and
on leaving a results screen, guards the career ones); none fires in the attract demo or a replay; medals are the
game's own.

## Kudos Challenge

The 13 Kudos Challenge levels are PGR1's seasons, cities and tours. A level is complete when every challenge in it
has a medal; that opens the next.

| Achievement | Description | Points |
| --- | --- | --- |
| First Steps | Complete the Introduction level | 2 |
| Amateur Hour | Complete the Amateur Season | 3 |
| Golden Gate | Complete the San Francisco level | 3 |
| Mind the Gap | Complete the London level | 5 |
| Rising Sun | Complete the Tokyo level | 5 |
| Empire State | Complete the New York level | 5 |
| Going Pro | Complete the Professional Season | 10 |
| Globetrotter | Complete the World Tour | 10 |
| Challenge Accepted | Complete the Challenge Tour | 10 |
| Long Haul | Complete the Endurance Season | 10 |
| World Class | Complete the World Class Season | 25 |
| By Invitation Only | Complete the Invitational Season | 25 |
| Kudos King | Complete the Bonus Level | 25 |
| Golden Touch | Gold in every challenge of a Kudos Challenge level | 5 |
| Gold Standard | Gold in every challenge of every Kudos Challenge level | 50 |

## Other modes

| Achievement | Description | Points |
| --- | --- | --- |
| Quick Study | Complete every Quick Race level | 5 |
| Arcade Ace | Complete every Arcade Race level | 10 |
| Against the Clock | Earn a new medal in Medal Pursuit | 10 |
| Head to Head | Win a One on One race | 5 |
| Split Decision | Finish a split screen race | 3 |

## Kudos and cars

| Achievement | Description | Points |
| --- | --- | --- |
| Showboat | Earn 10,000 Kudos in total | 3 |
| Crowd Pleaser | Earn 100,000 Kudos in total | 10 |
| Legend | Earn 200,000 Kudos in total | 25 |
| Collector | Unlock 10 cars | 5 |
| Full Garage | Unlock every car | 25 |

## In a race

| Achievement | Description | Points |
| --- | --- | --- |
| Combo Artist | Reach a x10 Kudos combo | 10 |
| Two Wheels Good | Drive on two wheels for a second in a race | 5 |
| Cone Master | Pass 15 cone gates in one race | 5 |
| Clean Sweep | Win a race against other cars without touching a wall or a car | 10 |
| Manual Labour | Win a race against other cars with the manual gearbox | 5 |
| Night Owl | Win a race against other cars at night | 5 |

## Changes from the draft

- The six seasons, the city challenges and the World Tour are levels of the Kudos Challenge, so each level has its own
  achievement. There is no World Champion season: championships.ini, champs.ini and fin_chap.ini are never loaded.
  Quick Race and Arcade Race take its place.
- Legend is 200,000 Kudos, the game's own last Kudos reward (the F50); the levels' Kudos add up to about 318,000.
- Time Attack has no targets; Against the Clock is Medal Pursuit's medals instead. Head to Head mode cannot be reached;
  Head to Head is the One on One races.
- Cone Master counts gates in one race: the number of gates in a race is not known yet.

## Tested

Headless, with the game's memory set as the game sets it: a level (Amateur Hour), Golden Touch, Showboat, Crowd
Pleaser, Collector, Combo Artist, Clean Sweep, Manual Labour and Head to Head fire at once; Night Owl stays locked by
day; nothing fires when the whole profile changes in one frame (as a load does) or in a normal race. The real level
completion was also followed through the game (the summary byte set at the race's end, the total unchanged). Traced from the game's code but not yet
earned in play: Gold Standard, Full Garage, Quick Study, Arcade Ace, Against the Clock, Split Decision, Two Wheels
Good, Cone Master and Night Owl.
