#include "GUI.h"

#define IMGUI_IMPLEMENTATION
#include "misc/single_file/imgui_single_file.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "IconsFontAwesome5.h"

#include <stdio.h>
#include <iostream>

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Native file dialog
#include "nfd/nfd.h"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

// We use this to override resizing to allow vertical only.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) 
{
    switch (uMsg) 
    {
        case WM_GETMINMAXINFO: 
        {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            long lockedOuterWidth = (long)dwRefData;
            mmi->ptMinTrackSize.x = lockedOuterWidth;
            mmi->ptMaxTrackSize.x = lockedOuterWidth;
            return 0;
        }

        case WM_SETCURSOR: 
        {
            // If the mouse is over the left or right resize borders
            WORD hitTest = LOWORD(lParam);
            if (hitTest == HTLEFT || hitTest == HTRIGHT) 
            {
                // Set the cursor back to the standard arrow manually
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                return TRUE;
            }
            break;
        }
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}
#endif

// Fonts we loaded from the resources folder
std::unordered_map<std::string, ImFont*> fonts;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void setupStyle()
{
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.04f, 0.04f, 0.04f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.15f, 0.15f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.48f, 0.26f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.37f, 0.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.21f, 0.16f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.45f, 0.26f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.41f, 0.00f, 1.00f, 0.40f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.48f, 0.26f, 0.98f, 0.52f);
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.20f, 0.20f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.34f, 0.06f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.15f, 0.15f, 0.15f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(1.00f, 1.00f, 1.00f, 0.13f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.40f, 0.26f, 0.98f, 0.50f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.20f, 0.58f, 0.73f);
    colors[ImGuiCol_TabSelected]            = ImVec4(0.29f, 0.20f, 0.68f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabDimmed]              = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected]      = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline]  = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink]               = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

float GUI::dpiScale = 1.0f;

GUI::GUI()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
}

void GUI::registerSettingsHandler(const char* name, std::function<void(const char*, const char*)> onReadLine, std::function<void(ImGuiTextBuffer*)> onWrite)
{
    auto entry = std::make_unique<SettingsHandler>();
    entry->typeName = name;
    entry->readLineFn = onReadLine;
    entry->writeAllFn = onWrite;

    ImGuiSettingsHandler imgui_handler;
    imgui_handler.TypeName = entry->typeName.c_str();
    imgui_handler.TypeHash = ImHashStr(imgui_handler.TypeName);
    imgui_handler.UserData = entry.get();
    imgui_handler.ReadOpenFn = GUI::imGuiSettingsReadOpen;
    imgui_handler.ReadLineFn = GUI::imGuiSettingsReadLine;
    imgui_handler.WriteAllFn = GUI::imGuiSettingsWriteAll;

    ImGui::GetCurrentContext()->SettingsHandlers.push_back(imgui_handler);
    settingsHandlers.push_back(std::move(entry));
}

bool GUI::initialize(int width, int height, const char* windowTitle)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        return false;
    }

    // Get DPI scaling, valid on GLFW 3.3+ only
    dpiScale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); 

    // Create window with graphics context
    windowWidth = DPI(width);
    windowHeight = DPI(height);
    window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
    if (window == nullptr)
    {
        return false;
    }

#ifdef _WIN32
    {
        // Get the true window width then apply a subclass so we can lock horizontal resizing.
        HWND hwnd = glfwGetWin32Window(window);
        RECT rect;
        GetWindowRect(hwnd, &rect);
        SetWindowSubclass(hwnd, WindowProc, 1, rect.right - rect.left);
    }
#endif

    glfwSetWindowUserPointer(window, this);

    // Resize window callback
    auto resizeCallbackFunc = [](GLFWwindow* window, int width, int height)
        {
            static_cast<GUI*>(glfwGetWindowUserPointer(window))->onResizeCallback(window, width, height);
        };
    glfwSetFramebufferSizeCallback(window, resizeCallbackFunc);
    
    // Key down callback
    auto keyCallbackFunc = [](GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        static_cast<GUI*>(glfwGetWindowUserPointer(window))->onKeyCallback(key, scancode, action, mods);
    };
    glfwSetKeyCallback(window, keyCallbackFunc);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.IniFilename = "settings/app.ini";

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    // Default font + icon font
    io.Fonts->AddFontDefault();
    float baseFontSize = 16.0f; // 13.0f is the size of the default font. Change to the font size you use.
    float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    fonts["Icon"] = io.Fonts->AddFontFromFileTTF("resources/fa-solid-900.ttf", iconFontSize, &icons_config, icons_ranges);

    // Google's Inter font
    fonts["Inter"] = io.Fonts->AddFontFromFileTTF("resources/Inter_18pt-Regular.ttf", 18.0f);

    // Reactor7 font
    fonts["Reactor7"] = io.Fonts->AddFontFromFileTTF("resources/Reactor7.ttf", 18.0f);

    // Setup style
    setupStyle();

    // Apply DPI scaling to style and fonts
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(dpiScale);
    style.FontScaleDpi = dpiScale;

    // Window icon
    {
        const char* iconPath = "resources/icon.png";

        GLFWimage images[1];
        images[0].pixels = stbi_load(iconPath, &images[0].width, &images[0].height, 0, 4); // RGBA

        if (images[0].pixels) 
        {
            glfwSetWindowIcon(window, 1, images);
            stbi_image_free(images[0].pixels);
        }
        else 
        {
            fprintf(stderr, "Failed to load icon: %s\n", iconPath);
        }
    }

    // Native file dialog
    NFD_Init();

    return true;
}

