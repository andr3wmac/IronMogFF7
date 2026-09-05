import argparse
import glob
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# If ffmpeg.exe (and ffprobe.exe) are next to this script, make them discoverable by 
# ffmpeg-normalize and pymusiclooper no matter which directory
_SCRIPT_DIR = Path(__file__).resolve().parent
if (_SCRIPT_DIR / "ffmpeg.exe").exists():
    os.environ["PATH"] = str(_SCRIPT_DIR) + os.pathsep + os.environ.get("PATH", "")
    # ffmpeg-normalize honors FFMPEG_PATH to locate the ffmpeg binary directly.
    os.environ.setdefault("FFMPEG_PATH", str(_SCRIPT_DIR / "ffmpeg.exe"))

def normalize_files(files, output_dir, bitrate="320k"):
    files = list(files)
    if not files:
        return

    os.makedirs(output_dir, exist_ok=True)

    command = [
        sys.executable, "-m", "ffmpeg_normalize",
        *files,
        "--preset", "podcast",
        "-c:a", "libmp3lame",
        "-b:a", bitrate,
        "-of", output_dir,
        "-ext", "mp3",
        "-f",
    ]

    subprocess.run(command)

def normalize_in_place(files, bitrate="320k"):
    files = list(files)
    if not files:
        return

    with tempfile.TemporaryDirectory() as tmp:
        normalize_files(files, tmp, bitrate=bitrate)
        for file in files:
            result = os.path.join(tmp, os.path.basename(file))
            if os.path.exists(result):
                shutil.move(result, file)
            else:
                print(f"Warning: no normalized output produced for {file}")

def batch_normalize_folder(input_dir, output_dir):
    normalize_files(glob.glob(f"{input_dir}/*.mp3"), output_dir)

def get_loop_points(file_path):
    cmd = [sys.executable, "-m", "pymusiclooper", "export-points", "--path", file_path]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        output = result.stdout

        loop_start = None
        loop_end = None

        for line in output.splitlines():
            if "LOOP_START:" in line:
                loop_start = int(line.split(":")[1].strip())
            elif "LOOP_END:" in line:
                loop_end = int(line.split(":")[1].strip())

        return loop_start, loop_end

    except subprocess.CalledProcessError as e:
        print(f"Error processing {file_path}: {e}")
        return None, None

def find_loops(files):
    for file in files:
        loop_start, loop_end = get_loop_points(file)

        if loop_start and loop_end:
            print(str(file) + ": " + str(loop_start) + " - " + str(loop_end))
            cfg_path = file.replace(".mp3", ".cfg")

            with open(cfg_path, "w") as f:
                f.write("LoopStart=" + str(loop_start) + "\n")
                f.write("LoopEnd=" + str(loop_end) + "\n")

def batch_loop_folder(input_dir):
    find_loops(glob.glob(f"{input_dir}/*.mp3"))

def resolve_targets(path):
    if os.path.isdir(path):
        return sorted(glob.glob(os.path.join(path, "*.mp3")))
    if os.path.isfile(path) and path.lower().endswith(".mp3"):
        return [path]
    raise SystemExit(f"Not an .mp3 file or a folder of mp3s: {path}")

def main():
    parser = argparse.ArgumentParser(description="Volume balance and/or loop-detect a single mp3 or a folder of mp3s, in place.")
    parser.add_argument("path", help="An .mp3 file or a folder containing .mp3 files")
    parser.add_argument("--no-normalize", dest="normalize", action="store_false", help="Skip volume balancing")
    parser.add_argument("--no-loops", dest="loops", action="store_false", help="Skip loop detection")
    parser.add_argument("--bitrate", default="320k", help="mp3 bitrate for normalized output (default: 320k)")
    parser.set_defaults(normalize=True, loops=True)
    args = parser.parse_args()

    files = resolve_targets(args.path)
    if not files:
        raise SystemExit(f"No .mp3 files found in: {args.path}")

    if args.normalize:
        print(f"Normalizing {len(files)} file(s) in place..")
        normalize_in_place(files, bitrate=args.bitrate)

    if args.loops:
        print(f"Finding loops for {len(files)} file(s)..")
        find_loops(files)

    print("Done.")

if __name__ == "__main__":
    main()
