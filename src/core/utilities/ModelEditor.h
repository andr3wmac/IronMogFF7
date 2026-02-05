#pragma once

#include "core/game/GameData.h"
#include "core/game/GameManager.h"
#include "core/utilities/Utilities.h"

#include <cstdint>
#include <set>

class ModelEditor
{
public:
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
        std::string modelName;
        std::vector<ModelEditorPart> parts;
    };

    ModelEditor();
    ~ModelEditor();

    void setup(GameManager* gameManager);
    void clear();
    void findFieldModels();
    void openBattleModels();
    bool areBattleModelsLoaded();
    std::vector<ModelEditor::ModelEditorModel>& getOpenModels();

    // Overwrites a parts color entirely without any regard for its original color.
    void setPartColor(int modelIndex, int partIndex, Utilities::Color color, const std::set<int>& excludedPolys = std::set<int>());

    // Applies a tint to an entire part minus any polys in excludedPolys.
    void tintPart(int modelIndex, int partIndex, Utilities::Color color, const std::set<int>& excludedPolys = std::set<int>());

    // Applies a tint to all the polys in a part specified by includedPolys.
    void tintPolys(int modelIndex, int partIndex, Utilities::Color color, const std::set<int>& includedPolys);

    // Applies a tint to all the polys in a part within the range of polyStart to polyEnd, inclusive.
    void tintPolyRange(int modelIndex, int partIndex, Utilities::Color color, uint32_t polyStart, uint32_t polyEnd);

protected:
    GameManager* game;

    uint32_t* buffer = nullptr;
    uintptr_t bufferAddress = 0;
    size_t bufferSize = 0;

    bool openFieldModel(int bufferIdx, const Model& model);
    bool openBattleModel(int bufferIdx, const BattleModel& model);

    int readPoly(int bufferIdx, ModelEditorPoly& polyOut);
    Utilities::Color tintVertexColor(const Utilities::Color& src, const Utilities::Color& tint);
    
    std::vector<ModelEditorModel> openModels;
};
