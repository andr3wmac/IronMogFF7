#include "ModelEditor.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"

#include <string>

ModelEditor::ModelEditor(GameManager* gameManager)
{
    game = gameManager;
}

bool ModelEditor::findModel(std::string modelName)
{
    uintptr_t startAddress = 1284000;
    size_t searchArea = 200000;
    size_t uintCount = searchArea / 4;

    if (searchData == nullptr)
    {
        searchData = new uint32_t[uintCount];
    }

    game->read(startAddress, searchArea, (uint8_t*)searchData);

    std::vector<uintptr_t> addresses;

    for (int i = 0; i < uintCount; ++i)
    {
        uint32_t data0 = searchData[i];
        uint8_t cmd0 = (data0 >> 24) & 0xFF;

        // polygon, gouraud, tri
        if (cmd0 == 0x30)
        {
            uint32_t word1 = searchData[i + 7];
            uint8_t cmd1 = (word1 >> 24) & 0xFF;

            if (cmd1 == 0x30 || cmd1 == 0x38)
            {
                addresses.push_back(startAddress + (i * 4));
            }
        }

        // polygon, gouraud, quad 
        if (cmd0 == 0x38)
        {
            uint32_t word1 = searchData[i + 9];
            uint8_t cmd1 = (word1 >> 24) & 0xFF;

            if (cmd1 == 0x30 || cmd1 == 0x38)
            {
                addresses.push_back(startAddress + (i * 4));
            }
        }
    }

    for (int i = 0; i < addresses.size(); ++i)
    {
        if (openModel(addresses[i], modelName))
        {
            LOG("Found model at: %d", addresses[i]);
            return true;
        }
    }

    return false;
}

bool ModelEditor::openModel(uintptr_t address, std::string modelName)
{
    if (GameData::models.count(modelName) == 0)
    {
        return false;
    }

    //readAllPolys(address, 1000);

    Model& model = GameData::models[modelName];
    parts.clear();

    uintptr_t curAddr = address;
    for (ModelPart& part : model.parts)
    {
        ModelEditorPart editorPart;

        for (int bufferIdx = 0; bufferIdx < 2; ++bufferIdx)
        {
            std::vector<ModelEditorPoly> readPolys;

            for (int i = 0; i < part.quadColorTex; ++i)
            {
                ModelEditorPoly poly;
                size_t readSize = readPoly(curAddr, poly);
                if (readSize == 0 || poly.textured == false)
                {
                    return false;
                }

                readPolys.push_back(poly);
                curAddr += readSize;
            }

            for (int i = 0; i < part.triColorTex; ++i)
            {
                ModelEditorPoly poly;
                size_t readSize = readPoly(curAddr, poly);
                if (readSize == 0 || poly.textured == false)
                {
                    return false;
                }

                readPolys.push_back(poly);
                curAddr += readSize;
            }

            for (int i = 0; i < part.triColor; ++i)
            {
                ModelEditorPoly poly;
                size_t readSize = readPoly(curAddr, poly);
                if (readSize == 0)
                {
                    return false;
                }

                readPolys.push_back(poly);
                curAddr += readSize;
            }

            for (int i = 0; i < part.quadColor; ++i)
            {
                ModelEditorPoly poly;
                size_t readSize = readPoly(curAddr, poly);
                if (readSize == 0)
                {
                    return false;
                }

                readPolys.push_back(poly);
                curAddr += readSize;
            }

            if (bufferIdx == 0)
            {
                editorPart.polys = readPolys;
            }
            else 
            {
                for (int i = 0; i < readPolys.size(); ++i)
                {
                    editorPart.polys[i].bufferAddressB = readPolys[i].bufferAddressA;
                }
            }
        }

        parts.push_back(editorPart);
    }

    return true;
}

