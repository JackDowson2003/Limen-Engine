#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <cstdint>
#include <iostream>

#include "Render/EditorUIRenderer.h"
#include "Limen/Application/Window.h"
#include "Limen/Core/Log.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Limen/Renderer/RendererCommand.h"
#include "Limen/RHI/RendererAPI.h"
#include "Limen/RHI/Texture.h"

namespace
{
    struct Pixel
    {
        std::uint8_t Red = 0;
        std::uint8_t Green = 0;
        std::uint8_t Blue = 0;
        std::uint8_t Alpha = 0;
    };

    static_assert(sizeof(Pixel) == 4);

    // 测试逻辑尺寸与帧缓冲尺寸不相同的窗口；OpenGL Context 由 GLFW 隐藏窗口持有。
    class TestWindow final : public Limen::Window
    {
    public:
        explicit TestWindow(GLFWwindow *window) : m_Window(window) {}

        void PollEvents() override {}
        void Present() override {}
        std::uint32_t GetWidth() const override { return 32; }
        std::uint32_t GetHeight() const override { return 32; }
        std::uint32_t GetWindowWidth() const override { return 16; }
        std::uint32_t GetWindowHeight() const override { return 16; }
        void SetEventCallback(const EventCallbackFn &) override {}
        void SetVSync(bool) override {}
        bool IsVSync() const override { return false; }
        void *GetNativeWindow() const override { return m_Window; }

    private:
        GLFWwindow *m_Window;
    };

    bool Near(const std::uint8_t actual, const std::uint8_t expected, const int tolerance)
    {
        const int difference = static_cast<int>(actual) - static_cast<int>(expected);
        return difference >= -tolerance && difference <= tolerance;
    }

    Pixel ReadPixel(const int x, const int y)
    {
        Pixel pixel;
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel);
        return pixel;
    }

    bool ExpectPixel(const char *name, const Pixel actual, const Pixel expected)
    {
        if (Near(actual.Red, expected.Red, 3) &&
            Near(actual.Green, expected.Green, 3) &&
            Near(actual.Blue, expected.Blue, 3) &&
            Near(actual.Alpha, expected.Alpha, 2))
            return true;

        std::cerr << name << ": expected RGBA ("
                  << static_cast<int>(expected.Red) << ", "
                  << static_cast<int>(expected.Green) << ", "
                  << static_cast<int>(expected.Blue) << ", "
                  << static_cast<int>(expected.Alpha) << "), got ("
                  << static_cast<int>(actual.Red) << ", "
                  << static_cast<int>(actual.Green) << ", "
                  << static_cast<int>(actual.Blue) << ", "
                  << static_cast<int>(actual.Alpha) << ")\n";
        return false;
    }

    bool Check(const char *name, const bool condition)
    {
        if (condition)
            return true;

        std::cerr << "FAIL: " << name << '\n';
        return false;
    }

    bool ExpectPartialGreen(const Pixel actual)
    {
        // 当前 Editor 在 RGBA8 上编码后混合；这里只锁定 Alpha 和可见通道，
        // 不把尚未确定的混合颜色空间数值当成长期契约。
        if (actual.Red <= 3 && actual.Blue <= 3 &&
            actual.Green > 32 && actual.Green < 224 &&
            Near(actual.Alpha, 128, 2))
            return true;

        std::cerr << "partially transparent green: unexpected RGBA ("
                  << static_cast<int>(actual.Red) << ", "
                  << static_cast<int>(actual.Green) << ", "
                  << static_cast<int>(actual.Blue) << ", "
                  << static_cast<int>(actual.Alpha) << ")\n";
        return false;
    }

    bool AnyVisiblePixel(const int x0, const int y0, const int x1, const int y1)
    {
        for (int y = y0; y < y1; ++y)
        {
            for (int x = x0; x < x1; ++x)
            {
                if (ReadPixel(x, y).Alpha > 0)
                    return true;
            }
        }
        return false;
    }

    void FillAsymmetricTexture(std::uint8_t (&data)[4 * 4 * 4])
    {
        // SetData 的第 0 行是 OpenGL 纹理底行。四个象限各占 2x2 像素，
        // 使 GL_LINEAR 采样点落在同色块内，避免边界插值影响方向检查。
        for (int y = 0; y < 4; ++y)
        {
            for (int x = 0; x < 4; ++x)
            {
                Pixel pixel;
                if (y >= 2 && x < 2)
                    pixel = {255, 0, 0, 255};       // 左上：红
                else if (y >= 2)
                    pixel = {0, 255, 0, 128};       // 右上：半透明绿
                else if (x < 2)
                    pixel = {0, 0, 255, 255};       // 左下：蓝
                else
                    pixel = {128, 64, 32, 255};     // 右下：sRGB 中间值

                const int offset = (y * 4 + x) * 4;
                data[offset + 0] = pixel.Red;
                data[offset + 1] = pixel.Green;
                data[offset + 2] = pixel.Blue;
                data[offset + 3] = pixel.Alpha;
            }
        }
    }
}

