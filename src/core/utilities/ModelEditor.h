#pragma once

#include <cstdint>
#include "core/game/GameManager.h"

class ModelEditor
{
    struct ModelEditorVertex
    {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
    };

    struct ModelEditorPoly
    {
        bool textured = false;
        uintptr_t bufferAddressA = 0;
        uintptr_t bufferAddressB = 0;
        size_t stride = 0;
        std::vector<ModelEditorVertex> vertices;
    };

    struct ModelEditorPart
    {
        std::vector<ModelEditorPoly> polys;
    };

public:
    ModelEditor(GameManager* gameManager);

    bool findModel(std::string modelName);
    bool openModel(uintptr_t address, std::string modelName);
    void setPartColor(int partIndex, uint8_t red, uint8_t green, uint8_t blue);
    void setPartTint(int partIndex, uint8_t red, uint8_t green, uint8_t blue);

protected:
    GameManager* game;
    std::vector<ModelEditorPart> parts;
    uint32_t* searchData = nullptr;

    size_t readPoly(uintptr_t address, ModelEditorPoly& polyOut);
    void readAllPolys(uintptr_t address, int maxReadPolys);
    ModelEditorVertex tintVertexColor(const ModelEditorVertex& src, const ModelEditorVertex& tint);
};
