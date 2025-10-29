#pragma once

#include "core/game/GameData.h"
#include "core/game/GameManager.h"
#include "core/utilities/Utilities.h"

#include <cstdint>
#include <set>

class ModelEditor
{
    struct ModelEditorPoly
    {
        bool textured = false;
        uintptr_t bufferAddressA = 0;
        uintptr_t bufferAddressB = 0;
        size_t vertexStride = 0;
        std::vector<Utilities::Color> vertexColors;
    };

    struct ModelEditorPart
    {
        std::vector<ModelEditorPoly> polys;
    };

    struct ModelEditorModel
    {
        std::vector<ModelEditorPart> parts;
    };

public:
    ModelEditor();
    ~ModelEditor();

    void setup(GameManager* gameManager);
    void findModels();
    std::vector<std::string> getOpenModelNames();

    // Overwrites a parts color entirely without any regard for its original color.
    void setPartColor(std::string modelName, int partIndex, Utilities::Color color, const std::set<int>& excludedPolys = std::set<int>());

    // Applies a tint to an entire part minus any polys in excludedPolys.
    void tintPart(std::string modelName, int partIndex, Utilities::Color color, const std::set<int>& excludedPolys = std::set<int>());

    // Applies a tint to all the polys in a part specified by includedPolys.
    void tintPolys(std::string modelName, int partIndex, Utilities::Color color, const std::set<int>& includedPolys);

    // Applies a tint to all the polys in a part within the range of polyStart to polyEnd, inclusive.
    void tintPolyRange(std::string modelName, int partIndex, Utilities::Color color, uint32_t polyStart, uint32_t polyEnd);

protected:
    GameManager* game;

    uint32_t* buffer = nullptr;
    uintptr_t bufferAddress = 0;
    size_t bufferSize = 0;

    bool openModel(int bufferIdx, const Model& model);
    int readPoly(int bufferIdx, ModelEditorPoly& polyOut);
    Utilities::Color tintVertexColor(const Utilities::Color& src, const Utilities::Color& tint);
    
    std::unordered_map<std::string, ModelEditorModel> openModels;
};
