# IronMog FF7 Music Builder

This utility allows you to generate the curated music for the IronMog FF7 mod. Since the mod does not include copyrighted music files, this script automates the process of downloading, normalizing, and organizing tracks from *Final Fantasy VI, VII, VIII, and IX*.

---

## Prerequisites

Before running the script, ensure you have the following installed:

* **Python 3.9 or higher:** [Download Python](https://www.python.org/downloads/)
* **FFmpeg:** Required for the `ffmpeg-normalize` dependency. 
    * *Note:* Ensure FFmpeg is added to your system's **PATH** so the script can access it.

## Installation

1. **Clone or Download** this repository to your local machine.
2. **Open a Terminal** (Command Prompt or PowerShell) in the script's folder.
3. **Install Dependencies** by running:
  ```bash
  pip install -r requirements.txt
  ```

## Usage

The process is fully automated. To build your music library, simply run the build script:
  ```bash
python build.py
  ```
When its finished you can copy the 'music' folder into your IronMog FF7 folder.
