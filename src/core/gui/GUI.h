#pragma once
#include <functional>
#include "core/utilities/Event.h"
#include "core/utilities/Utilities.h"

struct GLFWwindow;
struct GUIImage;
struct ImGuiContext;
struct ImGuiSettingsHandler;
struct ImGuiTextBuffer;

struct SettingsHandler
{
    std::string typeName;
    std::function<void(const char* section, const char* line)> readLineFn;
    std::function<void(ImGuiTextBuffer* buf)> writeAllFn;
    std::string lastSectionName = "";
};

class GUI
{
public:
    int windowWidth = 0;
    int windowHeight = 0;
    Event<int, int> onKeyPress;
    Event<int, int> onResize;

    GUI();
    bool initialize(int width, int height, const char* windowTitle);
    void destroy();

    bool beginFrame();
    void endFrame();

    bool wasWindowClosed();

    void pushFont(const std::string& fontName);
    void popFont();

    std::string openFileDialog();
    std::string saveFileDialog();

private:
    GLFWwindow* window;

    void onResizeCallback(GLFWwindow* window, int width, int height);
    void onKeyCallback(int key, int scancode, int action, int mods);

public:
    static void readIni(const char* filePath, const char* name, std::function<void(const char*, const char*)> onReadLine);
    static void registerSettingsHandler(const char* name, std::function<void(const char*, const char*)> onReadLine, std::function<void(ImGuiTextBuffer*)> onWrite);

    static void drawImage(GUIImage& image, int width, int height, float alpha = 1.0f);
    static void drawColorGrid(const std::string& name, std::vector<Utilities::Color>& colors, std::function<void(int, Utilities::Color)> onClickCallback = {}, float boxSize = 16.0f, float spacing = 2.0f, int colorsPerRow = 24);
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

// Helper function that scales a value by DPI scale for displays using scaling
template <typename T>
inline T DPI(T value)
{
    return static_cast<T>(value * GUI::dpiScale);
}