int main()
{
    Limen::Log::Init();

    if (glfwInit() != GLFW_TRUE)
    {
        std::cerr << "GLFW initialization failed\n";
        return 1;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow *contextWindow = glfwCreateWindow(32, 32, "Limen Editor UI texture test", nullptr, nullptr);
    if (!contextWindow)
    {
        std::cerr << "OpenGL test window creation failed\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(contextWindow);
    if (gladLoadGL(glfwGetProcAddress) == 0)
    {
        std::cerr << "GLAD initialization failed\n";
        glfwDestroyWindow(contextWindow);
        glfwTerminate();
        return 1;
    }

    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major < 4 || (major == 4 && minor < 1))
    {
        std::cerr << "OpenGL 4.1 required; got " << major << '.' << minor << '\n';
        glfwDestroyWindow(contextWindow);
        glfwTerminate();
        return 1;
    }

    Limen::RendererAPI::SetAPI(Limen::RendererAPI::API::OPENGL);
    Limen::RendererCommand::Init();

    GLuint outputTexture = 0;
    GLuint framebuffer = 0;
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
    const bool framebufferReady = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

    bool success = framebufferReady;
    if (!framebufferReady)
        std::cerr << "Editor UI test framebuffer is incomplete\n";

    {
        Limen::Texture2DSpecification specification;
        specification.Width = 4;
        specification.Height = 4;
        specification.Format = Limen::TextureFormat::RGBA8;
        specification.ColorSpace = Limen::TextureColorSpace::SRGB;
        specification.GenerateMipmaps = false;

        auto sourceTexture = Limen::Texture2D::Create(specification);
        if (!sourceTexture)
        {
            std::cerr << "Editor UI source texture creation failed\n";
            success = false;
        }

        LimenEditor::EditorUIRenderer renderer;
        const bool rendererReady = renderer.Initialize();
        if (!rendererReady)
        {
            std::cerr << "Editor UI renderer initialization failed\n";
            success = false;
        }

        if (framebufferReady && sourceTexture && rendererReady)
        {
            std::uint8_t data[4 * 4 * 4]{};
            FillAsymmetricTexture(data);
            sourceTexture->SetData(data, sizeof(data));

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glDisable(GL_FRAMEBUFFER_SRGB);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            TestWindow window(contextWindow);
            renderer.DrawTexturedRect(window, {4.0f, 4.0f}, {8.0f, 8.0f}, *sourceTexture);

            // 矩形从 16x16 逻辑空间的 (4,4)-(12,12) 映射到 32x32 物理空间。
            success = ExpectPixel("outside rectangle", ReadPixel(2, 2), {0, 0, 0, 0}) && success;
            success = ExpectPixel("top-left UV", ReadPixel(11, 20), {255, 0, 0, 255}) && success;
            success = ExpectPartialGreen(ReadPixel(20, 20)) && success;
            success = ExpectPixel("bottom-left UV", ReadPixel(11, 11), {0, 0, 255, 255}) && success;
            success = ExpectPixel("sRGB gray roundtrip", ReadPixel(20, 11), {128, 64, 32, 255}) && success;
            success = ExpectPixel("logical-to-framebuffer edge", ReadPixel(6, 16), {0, 0, 0, 0}) && success;

            // 直通路径：把已知显示编码颜色写进离屏纹理，再直通绘制到默认帧缓冲。
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDrawBuffer(GL_BACK);
            glReadBuffer(GL_BACK);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            renderer.DrawNativeTextureRect(
                window,
                {4.0f, 4.0f},
                {8.0f, 8.0f},
                static_cast<std::uintptr_t>(outputTexture)
            );
            success = ExpectPixel("native texture pass-through",
                                  ReadPixel(16, 16),
                                  {64, 128, 191, 255}) && success;
            success = ExpectPixel("native texture rect edge",
                                  ReadPixel(28, 28),
                                  {0, 0, 0, 0}) && success;

            // 文字：字形图集 Build 后绘制，画面里应出现非零 Alpha 的像素。
            Limen::FontAtlas textAtlas(256, 32.0f);
            const bool fontReady =
                textAtlas.LoadFontFile(LIMEN_TEST_FONT_PATH);
            if (fontReady)
            {
                textAtlas.AddText("Limen A 场");
                textAtlas.Build();
            }
            if (fontReady && textAtlas.IsBuilt())
            {
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                renderer.DrawText(
                    window,
                    {4.0f, 4.0f},
                    {1.0f, 1.0f, 1.0f, 1.0f},
                    textAtlas,
                    "Limen A 场");
                const bool visible = AnyVisiblePixel(0, 0, 32, 32);
                success = Check("DrawText emits no GL error",
                                glGetError() == GL_NO_ERROR) && success;
                success = Check("DrawText produces visible pixels", visible) && success;

                // 隔离检查：整屏画一个图集左上象限，排除 DrawText 排版干扰。
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                renderer.DrawGlyph(
                    window,
                    {0.0f, 0.0f},
                    {16.0f, 16.0f},
                    {1.0f, 1.0f, 1.0f, 1.0f},
                    textAtlas.GetTexture(),
                    {0.0f, 0.0f},
                    {0.25f, 0.25f});
                const bool glyphQuadVisible = AnyVisiblePixel(0, 0, 32, 32);
                success = Check("glyph quad emits no GL error",
                                glGetError() == GL_NO_ERROR) && success;
                success = Check("glyph quad produces visible pixels",
                                glyphQuadVisible) && success;

                // 方向检查：正立的 'L' 是竖笔在左、横杠在底部；
                // 整屏 quad 上，底部右侧应被横杠覆盖、顶部右侧应为空。
                const Limen::FontAtlas::Glyph *letterL =
                    textAtlas.FindGlyph(U'L');
                if (letterL)
                {
                    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    renderer.DrawGlyph(
                        window,
                        {0.0f, 0.0f},
                        {16.0f, 16.0f},
                        {1.0f, 1.0f, 1.0f, 1.0f},
                        textAtlas.GetTexture(),
                        {letterL->U0, letterL->V0},
                        {letterL->U1, letterL->V1});
                    const Pixel bottomRight = ReadPixel(24, 2);
                    const Pixel topRight = ReadPixel(24, 30);
                    success = Check(
                        "L bar is at glyph bottom (upright)",
                        bottomRight.Alpha > 100 && topRight.Alpha <= 3
                    ) && success;
                }
                else
                {
                    std::cerr << "glyph 'L' missing from atlas\n";
                    success = false;
                }

                // 裁剪：左半裁剪区内画整屏红色，右侧必须保持清屏色。
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                renderer.SetClipRect(window, {0.0f, 0.0f}, {8.0f, 16.0f});
                renderer.DrawSolidRect(
                    window,
                    {0.0f, 0.0f},
                    {16.0f, 16.0f},
                    {1.0f, 0.0f, 0.0f, 1.0f});
                renderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
                success = Check("scissor is disabled after clip cancel",
                                !glIsEnabled(GL_SCISSOR_TEST)) && success;

                success = ExpectPixel("clipped left half is red",
                                      ReadPixel(4, 16),
                                      {255, 0, 0, 255}) && success;
                success = ExpectPixel("unclipped right half stays clear",
                                      ReadPixel(24, 16),
                                      {0, 0, 0, 0}) && success;

                // 取消裁剪后，右侧区域必须可以继续正常绘制。
                renderer.DrawSolidRect(
                    window,
                    {12.0f, 12.0f},
                    {4.0f, 4.0f},
                    {0.0f, 1.0f, 0.0f, 1.0f});
                success = Check("green draw emits no GL error",
                                glGetError() == GL_NO_ERROR) && success;
                success = ExpectPixel("draw after clip cancel reaches right half",
                                      ReadPixel(28, 4),
                                      {0, 255, 0, 255}) && success;
            }
            else
            {
                std::cerr << "Editor UI text atlas build failed\n";
                success = false;
            }
        }

        renderer.Shutdown();
        sourceTexture.reset();
    }

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "Editor UI OpenGL error: " << error << '\n';
        success = false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &outputTexture);
    Limen::RendererCommand::Shutdown();
    glfwDestroyWindow(contextWindow);
    glfwTerminate();

    if (success)
        std::cout << "Editor UI texture UV, sRGB, alpha, and scale passed\n";
    return success ? 0 : 1;
}
