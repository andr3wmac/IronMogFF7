# IronMog FF7 Music Builder

This utility generates the curated music folders for the IronMog FF7 mod using tracks from *Final Fantasy VI, VII, VIII, IX, and X*. `build.py` downloads the source archives, extracts and renames the MP3s using their embedded titles, normalizes their volume, detects loop points, and copies the selected tracks into the mod's music folders.

---

## Prerequisites

Before running the script, ensure you have the following installed:

* **Python 3.10 or higher:** [Download Python](https://www.python.org/downloads/). The dependencies are not version-pinned; their supported Python versions may change.
* **FFmpeg and FFprobe:** Required for audio processing. Add them to **PATH**, or place `ffmpeg.exe` and `ffprobe.exe` next to `process_music.py` and `build.py`.
* **Internet access:** Required to download any source archives that are not already in `workspace`.

## Setup

1. **Clone or Download** this repository to your local machine.
2. **Open a Terminal** (Command Prompt or PowerShell) in `tools/music`, the folder containing `build.py`. Keep this as your working directory for every build command below.
3. **Install Dependencies** by running:
   ```sh
   python -m pip install -r requirements.txt
   ```
4. **Create the workspace folder** if it does not already exist:

   ```sh
   mkdir workspace
   ```

   The downloader writes archives into this folder but does not create it.

## Usage

All build stages are currently enabled by the Boolean settings near the top of `build.py`. After completing setup, run:

```sh
python build.py
```

**A rebuild deletes the existing `music` folder in your working directory.** It also replaces the extracted source folders in `workspace`. Back up any custom files there before rebuilding. Running `build.py` from another directory can delete that directory's `music` folder and then fail because the other paths are relative to the working directory.

Existing ZIP archives are reused without downloading them again. The other enabled stages still run: source MP3s are extracted into `workspace/FF6`, `FF7`, `FF8`, `FF9`, and `FFX`; normalized MP3s are written into the corresponding `*_Normalized` folders; and detected loop points are saved as adjacent `.cfg` files. Normalization uses the `ffmpeg-normalize` `podcast` preset and encodes MP3s at 320 kbps.

The final `music` folder contains the selected tracks in the folders listed in `populate_music`, using the source archive's embedded track titles with a game prefix. Some folders are intentionally empty. Once the build finishes successfully, copy the generated `music` folder into your IronMog FF7 folder.

For each selected track, its `.cfg` file is copied from the first existing location in this order:

1. `configs/<music folder>/<track name>.cfg`
2. `configs/<track name>.cfg`
3. The `.cfg` beside the source MP3, normally in `workspace/<game>_Normalized`

Loop detection does not guarantee a `.cfg` for every track. Processing failures can also leave missing output even if the script prints `Done.`; check the terminal for errors or warnings.

### Reusing processed sources

To rebuild only the curated folders from an already prepared workspace, set `download_music`, `unzip_sources`, `delete_unwanted`, `rename_files`, `normalize_volume`, and `find_loops` to `False`, and leave `create_folders` and `populate_music` set to `True`.

Population uses a game's `*_Normalized` folder whenever that folder exists; otherwise it uses the renamed source folder. An incomplete normalized folder therefore causes missing-file errors rather than falling back to individual unnormalized tracks. Loop detection in `build.py` only scans the normalized folders.

### Adding Additional Songs

To normalize and detect loop points for a single MP3 or the MP3s directly inside a folder, run:

```sh
python process_music.py "path/to/song.mp3"
python process_music.py "path/to/folder"
```

Folder processing does not search subfolders. By default, normalization overwrites the input MP3s, and successful loop detection writes or overwrites adjacent `.cfg` files. Back up your files first.

Available options:

* `--no-normalize`: Only detect loop points; leave the MP3 audio unchanged.
* `--no-loops`: Only normalize volume.
* `--bitrate 192k`: Change the normalized MP3 bitrate from the default `320k`.

This command processes files where they are; it does not add them to the curated build arrays. To retain additional songs across rebuilds, keep their source files outside the generated `music` folder and copy the processed MP3s and `.cfg` files into the desired mod music folder after building.
