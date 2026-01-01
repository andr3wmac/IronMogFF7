<p align="center">
  <img src="img/logo.png?raw=true" alt="IronMog FF7 Logo" title="IronMog FF7 Logo">
</p>

Inspired by IronMon and IronMario 64, IronMog FF7 is a modification for Final Fantasy 7 (PS1, emulator only) that combines a randomizer with a difficult ruleset to create a new challange.

# Rules
| Rule | Description |
| ---- | --- |
| No Escapes | Escaping from battles is prohibited, including the use of Exit materia. |
| No Limit Breaks | Limit breaks are disabled. |
| No Saving | Saving your game is prohibited. |
| No Summon Materia | Summon materia cannot be found or purchased. |
| Permadeath Characters | If a character dies, they cannot be revived and will remain dead for the rest of the playthrough. |
| Randomized Encounters | Field and world map encounters are randomized to any enemy formation within 5 levels of the one originally encountered. |
| Randomized Enemy Drops | Enemy drops and steals are randomized. |
| Randomized E.Skills | When you learn an Enemy Skill from an enemy, the skill you actually gain is randomized. |
| Randomized Field Items | Any items obtained from the field (such as from boxes or chests) are randomized. |
| Randomized Shops | Shop inventories are randomized. |
| Randomized World Map | World map entrances are shuffled — entering Kalm might take you to Midgar. |

# Extras
| Extra | Description |
| ---- | --- |
| Randomized Colors | Playable characters’ clothing colors are randomized. |
| Randomized Music | Music tracks are randomized and can include music from other games. This feature requires additional setup steps, described [here](music/README.md). |

# Requirements
- Windows
- PS1 Emulator (DuckStation or BizHawk recommended)
- Final Fantasy VII US NTSC (SCUS-94163)

# How to Play
- Download the latest IronMog FF7 build from the Releases page
- Open your emulator (DuckStation or BizHawk) and load Final Fantasy VII
- Open IronMogFF7.exe
- Wait until game is at the main menu
- Select your emulator type and press Connect
- Begin playing

# How to Build
Currently only Windows platforms are supported and Microsoft Visual Studio 2022 is recommended.
- Clone the repository
- Run `GenerateProjectFiles.bat`
- Open the generated `IronMogFF7.sln`
- Build

# Resources
The following resources were referenced during development:
- [FF7 Memory Values Spreadsheet](https://docs.google.com/spreadsheets/d/13wBwF1yRejK7o9IJSIVNzrzTeK5qHFpGnqi7NgB6QQQ/edit?gid=416136141#gid=416136141)
- [ff7-lib.rs](https://github.com/maciej-trebacz/ff7-lib.rs)
- [FF7 - Final Fantasy Inside](https://wiki.ffrtt.ru/index.php/FF7)
- [Qhimm.com Forums](https://forums.qhimm.com/)
