#pragma once
#include <functional>

bool loadTextureFromMemory(const void* data, size_t data_size, uint32_t* out_texture, int* out_width, int* out_height);
bool loadTextureFromFile(const char* file_name, uint32_t* out_texture, int* out_width, int* out_height);

struct GLFWwindow;

class GUI
{
public:
    bool initialize();
    void destroy();

    bool beginFrame();
    void endFrame();

    bool wasWindowClosed();
    void drawLogo();

private:
    GLFWwindow* window;
};