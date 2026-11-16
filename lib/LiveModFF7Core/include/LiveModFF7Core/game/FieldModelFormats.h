#pragma once
#include <array>
#include <cstdint>

// In-memory formats of the PSX field model loader. See FieldModelOffsets for where they live.

// One record of the DAT model section: which model a field entity uses and how to allocate it.
struct FieldDatRecord
{
    uint8_t face, bones, parts, clips;
    uint8_t slot;     // Runtime model index assigned by the allocator, 0xFF when disabled.
    uint8_t enabled;
    uint8_t loaded;   // Set once the model's BCX has been read.
    uint8_t globalID; // Global party model ID (1-9), 0 for a model local to the field's BSX.
};

struct FieldDatSection
{
    uint16_t size, count;
    std::array<FieldDatRecord, 64> records;
};

// Runtime model record built by the allocator, 36 bytes each.
struct FieldRuntimeModel
{
    uint8_t unknown0[2];
    uint8_t bones, parts, clips;
    uint8_t unknown5[15];
    uint8_t globalID;
    uint8_t face;
    uint8_t unknown22[2];
    uint16_t partsOffset;      // Part descriptors, relative to skeleton.
    uint16_t animationsOffset; // Animation descriptors, relative to skeleton.
    uint32_t skeleton;         // Bone, part and animation metadata: bones * 4 + parts * 32 + clips * 16 bytes.
    uint32_t matrices;
};
static_assert(sizeof(FieldRuntimeModel) == 36, "Runtime field model record is 36 bytes.");

// Polygon types in the order a part stores their counts and draw packets.
enum FieldPolygonType : uint8_t
{
    QuadColorTex,
    TriColorTex,
    QuadMonoTex,
    TriMonoTex,
    TriMono,
    QuadMono,
    TriColor,
    QuadColor,
    FieldPolygonTypeCount
};

// GPU packet size of each polygon type, including its leading ordering table tag.
constexpr std::array<uint32_t, FieldPolygonTypeCount> FieldPacketBytes = {52, 40, 40, 32, 20, 24, 28, 36};

// Loaded part descriptor, 32 bytes each.
struct FieldPartDescriptor
{
    uint8_t unknown0;
    uint8_t bone;
    uint8_t vertices;
    uint8_t unknown3;
    std::array<uint8_t, FieldPolygonTypeCount> polygons; // Count of each FieldPolygonType.
    uint8_t unknown12[10];
    uint16_t packetBytes; // One draw buffer; the second immediately follows it.
    uint32_t data;        // Relocated polygon data.
    uint32_t packets;     // Double buffered draw packets, grouped by FieldPolygonType.
};
static_assert(sizeof(FieldPartDescriptor) == 32, "Field part descriptor is 32 bytes.");

// Loaded animation descriptor, 16 bytes each.
struct FieldAnimationDescriptor
{
    uint16_t frames;
    uint8_t bones;
    uint8_t animatedTranslations, staticTranslations, animatedRotations;
    uint16_t translationOffset, staticOffset, rotationOffset;
    uint32_t data; // Relocated, already scaled animation data.
};
static_assert(sizeof(FieldAnimationDescriptor) == 16, "Field animation descriptor is 16 bytes.");