void ModelEditor::setPartColor(int partIndex, uint8_t red, uint8_t green, uint8_t blue)
{
    if (partIndex >= parts.size())
    {
        return;
    }

    ModelEditorPart& part = parts[partIndex];
    for (int i = 0; i < part.polys.size(); ++i)
    {
        ModelEditorPoly& poly = part.polys[i];
       
        for (int v = 0; v < poly.vertices.size(); ++v)
        {
            uintptr_t colorAddrA = poly.bufferAddressA + (v * poly.stride);
            uintptr_t colorAddrB = poly.bufferAddressB + (v * poly.stride);

            game->write<uint8_t>(colorAddrA + 0, red);
            game->write<uint8_t>(colorAddrB + 0, red);
            game->write<uint8_t>(colorAddrA + 1, green);
            game->write<uint8_t>(colorAddrB + 1, green);
            game->write<uint8_t>(colorAddrA + 2, blue);
            game->write<uint8_t>(colorAddrB + 2, blue);
        }
    }
}

void ModelEditor::setPartTint(int partIndex, uint8_t red, uint8_t green, uint8_t blue)
{
    if (partIndex >= parts.size())
    {
        return;
    }

    ModelEditorPart& part = parts[partIndex];
    for (int i = 0; i < part.polys.size(); ++i)
    {
        ModelEditorPoly& poly = part.polys[i];

        for (int v = 0; v < poly.vertices.size(); ++v)
        {
            uintptr_t colorAddrA = poly.bufferAddressA + (v * poly.stride);
            uintptr_t colorAddrB = poly.bufferAddressB + (v * poly.stride);

            ModelEditorVertex tinted = tintVertexColor(poly.vertices[v], { red, green, blue });

            game->write<uint8_t>(colorAddrA + 0, tinted.r);
            game->write<uint8_t>(colorAddrB + 0, tinted.r);
            game->write<uint8_t>(colorAddrA + 1, tinted.g);
            game->write<uint8_t>(colorAddrB + 1, tinted.g);
            game->write<uint8_t>(colorAddrA + 2, tinted.b);
            game->write<uint8_t>(colorAddrB + 2, tinted.b);
        }
    }
}

ModelEditor::ModelEditorVertex ModelEditor::tintVertexColor(const ModelEditorVertex& src, const ModelEditorVertex& tint)
{
    // Convert to grayscale (perceptual luminance)
    float gray = 0.299f * src.r + 0.587f * src.g + 0.114f * src.b;

    // Multiply by tint color (normalized 0–1)
    float rf = gray * (tint.r / 255.0f);
    float gf = gray * (tint.g / 255.0f);
    float bf = gray * (tint.b / 255.0f);

    // Clamp and convert back to 8-bit
    auto clamp8 = [](float v) -> uint8_t {
        return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
        };

    return { clamp8(rf), clamp8(gf), clamp8(bf) };
}

