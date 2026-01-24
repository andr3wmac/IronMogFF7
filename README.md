<p align="center">
  <img src="resources/logo.png?raw=true" alt="IronMog FF7 Logo" title="IronMog FF7 Logo">
</p>

Drawing inspiration from IronMon and IronMario 64, IronMog FF7 is a specialized mod for the PlayStation 1 version of Final Fantasy VII thats been designed for use with emulators. It combines a randomizer with specific gameplay restrictions to provide a challenging experience.

The mod is currently in beta, and the official "IronMog" challenge rules are still being developed. Until the community reaches a consensus on a standard ruleset, the mod serves as a flexible randomizer for players looking to create their own custom challenges.

# Features
The mod allows you to enforce restrictions such as no limit breaks, no summons, no escapes, etc. It can also randomize boss attributes, encounters, items, materia, shops, and more. 

For a comprehensive breakdown of all available options, please visit the [Features wiki page](https://github.com/andr3wmac/IronMogFF7/wiki/Features).

# Requirements
- Windows
- PS1 Emulator (DuckStation or BizHawk recommended)
- Final Fantasy VII US NTSC (SCUS-94163)
  
To ensure the best experience with the mod, please use your emulator's default settings. We specifically recommend avoiding features like Runahead, as they may conflict with the mod's internal logic.

Fast Forward and Rewind are also known to cause issues and should be avoided, but Save States are fully supported and do not interfere with the mod’s functions.

# How to Play
- **Download**: Get the latest version of IronMog FF7 from the [Releases](https://github.com/andr3wmac/IronMogFF7/releases) page.
- **Launch Game**: Open your emulator (DuckStation or BizHawk) and load Final Fantasy VII.
- **Prepare**: Wait until the game reaches the Main Menu.
- **Run Mod**: Open IronMogFF7.exe.
- **Connect**: Select your emulator type, adjust your settings, and click Connect.
- **Start**: Once the status light turns green, you are ready to start a New Game.

To use an emulator other than DuckStation or BizHawk you must determine the memory address for the PS1 RAM and then select Custom under emulator and enter the address, then connect.

# Music Randomization
To provide music randomization, the mod mutes the original in-game music and plays a random MP3 from a designated folder mapped to that specific game track.

Please note: Due to copyright restrictions, the MP3 files are not included in the repository or release files. Instead, we provide a Python script that automatically downloads and processes tracks from Final Fantasy VI, VII, VIII, and IX to build the curated music folder.

You can find the instructions for this script [here](https://github.com/andr3wmac/IronMogFF7/tree/main/tools/music).

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

# License
This project is open-source and released under the MIT License. You are free to use, modify, and distribute the software as long as the original copyright notice and license are included. For more details, see the LICENSE file in the repository.
