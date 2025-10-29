![IronMog FF7 Logo](img/logo.png?raw=true "IronMog FF7 Logo")

Inspired by IronMon and IronMario 64, IronMog FF7 is a modification for Final Fantasy 7 (PSX) that combines a randomizer with a difficult ruleset to create a new challange.

# Rules
| Rule | Description |
| ---- | --- |
| No Escapes | Escaping from battles is prohibited, including the use of Exit materia. |
| No Limit Breaks | Limit breaks are disabled. |
| No Summon Materia | Summon materia cannot be found or purchased. |
| Permadeath Characters | If a character dies, they cannot be revived and will remain dead for the rest of the playthrough. |
| Randomized Encounters | Field and world map encounters are randomized to any enemy formation within 5 levels of the one originally encountered. |
| Randomized E.Skills | When you learn an Enemy Skill from an enemy, the skill you actually gain is randomized. |
| Randomized Enemy Drops | Enemy drops and steals are randomized. |
| Randomized Field Items | Any items obtained from the field (such as from boxes or chests) are randomized. |
| Randomized Shops | Shop inventories are randomized. |
| Randomized World Map | World map entrances are shuffled — entering Kalm might take you to Midgar. |

# Extras
| Extra | Description |
| ---- | --- |
| Randomized Colors | Playable characters’ clothing colors are randomized. |
| Randomized Music | Music tracks are randomized. This feature requires additional setup steps, described [here](#music-randomization). |

# How to Play
- Download the latest IronMog FF7 build from the Releases page
- Open your emulator (DuckStation or BizHawk) and load your FF7 rom
- Open IronMogFF7.exe
- Select your emulator type and press Attach
- Begin playing

# How to Build
Currently only Windows platforms are supported and Microsoft Visual Studio 2022 is recommended.
- Clone the repository
- Run `GenerateProjectFiles.bat`
- Open the generated `IronMogFF7.sln`
- Build

# Music Randomization
Due not wanting to include copywritten music on github music randomization requires obtaining or building a music folder of mp3s yourself.
