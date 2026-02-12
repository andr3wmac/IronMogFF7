import re
from collections import defaultdict

# The point of this script is to take a GameDataGenerated.cpp from vanilla game and patch
# it with the differences found comparing to CSR generated game data. These changes are
# appended to the end and guarded by checks based on which disc the player is on. This is 
# necessary because CSR patches the discs differently unlike the original game.

def parse_fields(file_path):
    fields = defaultdict(list)
    current_field_id = None
    
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            line = line.strip()
            if not line: continue
            
            # Match addField(0x123, "name"); etc
            match = re.search(r'addField\((0x[0-9A-Fa-f]+),', line)
            if match:
                current_field_id = match.group(1)
                fields[current_field_id].append(line)
            elif current_field_id:
                fields[current_field_id].append(line)
    return fields

def generate_cpp_patches(original_path, disc_files):
    original_data = parse_fields(original_path)
    output_cpp = []

    for disc_num, disc_path in enumerate(disc_files, 1):
        disc_data = parse_fields(disc_path)
        modified_fields = []

        # Identify which fields are different from original
        for fid, lines in disc_data.items():
            if fid not in original_data or original_data[fid] != lines:
                modified_fields.append((fid, lines))

        if modified_fields:
            output_cpp.append(f"    if (gameVersion == GameVersion::PlayStationUS_CSR && gameDisc == {disc_num})")
            output_cpp.append("    {")
            for fid, lines in modified_fields:
                output_cpp.append(f"        clearField({fid});")
                for line in lines:
                    output_cpp.append(f"        {line}")
            output_cpp.append("    }\n")

    return "\n".join(output_cpp)

# Run the generator
disc_paths = ["GameDataGeneratedCSR1.cpp", "GameDataGeneratedCSR2.cpp", "GameDataGeneratedCSR3.cpp"]
patch_code = generate_cpp_patches("GameDataGenerated.cpp", disc_paths)

# Load the original file to patch
with open("GameDataGenerated.cpp", 'r') as f:
    content = f.read()

# Replace the closing brace with the patches + a new closing brace
final_content = content.strip().rstrip('}') + "\n" + patch_code + "}"

with open("GameDataGenerated_Patched.cpp", 'w') as f:
    f.write(final_content)