size_t ModelEditor::readPoly(uintptr_t address, ModelEditorPoly& polyOut)
{
    polyOut.bufferAddressA = address;

    uintptr_t curAddr = address;
    uintptr_t startAddr = curAddr;

    uint32_t word0 = game->read<uint32_t>(curAddr);

    ModelEditorVertex vert0;
    vert0.r = word0 & 0xFF;              // bits 7..0
    vert0.g = (word0 >> 8) & 0xFF;       // bits 15..8
    vert0.b = (word0 >> 16) & 0xFF;      // bits 23..16
    uint8_t cmd  = (word0 >> 24) & 0xFF; // bits 31..24

    uint8_t polygon_render  = (cmd >> 5) & 0b111; // bits 31-29
    uint8_t gouraud_flag    = (cmd >> 4) & 0b1;   // bit 28
    uint8_t quad_flag       = (cmd >> 3) & 0b1;   // bit 27
    uint8_t textured_flag   = (cmd >> 2) & 0b1;   // bit 26
    uint8_t semi_transp     = (cmd >> 1) & 0b1;   // bit 25
    uint8_t raw_tex_mod     = cmd & 0b1;          // bit 24

    if (polygon_render != 1)
    {
        //LOG("Unknown command. Skipping...");
        return 0;
    }

    if (gouraud_flag != 1)
    {
        //LOG("Unknown primitive. Skipping...");
        return 0;
    }

    polyOut.textured = textured_flag == 1;
    polyOut.stride = 8 + (textured_flag * 4);

    // Vert0 XXXXYYYY
    curAddr += 8;
    if (textured_flag == 1)
    {
        // Vert0 UV
        curAddr += 4;
    }
    polyOut.vertices.push_back(vert0);

    // Vert1 RRGGBB00, XXXXYYYY
    uint32_t word1 = game->read<uint32_t>(curAddr);
    ModelEditorVertex vert1;
    vert1.r = word1 & 0xFF;
    vert1.g = (word1 >> 8) & 0xFF;
    vert1.b = (word1 >> 16) & 0xFF;
    curAddr += 8;
    if (textured_flag == 1)
    {
        // Vert0 ClutVVUU
        curAddr += 4;
    }
    polyOut.vertices.push_back(vert1);

    // Vert2 RRGGBB00, XXXXYYYY
    uint32_t word2 = game->read<uint32_t>(curAddr);
    ModelEditorVertex vert2;
    vert2.r = word2 & 0xFF;
    vert2.g = (word2 >> 8) & 0xFF;
    vert2.b = (word2 >> 16) & 0xFF;
    curAddr += 8;
    if (textured_flag == 1)
    {
        // Vert2 ClutVVUU
        curAddr += 4;
    }
    polyOut.vertices.push_back(vert2);

    if (quad_flag == 1)
    {
        // Vert3 RRGGBB00, XXXXYYYY
        uint32_t word3 = game->read<uint32_t>(curAddr);
        ModelEditorVertex vert3;
        vert3.r = word3 & 0xFF;
        vert3.g = (word3 >> 8) & 0xFF;
        vert3.b = (word3 >> 16) & 0xFF;
        curAddr += 8;
        if (textured_flag == 1)
        {
            // Vert3 ClutVVUU
            curAddr += 4;
        }
        polyOut.vertices.push_back(vert3);
    }

    // Not sure what the extra 4 bytes is on the end of every command buffer entry
    curAddr += 4;

    return curAddr - address;
}

void ModelEditor::readAllPolys(uintptr_t address, int maxReadPolys)
{
    uintptr_t curAddr = address;

    for (int i = 0; i < maxReadPolys; ++i)
    {
        ModelEditorPoly poly;
        size_t readSize = readPoly(curAddr, poly);

        if (readSize == 0)
        {
            return;
        }

        if (poly.vertices.size() == 3)
        {
            ModelEditorVertex& v0 = poly.vertices[0];
            ModelEditorVertex& v1 = poly.vertices[1];
            ModelEditorVertex& v2 = poly.vertices[2];

            if (poly.textured)
            {
                LOG("Tri Tex %d: %d %d %d, %d %d %d, %d %d %d", curAddr, v0.r, v0.g, v0.b, v1.r, v1.g, v1.b, v2.r, v2.g, v2.b);
            }
            else 
            {
                LOG("Tri %d: %d %d %d, %d %d %d, %d %d %d", curAddr, v0.r, v0.g, v0.b, v1.r, v1.g, v1.b, v2.r, v2.g, v2.b);
            }
        }

        if (poly.vertices.size() == 4)
        {
            ModelEditorVertex& v0 = poly.vertices[0];
            ModelEditorVertex& v1 = poly.vertices[1];
            ModelEditorVertex& v2 = poly.vertices[2];
            ModelEditorVertex& v3 = poly.vertices[3];

            if (poly.textured)
            {
                LOG("Quad Tex %d: %d %d %d, %d %d %d, %d %d %d, %d %d %d", curAddr, v0.r, v0.g, v0.b, v1.r, v1.g, v1.b, v2.r, v2.g, v2.b, v3.r, v3.g, v3.b);
            }
            else
            {
                LOG("Quad %d: %d %d %d, %d %d %d, %d %d %d, %d %d %d", curAddr, v0.r, v0.g, v0.b, v1.r, v1.g, v1.b, v2.r, v2.g, v2.b, v3.r, v3.g, v3.b);
            }
        }

        curAddr += readSize;
    }
}