void GUI::destroy()
{
    NFD_Quit();

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

bool GUI::beginFrame()
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
    {
        ImGui_ImplGlfw_Sleep(10);
        return false;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Force frame to be the size of the window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    return true;
}

void GUI::endFrame()
{
    static const ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

bool GUI::wasWindowClosed()
{
    return glfwWindowShouldClose(window);
}

void GUI::onResizeCallback(GLFWwindow* window, int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    onResize.invoke(width, height);
}

void GUI::onKeyCallback(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        onKeyPress.invoke(key, mods);
    }
}

void GUI::pushFont(const std::string& fontName)
{
    if (fonts.count(fontName) == 0)
    {
        return;
    }

    ImGui::PushFont(fonts[fontName]);
}

void GUI::popFont()
{
    ImGui::PopFont();
}

std::string GUI::openFileDialog()
{
    std::string result = "";
    nfdu8char_t* outPath;
    nfdu8filteritem_t filters[1] = { { "Settings File", "cfg" } };

    nfdopendialogu8args_t args = { 0 };
    args.filterList = filters;
    args.filterCount = 1;

    nfdresult_t err = NFD_OpenDialogU8_With(&outPath, &args);
    if (err == NFD_OKAY)
    {
        result = outPath;
        NFD_FreePathU8(outPath);
    }
    else if (err == NFD_CANCEL)
    {
        // Do nothing.
    }
    else
    {
        fprintf(stderr, "Error: %s\n", NFD_GetError());
    }

    return result;
}

std::string GUI::saveFileDialog()
{
    std::string result = "";
    nfdchar_t* savePath;
    nfdfilteritem_t filterItem[1] = { { "Settings File", "cfg" } };

    nfdsavedialogu8args_t args = { 0 };
    args.filterList = filterItem;
    args.filterCount = 1;
    args.defaultName = "Untitled.cfg";

    nfdresult_t err = NFD_SaveDialogU8_With(&savePath, &args);
    if (err == NFD_OKAY)
    {
        result = savePath;
        NFD_FreePath(savePath);
    }
    else if (err == NFD_CANCEL)
    {
        // Do nothing.
    }
    else 
    {
        fprintf(stderr, "Error: %s\n", NFD_GetError());
    }

    return result;
}

bool GUIImage::loadFromMemory(const void* data, size_t data_size)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    textureID = image_texture;;
    width     = image_width;
    height    = image_height;

    return true;
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool GUIImage::loadFromFile(const char* file_name)
{
    FILE* f = fopen(file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    fclose(f);
    bool ret = loadFromMemory(file_data, file_size);
    IM_FREE(file_data);
    return ret;
}

void GUI::drawImage(GUIImage& image, int width, int height, float alpha)
{
    if (alpha < 1.0f)
    {
        ImGui::ImageWithBg((ImTextureID)image.textureID, ImVec2((float)width, (float)height), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, alpha));
    }
    else
    {
        ImGui::Image((ImTextureID)image.textureID, ImVec2((float)width, (float)height));
    }
}

void GUI::drawColorGrid(const std::string& name, std::vector<Utilities::Color>& colors, std::function<void(int, Utilities::Color)> onClickCallback, float boxSize, float spacing, int colorsPerRow)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 startPos = ImGui::GetCursorScreenPos();

    for (size_t i = 0; i < colors.size(); ++i)
    {
        ImU32 color = IM_COL32(colors[i].r, colors[i].g, colors[i].b, 255);

        // compute position in grid
        int row = static_cast<int>(i / colorsPerRow);
        int col = static_cast<int>(i % colorsPerRow);
        ImVec2 p0 = ImVec2(startPos.x + col * (boxSize + spacing), startPos.y + row * (boxSize + spacing));
        ImVec2 p1 = ImVec2(p0.x + boxSize, p0.y + boxSize);

        // draw filled rect
        drawList->AddRectFilled(p0, p1, color);
        drawList->AddRect(p0, p1, IM_COL32(60, 60, 60, 255)); // border

        // Make an invisible button for interaction
        ImGui::SetCursorScreenPos(p0);
        ImGui::InvisibleButton((name + ".color" + std::to_string(i)).c_str(), ImVec2(boxSize, boxSize));

        if (ImGui::IsItemClicked())
        {
            if (onClickCallback)
            {
                onClickCallback((int)i, colors[i]);
            }
        }
    }

    // advance cursor so the next ImGui item doesn't overlap
    //int totalRows = static_cast<int>((colors.size() + colorsPerRow - 1) / colorsPerRow);
    //ImGui::Dummy(ImVec2(0.0f, totalRows * (boxSize + spacing)));
}

void GUI::wrappedTooltip(const std::string& text, float maxWidth)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + maxWidth);
        ImGui::TextWrapped(text.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void GUI::textCentered(const std::string& text, int width)
{
    auto windowWidth = width;
    auto textWidth = ImGui::CalcTextSize(text.c_str()).x;

    // Calculate the starting X position
    float cursorX = (windowWidth - textWidth) * 0.5f;
    if (cursorX > 0) 
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cursorX);
    }

    ImGui::Text(text.c_str());
}

void* GUI::imGuiSettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name)
{
    auto* cfg = static_cast<SettingsHandler*>(handler->UserData);
    cfg->lastSectionName = name;
    return (void*)(cfg->lastSectionName.c_str());
}

void GUI::imGuiSettingsReadLine(ImGuiContext*, ImGuiSettingsHandler* handler, void* entry, const char* line)
{
    auto* cfg = static_cast<SettingsHandler*>(handler->UserData);
    if (cfg->readLineFn) cfg->readLineFn(cfg->lastSectionName.c_str(), line);
}

void GUI::imGuiSettingsWriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    auto* cfg = static_cast<SettingsHandler*>(handler->UserData);
    if (cfg->writeAllFn) cfg->writeAllFn(buf);
}