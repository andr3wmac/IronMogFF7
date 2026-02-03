#include "ModelEditor.h"
#include "core/game/GameData.h"
#include "core/game/MemoryOffsets.h"
#include "core/utilities/Logging.h"

#include <string>

ModelEditor::ModelEditor()
{
    game            = nullptr;
    bufferAddress   = 0;
    bufferSize      = 0;
}

ModelEditor::~ModelEditor()
{
    if (buffer != nullptr)
    {
        delete[] buffer;
    }
}

void ModelEditor::setup(GameManager* gameManager)
{
    game = gameManager;
    bufferSize = 500000;
    openModels.clear();
}

void ModelEditor::findFieldModels()
{
    openModels.clear();

    bufferAddress = 1284000;
    size_t uintCount = bufferSize / 4;
    if (buffer == nullptr)
    {
        buffer = new uint32_t[uintCount];
    }

    game->read(bufferAddress, bufferSize, (uint8_t*)buffer);

    int polygonCount = 0;
    int startPolyIndex = 0;

    for (int i = 0; i < uintCount;)
    {
        uint32_t data0 = buffer[i];
        uint8_t cmd0 = (data0 >> 24) & 0xFF;

        // polygon, gouraud, quad, textured
        if (cmd0 == 0x3C)
        {
            if (polygonCount == 0) startPolyIndex = i;
            i += 13;
            polygonCount++;
            continue;
        }

        // polygon, gouraud, tri, textured
        if (cmd0 == 0x34)
        {
            if (polygonCount == 0) startPolyIndex = i;
            i += 10;
            polygonCount++;
            continue;
        }

        // polygon, gouraud, tri
        if (cmd0 == 0x30)
        {
            if (polygonCount == 0) startPolyIndex = i;
            i += 7;
            polygonCount++;
            continue;
        }

        // polygon, gouraud, quad 
        if (cmd0 == 0x38)
        {
            if (polygonCount == 0) startPolyIndex = i;
            i += 9;
            polygonCount++;
            continue;
        }

        if (polygonCount > 100)
        {
            for (Model& model : GameData::models)
            {
                if ((model.polyCount * 2) == polygonCount)
                {
                    if (openFieldModel(startPolyIndex, model))
                    {
                        DEBUG_LOG("Opened model: %s %d %d", model.name.c_str(), bufferAddress + (startPolyIndex * 4), polygonCount);
                        break;
                    }
                    else 
                    {
                        DEBUG_LOG("Failed to open model: %s %d %d", model.name.c_str(), bufferAddress + (startPolyIndex * 4), polygonCount);
                    }
                }

                // This can occur due to false positives in the initial search where theres an extra poly
                // on the front or back of the buffer due to bad luck
                if ((model.polyCount * 2) == polygonCount - 1)
                {
                    // First we see if its one on the end of the buffer
                    if (openFieldModel(startPolyIndex, model))
                    {
                        DEBUG_LOG("Opened model: %s %d %d", model.name.c_str(), bufferAddress + (startPolyIndex * 4), polygonCount - 1);
                        break;
                    }
                    else
                    {
                        // Assume its one on the front of the buffer and skip past it
                        ModelEditorPoly poly;
                        int readSize = readPoly(startPolyIndex, poly);

                        if (openFieldModel(startPolyIndex + readSize, model))
                        {
                            DEBUG_LOG("Opened model: %s %d %d", model.name.c_str(), bufferAddress + (startPolyIndex * 4), polygonCount - 1);
                            break;
                        }
                        else
                        {
                            DEBUG_LOG("Failed to open model: %s %d %d", model.name.c_str(), bufferAddress + (startPolyIndex * 4), polygonCount);
                        }
                    }
                }
            }
        }

        if (polygonCount > 0 && polygonCount < 4)
        {
            // This prevents making big jumps from false positives.
            i = startPolyIndex;
        }

        polygonCount = 0;
        i++;
    }
}

