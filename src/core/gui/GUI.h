#pragma once
#include <functional>
#include "core/utilities/Event.h"
#include "core/utilities/Utilities.h"

struct GLFWwindow;
struct GUIImage;

class GUI
{
public:
    Event<int, int> onKeyPress;

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

    void onKeyCallback(int key, int scancode, int action, int mods);

public:
    static void drawImage(GUIImage& image, int width, int height, float alpha = 1.0f);
    static void drawColorGrid(const std::string& name, std::vector<Utilities::Color>& colors, std::function<void(int, Utilities::Color)> onClickCallback = {}, float boxSize = 16.0f, float spacing = 2.0f, int colorsPerRow = 24);
    static void wrappedTooltip(const std::string& text, float maxWidth = 400.0f);
};

struct GUIImage
{
    uint32_t textureID = 0;
    int width = 0;
    int height = 0;

    bool loadFromMemory(const void* data, size_t data_size);
    bool loadFromFile(const char* file_name);
};