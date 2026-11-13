import re
from collections import OrderedDict

# The point of this script is to take a GameDataGenerated.cpp from vanilla game and patch
# it with the differences found comparing to game data generated from each CSR edition.
# These changes are appended to the end and guarded by checks based on the game version
# and which disc the player is on. This is necessary because CSR patches each disc
# differently unlike the original game. Single disc editions (CSR+, Highwind) are only
# guarded by game version.

FIELD_CALL = re.compile(r'^(addField\w*|clearField)\((0x[0-9A-Fa-f]+)\b')
CALL = re.compile(r'^(\w+)\(')

# Splits the body of a generated loadGameData into field data, keyed by field ID, and
# all other data, keyed by the function that adds it.
def parse_game_data(file_path):
    fields = OrderedDict()
    other = OrderedDict()

    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.read().splitlines()

    # Only the statements inside the function body.
    start = next(i for i, line in enumerate(lines) if line.strip() == "{")
    for line in lines[start + 1:]:
        line = line.strip()
        if not line or line.startswith("//") or line == "}":
            continue

        match = FIELD_CALL.match(line)
        if match:
            fields.setdefault(match.group(2), []).append(line)
            continue

        match = CALL.match(line)
        if not match:
            raise SystemExit("Unrecognized line in %s: %s" % (file_path, line))
        other.setdefault(match.group(1), []).append(line)

    return fields, other

def generate_cpp_patches(original_path, editions):
    original_fields, original_other = parse_game_data(original_path)
    output_cpp = []

    for enum_name, single_disc, disc_files in editions:
        for disc_num, disc_path in sorted(disc_files.items()):
            disc_fields, disc_other = parse_game_data(disc_path)

            # Only field data is merged. Anything else that differs would be silently
            # wrong in game, so refuse to continue until it's handled.
            for call in sorted(set(original_other) | set(disc_other)):
                if original_other.get(call) != disc_other.get(call):
                    raise SystemExit("%s disc %d: %s data differs from vanilla and can't be merged (%s)" % (enum_name, disc_num, call, disc_path))

            # Identify which fields are different from original
            modified_fields = []
            for fid, lines in disc_fields.items():
                if original_fields.get(fid) != lines:
                    modified_fields.append((fid, lines))

            removed_fields = [fid for fid in original_fields if fid not in disc_fields]
            if removed_fields:
                print("  %s disc %d: fields missing from the patched disc are kept from vanilla: %s" % (enum_name, disc_num, ", ".join(removed_fields)))

            print("  %s disc %d: %d modified fields" % (enum_name, disc_num, len(modified_fields)))
            if not modified_fields:
                continue

            if single_disc:
                output_cpp.append(f"    if (gameVersion == GameVersion::{enum_name})")
            else:
                output_cpp.append(f"    if (gameVersion == GameVersion::{enum_name} && gameDisc == {disc_num})")
            output_cpp.append("    {")
            for fid, lines in modified_fields:
                output_cpp.append(f"        clearField({fid});")
                for line in lines:
                    output_cpp.append(f"        {line}")
            output_cpp.append("    }\n")

    return "\n".join(output_cpp)

# 'editions' is a list of (GameVersion enum name, single disc, {disc number: generated .cpp path})
def patch_game_data(original_path, editions, output_path):
    patch_code = generate_cpp_patches(original_path, editions)

    # Load the original file to patch
    with open(original_path, 'r') as f:
        content = f.read()

    # Replace the closing brace with the patches + a new closing brace
    final_content = content.strip().rstrip('}') + "\n" + patch_code + "}"

    with open(output_path, 'w') as f:
        f.write(final_content)

if __name__ == "__main__":
    editions = [("PlayStationUS_CSR_0_13_0", False, {1: "GameDataGeneratedCSR1.cpp", 2: "GameDataGeneratedCSR2.cpp", 3: "GameDataGeneratedCSR3.cpp"})]
    patch_game_data("GameDataGenerated.cpp", editions, "GameDataGenerated_Patched.cpp")