void ModelEditor::openBattleModels()
{
    openModels.clear();

    bufferAddress = BattleOffsets::AllyModels[0];
    //bufferSize = 184320; // Each ally model gets 61440 bytes
    size_t uintCount = bufferSize / 4;
    if (buffer == nullptr)
    {
        buffer = new uint32_t[uintCount];
    }

    game->read(bufferAddress, bufferSize, (uint8_t*)buffer);
    int bufferIdx = 0;

    // Special handling of HICLOUD in the final fight
    uint16_t fieldID = game->getFieldID();
    if (fieldID == 763)
    {
        BattleModel* model = GameData::getBattleModel("HICLOUD");
        if (model != nullptr)
        {
            if (openBattleModel(model->headerSizes[0] / 4, *model))
            {
                DEBUG_LOG("Opened battle model: %s %d", model->name.c_str(), bufferAddress + (bufferIdx * 4));
            }
            else
            {
                DEBUG_LOG("Failed to open battle model: %s", model->name.c_str());
            }
        }
    }

    std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
    for (int i = 0; i < partyIDs.size(); ++i)
    {
        uint8_t& id = partyIDs[i];
        if (id == 0xFF)
        {
            continue;
        }

        BattleModel* model = GameData::getBattleModel(getCharacterName(id));
        if (model == nullptr)
        {
            continue;
        }

        bool openedModel = false;
        bufferIdx = (int)(BattleOffsets::AllyModels[i] - BattleOffsets::AllyModels[0]) / 4;

        for (int headerSize : model->headerSizes)
        {
            int tmpBufferIdx = bufferIdx + (headerSize / 4);
            if (openBattleModel(tmpBufferIdx, *model))
            {
                bufferIdx = tmpBufferIdx;
                DEBUG_LOG("Opened battle model: %s %d", model->name.c_str(), bufferAddress + (bufferIdx * 4));
                openedModel = true;
                break;
            }
        }

        if (!openedModel)
        {
            DEBUG_LOG("Failed to open battle model: %s", model->name.c_str());
        }
    }
}

bool ModelEditor::areBattleModelsLoaded()
{
    // We want a cheap method to ensure the battle model data is loaded
    // before we try to open the models. Our approach is to verify the 
    // vertex count is what and where it should be for each party model.

    // Special handling of HICLOUD in the final fight
    uint16_t fieldID = game->getFieldID();
    if (fieldID == 763)
    {
        BattleModel* model = GameData::getBattleModel("HICLOUD");
        if (model != nullptr)
        {
            // Check the last parts vertex count so we can ensure the full model is loaded.
            uintptr_t lastPartStartAddress = BattleOffsets::AllyModels[0] + model->headerSizes[0];
            for (int i = 0; i < model->parts.size() - 1; ++i)
            {
                lastPartStartAddress += model->parts[i].sizeInBytes;
            }
            const BattleModelPart& lastPart = model->parts[model->parts.size() - 1];

            uint32_t vertexCountData = game->read<uint32_t>(lastPartStartAddress);
            uint16_t vertexCount = (vertexCountData & 0xFFFF) / 8;
            if (vertexCount == lastPart.vertexCount)
            {
                // We return true here because if this is the final fight HICLOUD 
                // will be the only ally model loaded.
                return true;
            }
        }
    }

    std::array<uint8_t, 3> partyIDs = game->getPartyIDs();
    for (int i = 0; i < partyIDs.size(); ++i)
    {
        uint8_t& id = partyIDs[i];
        if (id == 0xFF)
        {
            continue;
        }

        std::string modelName = getCharacterName(id);
        if (modelName == "SEPHIROTH")
        {
            // In Kalm flashback Sephiroth is in our party. No color changes
            // for him so we just skip evaluating this one.
            continue;
        }

        BattleModel* model = GameData::getBattleModel(modelName);
        if (model == nullptr)
        {
            return false;
        }

        bool foundValidLastPart = false;

        for (int headerSize : model->headerSizes)
        {
            // Check the last parts vertex count so we can ensure the full model is loaded.
            uintptr_t lastPartStartAddress = BattleOffsets::AllyModels[i] + headerSize;
            for (int j = 0; j < model->parts.size() - 1; ++j)
            {
                lastPartStartAddress += model->parts[j].sizeInBytes;
            }
            const BattleModelPart& lastPart = model->parts[model->parts.size() - 1];

            uint32_t vertexCountData = game->read<uint32_t>(lastPartStartAddress);
            uint16_t vertexCount = (vertexCountData & 0xFFFF) / 8;
            if (vertexCount == lastPart.vertexCount)
            {
                foundValidLastPart = true;
                break;
            }
        }

        if (!foundValidLastPart)
        {
            return false;
        }
    }

    return true;
}

