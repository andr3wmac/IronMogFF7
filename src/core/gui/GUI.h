#pragma once
#include <functional>
#include "../utilities/Event.h"

struct GLFWwindow;
struct GUIImage;

class GUI
{
public:
    Event<int, int> onKeyPress;

    bool initialize();
    void destroy();

    bool beginFrame();
    void endFrame();

    bool wasWindowClosed();
    void drawImage(GUIImage& image, int width, int height, float alpha = 1.0f);

    void pushIconFont();
    void popIconFont();

private:
    GLFWwindow* window;

    void onKeyCallback(int key, int scancode, int action, int mods);
};

struct GUIImage
{
    uint32_t textureID = 0;
    int width = 0;
    int height = 0;

    bool loadFromMemory(const void* data, size_t data_size);
    bool loadFromFile(const char* file_name);
};