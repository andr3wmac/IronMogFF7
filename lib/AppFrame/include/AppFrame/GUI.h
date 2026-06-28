#pragma once

#include "AppFrame/Event.h"
#include "AppFrame/ImGui.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;
struct ImFont;
struct ImGuiContext;
struct ImGuiSettingsHandler;
struct ImGuiTextBuffer;

namespace AppFrame
{
    struct AppConfig;

    struct Color
    {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
    };

    struct FontSpec
    {
        std::string name;
        std::string path;
        float size = 18.0f;
    };

    struct SettingsHandler
    {
        std::string typeName;
        std::function<void(const char* section, const char* line)> readLineFn;
        std::function<void(ImGuiTextBuffer* buf)> writeAllFn;
        std::string lastSectionName = "";
    };

    struct GUIImage;

    class GUI
    {
    public:
        int windowWidth = 0;
        int windowHeight = 0;
        Event<int, int> onKeyPress;
        Event<int, int> onResize;

        GUI();
        bool initialize(const AppConfig& config);
        void destroy();

        bool beginFrame(bool pollEvents = true);
        void endFrame();

        bool wasWindowClosed();

        void pushFont(const std::string& fontName);
        void popFont();

        // Returns a font loaded during initialize() by name (e.g. "Reactor7"), or nullptr if not found.
        static ImFont* getFont(const std::string& fontName);

        std::string openFileDialog();
        std::string saveFileDialog();

    private:
        GLFWwindow* window = nullptr;
        std::string iniFilenameStorage;

        void onResizeCallback(GLFWwindow* window, int width, int height);
        void onKeyCallback(int key, int scancode, int action, int mods);

    public:
        static void readIni(const char* filePath, const char* name, std::function<void(const char*, const char*)> onReadLine);
        static void registerSettingsHandler(const char* name, std::function<void(const char*, const char*)> onReadLine, std::function<void(ImGuiTextBuffer*)> onWrite);

        static void drawImage(GUIImage& image, int width, int height, float alpha = 1.0f);
        static void drawColorGrid(const std::string& name, std::vector<Color>& colors, std::function<void(int, Color)> onClickCallback = {}, float boxSize = 16.0f, float spacing = 2.0f, int colorsPerRow = 24);
        template<typename TColor>
        static void drawColorGrid(const std::string& name, std::vector<TColor>& colors, float boxSize = 16.0f, float spacing = 2.0f, int colorsPerRow = 24);
        template<typename TColor, typename TCallback>
        static void drawColorGrid(const std::string& name, std::vector<TColor>& colors, TCallback onClickCallback, float boxSize = 16.0f, float spacing = 2.0f, int colorsPerRow = 24);
        static void wrappedTooltip(const std::string& text, float maxWidth = 400.0f);
        static void textCentered(const std::string& text, int width);
    
        static void* imGuiSettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name);
        static void imGuiSettingsReadLine(ImGuiContext*, ImGuiSettingsHandler* handler, void* entry, const char* line);
        static void imGuiSettingsWriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);

        static std::vector<std::unique_ptr<SettingsHandler>> settingsHandlers;
        static float dpiScale;
    };

    struct GUIImage
    {
        uint32_t textureID = 0;
        int width = 0;
        int height = 0;

        bool loadFromMemory(const void* data, size_t data_size);
        bool loadFromFile(const char* file_name);
    };

    // Helper function that scales a value by DPI scale for displays using scaling.
    template <typename T>
    inline T DPI(T value)
    {
        return static_cast<T>(value * GUI::dpiScale);
    }

    template<typename TColor>
    void GUI::drawColorGrid(const std::string& name, std::vector<TColor>& colors, float boxSize, float spacing, int colorsPerRow)
    {
        drawColorGrid(name, colors, [](int, TColor) {}, boxSize, spacing, colorsPerRow);
    }

    template<typename TColor, typename TCallback>
    void GUI::drawColorGrid(const std::string& name, std::vector<TColor>& colors, TCallback onClickCallback, float boxSize, float spacing, int colorsPerRow)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 startPos = ImGui::GetCursorScreenPos();

        for (size_t i = 0; i < colors.size(); ++i)
        {
            ImU32 color = IM_COL32(colors[i].r, colors[i].g, colors[i].b, 255);

            int row = static_cast<int>(i / colorsPerRow);
            int col = static_cast<int>(i % colorsPerRow);
            ImVec2 p0 = ImVec2(startPos.x + col * (boxSize + spacing), startPos.y + row * (boxSize + spacing));
            ImVec2 p1 = ImVec2(p0.x + boxSize, p0.y + boxSize);

            drawList->AddRectFilled(p0, p1, color);
            drawList->AddRect(p0, p1, IM_COL32(60, 60, 60, 255));

            ImGui::SetCursorScreenPos(p0);
            ImGui::InvisibleButton((name + ".color" + std::to_string(i)).c_str(), ImVec2(boxSize, boxSize));

            if (ImGui::IsItemClicked())
            {
                onClickCallback((int)i, colors[i]);
            }
        }
    }
}

// Compatibility wrapper while apps migrate to AppFrame::DPI.
template <typename T>
inline T DPI(T value)
{
    return AppFrame::DPI(value);
}