std::vector<ModelEditor::ModelEditorModel>& ModelEditor::getOpenModels()
{
    return openModels;
}

bool ModelEditor::openFieldModel(int bufferIdx, const Model& model)
{
    std::vector<ModelEditorPart> pendingParts;

    int curIdx = bufferIdx;
    for (const ModelPart& part : model.parts)
    {
        ModelEditorPart editorPart;

        // Model parts are double buffered
        for (int bufferCopy = 0; bufferCopy < 2; ++bufferCopy)
        {
            std::vector<ModelEditorPoly> readPolys;

            for (int i = 0; i < part.quadColorTex; ++i)
            {
                ModelEditorPoly poly;
                int readSize = readPoly(curIdx, poly);
                if (readSize == 0 || poly.textured == false)
                {
                    return false;
                }

                readPolys.push_back(poly);
                curIdx += readSize;
            }

            for (int i = 0; i < part.triColorTex; ++i)
            {
                ModelEditorPoly poly;
                int readSize = readPoly(curIdx, poly);
                if (readSize == 0 || poly.textured == false)
                {
                    return false;
                }

                readPolys.push_back(poly);
                curIdx += readSize;
            }

            for (int i = 0; i < part.triColor; ++i)
            {
                ModelEditorPoly poly;
                int readSize = readPoly(curIdx, poly);
                if (readSize == 0)
                {
                    return false;
                }

                readPolys.push_back(poly);
                curIdx += readSize;
            }

            for (int i = 0; i < part.quadColor; ++i)
            {
                ModelEditorPoly poly;
                int readSize = readPoly(curIdx, poly);
                if (readSize == 0)
                {
                    return false;
                }


                readPolys.push_back(poly);
                curIdx += readSize;
            }

            if (bufferCopy == 0)
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

        pendingParts.push_back(editorPart);
    }

    ModelEditorModel& editorModel = openModels.emplace_back();
    editorModel.modelName = model.name;
    editorModel.parts.insert(editorModel.parts.end(), pendingParts.begin(), pendingParts.end());

    return true;
}

bool ModelEditor::openBattleModel(int bufferIdx, const BattleModel& model)
{
    std::vector<ModelEditorPart> pendingParts;
    for (const BattleModelPart& battlePart : model.parts)
    {
        ModelEditorPart editorPart;

        uint16_t vertexCount = (buffer[bufferIdx] & 0xFFFF) / 8;
        if (vertexCount != battlePart.vertexCount)
        {
            return false;
        }
        int vertexDataSize = 4 + (battlePart.vertexCount * 8);
        bufferIdx += (vertexDataSize / 4);

        // Tri Mono Tex
        uint16_t triMonoTex = (buffer[bufferIdx] & 0xFFFF);
        if (triMonoTex != battlePart.triMonoTex)
        {
            return false;
        }
        bufferIdx += 1;
        for (int i = 0; i < battlePart.triMonoTex; ++i)
        {
            ModelEditorPoly poly;
            poly.bufferAddressA = bufferAddress + (bufferIdx * 4);
            poly.bufferAddressB = poly.bufferAddressA;
            poly.textured = true;
            editorPart.polys.push_back(poly);
            bufferIdx += 4; // 16 bytes
        }

        // Quad Mono Tex
        uint16_t quadMonoTex = (buffer[bufferIdx] & 0xFFFF);
        if (quadMonoTex != battlePart.quadMonoTex)
        {
            return false;
        }
        bufferIdx += 1;
        for (int i = 0; i < battlePart.quadMonoTex; ++i)
        {
            ModelEditorPoly poly;
            poly.bufferAddressA = bufferAddress + (bufferIdx * 4);
            poly.bufferAddressB = poly.bufferAddressA;
            poly.textured = true;
            editorPart.polys.push_back(poly);
            bufferIdx += 5; // 20 bytes
        }

        // Tri Color
        uint16_t triColor = (buffer[bufferIdx] & 0xFFFF);
        if (triColor != battlePart.triColor)
        {
            return false;
        }
        bufferIdx += 1;
        for (int i = 0; i < battlePart.triColor; ++i)
        {
            ModelEditorPoly poly;
            poly.bufferAddressA = bufferAddress + (bufferIdx * 4) + (2 * 4);
            poly.bufferAddressB = poly.bufferAddressA;
            poly.textured = true;
            poly.vertexStride = 4;

            for (int j = 0; j < 3; ++j)
            {
                uint32_t data = buffer[bufferIdx + 2 + j];

                Utilities::Color color;
                color.r = data & 0xFF;
                color.g = (data >> 8) & 0xFF;
                color.b = (data >> 16) & 0xFF;
                poly.vertexColors.push_back(color);
            }

            editorPart.polys.push_back(poly);
            bufferIdx += 5; // 20 bytes
        }

        // Quad Color
        uint16_t quadColor = (buffer[bufferIdx] & 0xFFFF);
        if (quadColor != battlePart.quadColor)
        {
            return false;
        }
        bufferIdx += 1;
        for (int i = 0; i < battlePart.quadColor; ++i)
        {
            ModelEditorPoly poly;
            poly.bufferAddressA = bufferAddress + (bufferIdx * 4) + (2 * 4);
            poly.bufferAddressB = poly.bufferAddressA;
            poly.textured = true;
            poly.vertexStride = 4;

            for (int j = 0; j < 4; ++j)
            {
                uint32_t data = buffer[bufferIdx + 2 + j];

                Utilities::Color color;
                color.r = data & 0xFF;
                color.g = (data >> 8) & 0xFF;
                color.b = (data >> 16) & 0xFF;
                poly.vertexColors.push_back(color);
            }

            editorPart.polys.push_back(poly);
            bufferIdx += 6; // 24 bytes
        }

        pendingParts.push_back(editorPart);
    }

    ModelEditorModel& editorModel = openModels.emplace_back();
    editorModel.modelName = model.name;
    editorModel.parts.insert(editorModel.parts.end(), pendingParts.begin(), pendingParts.end());
    return true;
}

void ModelEditor::setPartColor(int modelIndex, int partIndex, Utilities::Color color, const std::set<int>& excludedPolys)
{
    if (game == nullptr || modelIndex >= openModels.size())
    {
        return;
    }

    ModelEditorModel& editorModel = openModels[modelIndex];

    if (partIndex >= editorModel.parts.size())
    {
        return;
    }

    ModelEditorPart& part = editorModel.parts[partIndex];

    for (int i = 0; i < part.polys.size(); ++i)
    {
        if (excludedPolys.count(i) > 0)
        {
            continue;
        }

        ModelEditorPoly& poly = part.polys[i];
       
        for (int v = 0; v < poly.vertexColors.size(); ++v)
        {
            uintptr_t colorAddrA = poly.bufferAddressA + (v * poly.vertexStride);
            uintptr_t colorAddrB = poly.bufferAddressB + (v * poly.vertexStride);

            game->write<uint8_t>(colorAddrA + 0, color.r);
            game->write<uint8_t>(colorAddrB + 0, color.r);
            game->write<uint8_t>(colorAddrA + 1, color.g);
            game->write<uint8_t>(colorAddrB + 1, color.g);
            game->write<uint8_t>(colorAddrA + 2, color.b);
            game->write<uint8_t>(colorAddrB + 2, color.b);
        }
    }
}

void ModelEditor::tintPart(int modelIndex, int partIndex, Utilities::Color color, const std::set<int>& excludedPolys)
{
    if (game == nullptr || modelIndex >= openModels.size())
    {
        return;
    }

    ModelEditorModel& editorModel = openModels[modelIndex];

    if (partIndex >= editorModel.parts.size())
    {
        return;
    }

    ModelEditorPart& part = editorModel.parts[partIndex];

    for (int i = 0; i < part.polys.size(); ++i)
    {
        if (excludedPolys.count(i) > 0)
        {
            continue;
        }

        ModelEditorPoly& poly = part.polys[i];

        for (int v = 0; v < poly.vertexColors.size(); ++v)
        {
            uintptr_t colorAddrA = poly.bufferAddressA + (v * poly.vertexStride);
            uintptr_t colorAddrB = poly.bufferAddressB + (v * poly.vertexStride);

            Utilities::Color tinted = tintVertexColor(poly.vertexColors[v], { color.r, color.g, color.b });

            game->write<uint8_t>(colorAddrA + 0, tinted.r);
            game->write<uint8_t>(colorAddrB + 0, tinted.r);
            game->write<uint8_t>(colorAddrA + 1, tinted.g);
            game->write<uint8_t>(colorAddrB + 1, tinted.g);
            game->write<uint8_t>(colorAddrA + 2, tinted.b);
            game->write<uint8_t>(colorAddrB + 2, tinted.b);
        }
    }
}

void ModelEditor::tintPolys(int modelIndex, int partIndex, Utilities::Color color, const std::set<int>& includedPolys)
{
    if (game == nullptr || modelIndex >= openModels.size())
    {
        return;
    }

    ModelEditorModel& editorModel = openModels[modelIndex];

    if (partIndex >= editorModel.parts.size())
    {
        return;
    }

    ModelEditorPart& part = editorModel.parts[partIndex];

    for (int i = 0; i < part.polys.size(); ++i)
    {
        if (includedPolys.count(i) == 0)
        {
            continue;
        }

        ModelEditorPoly& poly = part.polys[i];

        for (int v = 0; v < poly.vertexColors.size(); ++v)
        {
            uintptr_t colorAddrA = poly.bufferAddressA + (v * poly.vertexStride);
            uintptr_t colorAddrB = poly.bufferAddressB + (v * poly.vertexStride);

            Utilities::Color tinted = tintVertexColor(poly.vertexColors[v], { color.r, color.g, color.b });

            game->write<uint8_t>(colorAddrA + 0, tinted.r);
            game->write<uint8_t>(colorAddrB + 0, tinted.r);
            game->write<uint8_t>(colorAddrA + 1, tinted.g);
            game->write<uint8_t>(colorAddrB + 1, tinted.g);
            game->write<uint8_t>(colorAddrA + 2, tinted.b);
            game->write<uint8_t>(colorAddrB + 2, tinted.b);
        }
    }
}

void ModelEditor::tintPolyRange(int modelIndex, int partIndex, Utilities::Color color, uint32_t polyStart, uint32_t polyEnd)
{
    if (game == nullptr || modelIndex >= openModels.size())
    {
        return;
    }

    ModelEditorModel& editorModel = openModels[modelIndex];

    if (partIndex >= editorModel.parts.size())
    {
        return;
    }

    ModelEditorPart& part = editorModel.parts[partIndex];

    for (uint32_t i = polyStart; i <= polyEnd; ++i)
    {
        if (i >= part.polys.size())
        {
            break;
        }

        ModelEditorPoly& poly = part.polys[i];

        for (int v = 0; v < poly.vertexColors.size(); ++v)
        {
            uintptr_t colorAddrA = poly.bufferAddressA + (v * poly.vertexStride);
            uintptr_t colorAddrB = poly.bufferAddressB + (v * poly.vertexStride);

            Utilities::Color tinted = tintVertexColor(poly.vertexColors[v], { color.r, color.g, color.b });

            game->write<uint8_t>(colorAddrA + 0, tinted.r);
            game->write<uint8_t>(colorAddrB + 0, tinted.r);
            game->write<uint8_t>(colorAddrA + 1, tinted.g);
            game->write<uint8_t>(colorAddrB + 1, tinted.g);
            game->write<uint8_t>(colorAddrA + 2, tinted.b);
            game->write<uint8_t>(colorAddrB + 2, tinted.b);
        }
    }
}

Utilities::Color ModelEditor::tintVertexColor(const Utilities::Color& src, const Utilities::Color& tint)
{
    // Convert to grayscale (perceptual luminance)
    float gray = (0.299f * src.r + 0.587f * src.g + 0.114f * src.b) / 255.0f;

    // Boost the darks in the gray so the tint color comes out more
    gray = pow(gray, 0.75f);

    // Multiply by tint color
    float rf = gray * tint.r;
    float gf = gray * tint.g;
    float bf = gray * tint.b;

    // Clamp and convert back to 8-bit
    auto clamp8 = [](float v) -> uint8_t {
        return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
        };

    return { clamp8(rf), clamp8(gf), clamp8(bf) };
}

int ModelEditor::readPoly(int bufferIdx, ModelEditorPoly& polyOut)
{
    if (buffer == nullptr)
    {
        return 0;
    }

    polyOut.bufferAddressA = bufferAddress + (bufferIdx * 4);

    int curIdx = bufferIdx;

    uint32_t word0 = buffer[curIdx];
    uint8_t cmd  = (word0 >> 24) & 0xFF; // bits 31..24

    uint8_t polygon_render  = (cmd >> 5) & 0b111; // bits 31-29
    uint8_t gouraud_flag    = (cmd >> 4) & 0b1;   // bit 28
    uint8_t quad_flag       = (cmd >> 3) & 0b1;   // bit 27
    uint8_t textured_flag   = (cmd >> 2) & 0b1;   // bit 26
    uint8_t semi_transp     = (cmd >> 1) & 0b1;   // bit 25
    uint8_t raw_tex_mod     = cmd & 0b1;          // bit 24

    if (polygon_render != 1 || gouraud_flag != 1)
    {
        //LOG("Unknown command. Skipping...");
        return 0;
    }

    polyOut.textured = textured_flag == 1;
    polyOut.vertexStride = 8 + (textured_flag * 4);

    // Vert0 XXXXYYYY
    Utilities::Color vert0;
    vert0.r = word0 & 0xFF;              // bits 7..0
    vert0.g = (word0 >> 8) & 0xFF;       // bits 15..8
    vert0.b = (word0 >> 16) & 0xFF;      // bits 23..16
    curIdx += 2;
    if (textured_flag == 1)
    {
        // Vert0 UV
        curIdx++;
    }
    polyOut.vertexColors.push_back(vert0);

    // Vert1 RRGGBB00, XXXXYYYY
    uint32_t word1 = buffer[curIdx];
    Utilities::Color vert1;
    vert1.r = word1 & 0xFF;
    vert1.g = (word1 >> 8) & 0xFF;
    vert1.b = (word1 >> 16) & 0xFF;
    curIdx += 2;
    if (textured_flag == 1)
    {
        // Vert0 ClutVVUU
        curIdx++;
    }
    polyOut.vertexColors.push_back(vert1);

    // Vert2 RRGGBB00, XXXXYYYY
    uint32_t word2 = buffer[curIdx];
    Utilities::Color vert2;
    vert2.r = word2 & 0xFF;
    vert2.g = (word2 >> 8) & 0xFF;
    vert2.b = (word2 >> 16) & 0xFF;
    curIdx += 2;
    if (textured_flag == 1)
    {
        // Vert2 ClutVVUU
        curIdx++;
    }
    polyOut.vertexColors.push_back(vert2);

    if (quad_flag == 1)
    {
        // Vert3 RRGGBB00, XXXXYYYY
        uint32_t word3 = buffer[curIdx];
        Utilities::Color vert3;
        vert3.r = word3 & 0xFF;
        vert3.g = (word3 >> 8) & 0xFF;
        vert3.b = (word3 >> 16) & 0xFF;
        curIdx += 2;
        if (textured_flag == 1)
        {
            // Vert3 ClutVVUU
            curIdx++;
        }
        polyOut.vertexColors.push_back(vert3);
    }

    // Not sure what the extra 4 bytes is on the end of every command buffer entry
    curIdx++;

    return curIdx - bufferIdx;
}