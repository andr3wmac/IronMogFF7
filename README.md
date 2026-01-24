<p align="center">
  <img src="resources/logo.png?raw=true" alt="IronMog FF7 Logo" title="IronMog FF7 Logo">
</p>

Inspired by IronMon and IronMario 64, IronMog FF7 is a modification for Final Fantasy 7 (PS1, emulator only) that combines a randomizer with a difficult ruleset to create a new challange.

The mod is currently in beta and the official IronMog challenge in not yet defined. For now it only functions as a flexible challenge randomizer until enough testing is done to come to a consensus on the official ruleset for the challenge.

# Requirements
- Windows
- PS1 Emulator (DuckStation or BizHawk recommended)
- Final Fantasy VII US NTSC (SCUS-94163)
  
For now it's recommended to configure your emulator with default settings and avoid enabling things like Runahead. It's also not recommended to use emulator features such as Fast Forward, or Rewind.

# How to Play
- Download the latest IronMog FF7 build from the Releases page
- Open your emulator (DuckStation or BizHawk) and load Final Fantasy VII
- Open IronMogFF7.exe
- Wait until game is at the main menu
- Select your emulator type, configure your settings, and press Connect
- Once the light turns green you can start a new game

# Music Randomization
Music randomization works by muting the in-game music and then selecting and playing a random mp3 from a folder thats associated with the song the game is currently playing. Due to copyright issues the mp3s are not included in this repository nor are they in release files. Instead, a python script is provided that can download, process, and build the music folder featuring songs from Final Fantasy VI, VII, VIII, and XI.

# How to Build
Currently only Windows platforms are supported and Microsoft Visual Studio 2022 is recommended.
- Clone the repository
- Run `GenerateProjectFiles.bat`
- Open the generated `IronMogFF7.sln`
- Build

# Contributors
- [Jordan Marczak](https://www.jordanmarczak.com/) - _Logo_
- [Saturn Sounds](https://linktr.ee/saturn.sounds) - _Music curation_
- [Zheal](https://www.twitch.tv/zheal) - _Consulting and testing_

# Resources
The following resources were referenced during development:
- [FF7 Memory Values Spreadsheet](https://docs.google.com/spreadsheets/d/13wBwF1yRejK7o9IJSIVNzrzTeK5qHFpGnqi7NgB6QQQ/edit?gid=416136141#gid=416136141)
- [ff7-lib.rs](https://github.com/maciej-trebacz/ff7-lib.rs)
- [FF7 - Final Fantasy Inside](https://wiki.ffrtt.ru/index.php/FF7)
- [Qhimm.com Forums](https://forums.qhimm.com/)
