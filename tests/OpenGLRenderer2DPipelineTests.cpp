#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <iostream>

#include "Limen/Core/Log.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/IndexBuffer.h"
#include "Limen/RHI/RendererAPI.h"
#include "Limen/Renderer/Renderer.h"
#include "Limen/RHI/Framebuffer.h"
#include "Limen/Renderer/RenderPass.h"
#include "Limen/Renderer/OrthoGraphicCamera.h"
#include "Limen/Renderer/Renderer2D.h"
#include "Limen/RHI/Shader.h"
#include "Limen/RHI/Texture.h"
#include "Limen/RHI/VertexArray.h"

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

    GLFWwindow *window = glfwCreateWindow(
        16, 16, "Limen Renderer2D test", nullptr, nullptr
    );

    if (window == nullptr)
    {
        std::cerr << "OpenGL test window creation failed\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    bool success = gladLoadGL(glfwGetProcAddress) != 0;
    if (!success)
    {
        std::cerr << "GLAD initialization failed\n";
    } else
    {
        GLint major = 0;
        GLint minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);

        success = major > 4 || (major == 4 && minor >= 1);
        if (!success)
        {
            std::cerr << "OpenGL 4.1 required; got "
                    << major << '.' << minor << '\n';
        }
    }

    if (success)
    {
        Limen::RendererAPI::SetAPI(Limen::RendererAPI::API::OPENGL);
        Limen::Renderer::Init();

        bool scenePassReady = false;
        bool resolvedBackgroundCorrect = false;
        bool resolvedQuadCorrect = false;
        bool resolvedTextureCorrect = false;
        bool displayPassReady = false;
        bool displayDrawReady = false;
        bool displayBackgroundCorrect = false;
        bool displayQuadCorrect = false;
        bool displayTextureCorrect = false;
        bool displayShaderReady = false;
        bool displayPipelineReady = false;
        bool displayGeometryReady = false;
        {
            Limen::FramebufferSpecification sceneSpec;
            sceneSpec.Width = 16;
            sceneSpec.Height = 16;
            sceneSpec.Samples = 4;

            auto sceneFramebuffer = Limen::Framebuffer::Create(sceneSpec);

            if (!sceneFramebuffer)
            {
                std::cerr << "Scene framebuffer creation failed\n";
            } else if (sceneFramebuffer->GetSpecification().Samples != 4)
            {
                std::cerr << "4x MSAA is unavailable\n";
            } else
            {
                Limen::Texture2DSpecification textureSpec;
                textureSpec.Width = 1;
                textureSpec.Height = 1;
                textureSpec.Format = Limen::TextureFormat::RGBA8;
                textureSpec.ColorSpace = Limen::TextureColorSpace::SRGB;
                textureSpec.GenerateMipmaps = false;

                auto testTexture = Limen::Texture2D::Create(textureSpec);
                if (!testTexture)
                {
                    std::cerr << "sRGB test texture creation failed\n";
                } else
                {
                    constexpr GLubyte inputPixel[4] = {128, 64, 0, 255};
                    testTexture->SetData(
                        inputPixel,
                        sizeof(inputPixel)
                    );
                }
                Limen::RenderPassSpecification passSpec;
                passSpec.TargetFramebuffer = sceneFramebuffer.get();
                passSpec.ColorLoadOperation =
                        Limen::AttachmentLoadOperation::Clear;
                passSpec.ColorStoreOperation =
                        Limen::AttachmentStoreOperation::Store;
                passSpec.ClearColor = {0.08f, 0.08f, 0.1f, 1.0f};
                passSpec.DebugName = "Renderer2D Scene Pass Test";

                Limen::RenderPass scenePass(passSpec);

                Limen::FramebufferSpecification displaySpec;
                displaySpec.Width = 16;
                displaySpec.Height = 16;
                displaySpec.Samples = 1;
                displaySpec.Attachments = {Limen::FramebufferAttachmentFormat::RGBA8};

                auto displayFramebuffer = Limen::Framebuffer::Create(displaySpec);
                Limen::Scope<Limen::RenderPass> displayPass;

                if (!displayFramebuffer)
                {
                    std::cerr << "Display framebuffer creation failed\n";
                } else
                {
                    Limen::RenderPassSpecification displayPassSpec;
                    displayPassSpec.TargetFramebuffer = displayFramebuffer.get();
                    displayPassSpec.ColorLoadOperation = Limen::AttachmentLoadOperation::Clear;
                    displayPassSpec.ColorStoreOperation = Limen::AttachmentStoreOperation::Store;
                    displayPassSpec.DepthLoadOperation = Limen::AttachmentLoadOperation::DontCare;
                    displayPassSpec.DepthStoreOperation = Limen::AttachmentStoreOperation::DontCare;
                    displayPassSpec.StencilLoadOperation = Limen::AttachmentLoadOperation::DontCare;
                    displayPassSpec.StencilStoreOperation = Limen::AttachmentStoreOperation::DontCare;
                    displayPassSpec.ClearColor = {0.0f, 1.0f, 0.0f, 1.0f};
                    displayPassSpec.DebugName = "Renderer2D Display Pass Test";

                    displayPass = Limen::CreateScope<Limen::RenderPass>(displayPassSpec);
                }

                Limen::ShaderLibrary shaderLibrary;
                auto displayShader = shaderLibrary.Load("Renderer2D/DisplayEncode");

                if (!displayShader)
                {
                    std::cerr << "Display shader load failed\n";
                } else
                {
                    displayShader->Bind();

                    GLint activeProgram = 0;
                    glGetIntegerv(GL_CURRENT_PROGRAM, &activeProgram);

                    const GLint sceneColorLocation = activeProgram != 0
                        ? glGetUniformLocation(
                            static_cast<GLuint>(activeProgram), "u_SceneColor"
                          )
                        : -1;

                    if (sceneColorLocation >= 0)
                        displayShader->SetInt("u_SceneColor", 0);

                    displayShader->UnBind();

                    const GLenum shaderSetupError = glGetError();
                    displayShaderReady =
                        activeProgram != 0 &&
                        sceneColorLocation >= 0 &&
                        shaderSetupError == GL_NO_ERROR;

                    if (!displayShaderReady)
                    {
                        std::cerr << "Display shader setup failed: program="
                                  << activeProgram
                                  << ", sampler location=" << sceneColorLocation
                                  << ", GL error=" << shaderSetupError << '\n';
                    }
                }

                Limen::Ref<Limen::GraphicsPipeline> displayPipeline;
                if (displayShaderReady)
                {
                    Limen::GraphicsPipelineSpecification displayPipelineSpec;
                    displayPipelineSpec.ShaderProgram = displayShader;
                    displayPipelineSpec.Blend = Limen::BlendMode::Opaque;
                    displayPipelineSpec.Topology = Limen::PrimitiveTopology::TriangleList;
                    displayPipelineSpec.Culling = Limen::CullMode::None;
                    displayPipelineSpec.DepthWriteEnabled = false;
                    displayPipelineSpec.DepthTestEnabled = false;
                    displayPipelineSpec.FrontFaceWinding = Limen::FrontFace::CounterClockwise;
                    displayPipelineSpec.DebugName = "Renderer2D Display Pipeline Test";

                    displayPipeline = Limen::GraphicsPipeline::Create(displayPipelineSpec);
                    if (displayPipeline)
                    {
                        const auto &createdSpec = displayPipeline->GetSpecification();
                        displayPipelineReady =
                            createdSpec.ShaderProgram == displayShader &&
                            createdSpec.Blend == Limen::BlendMode::Opaque &&
                            createdSpec.Topology == Limen::PrimitiveTopology::TriangleList &&
                            createdSpec.Culling == Limen::CullMode::None &&
                            !createdSpec.DepthWriteEnabled &&
                            !createdSpec.DepthTestEnabled &&
                            createdSpec.FrontFaceWinding == Limen::FrontFace::CounterClockwise;
                    }

                    if (!displayPipelineReady)
                        std::cerr << "Display pipeline creation or specification failed\n";
                }

                Limen::Scope<Limen::VertexArray> displayVertexArray;
                if (displayPipelineReady)
                {
                    displayVertexArray.reset(Limen::VertexArray::Create());
                    if (!displayVertexArray)
                    {
                        std::cerr << "Display vertex array creation failed\n";
                    } else
                    {
                        // Core Profile 的索引缓冲绑定属于 VAO 状态，须先绑定 VAO。
                        displayVertexArray->Bind();
                        constexpr uint32_t fullscreenIndices[] = {0, 1, 2};
                        Limen::Ref<Limen::IndexBuffer> indexBuffer;
                        indexBuffer.reset(Limen::IndexBuffer::Create(fullscreenIndices, 3));
                        if (indexBuffer)
                            displayVertexArray->SetIndexBuffer(indexBuffer);

                        GLint boundVertexArray = 0;
                        GLint boundIndexBuffer = 0;
                        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVertexArray);
                        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &boundIndexBuffer);
                        displayVertexArray->UnBind();

                        const GLenum geometryError = glGetError();
                        const auto &attachedIndexBuffer = displayVertexArray->GetIndexBuffer();
                        displayGeometryReady =
                            attachedIndexBuffer &&
                            attachedIndexBuffer->GetCount() == 3 &&
                            boundVertexArray != 0 &&
                            boundIndexBuffer != 0 &&
                            geometryError == GL_NO_ERROR;

                        if (!displayGeometryReady)
                        {
                            std::cerr << "Display fullscreen geometry setup failed: GL error "
                                      << geometryError << '\n';
                        }
                    }
                }

                Limen::Renderer::BeginFrame();
                scenePass.Begin();

                if (scenePass.IsActive())
                {
                    Limen::OrthoGraphicCamera camera(-1.0f, 1.0f, -1.0f, 1.0f);

                    Limen::Renderer2D::BeginScene(camera);
                    Limen::Renderer2D::DrawQuad(
                        glm::vec3(0.0f, 0.0f, 0.0f),
                        glm::vec2(1.0f, 1.0f),
                        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)
                    );
                    if (testTexture)
                    {
                        Limen::Renderer2D::DrawQuad(
                            glm::vec3(0.75f, 0.0f, 0.0f),
                            glm::vec2(0.5f, 1.0f),
                            testTexture
                        );
                    }
                    Limen::Renderer2D::EndScene();

                    scenePass.End();
                    scenePassReady = !scenePass.IsActive();
                } else
                {
                    std::cerr << "Scene pass did not begin\n";
                }

                if (displayPass && scenePassReady &&
                    displayPipelineReady && displayGeometryReady)
                {
                    displayPass->Begin();
                    if (displayPass->IsActive())
                    {
                        sceneFramebuffer->BindColorAttachment(0);
                        Limen::Renderer::SubmitFullscreen(
                            *displayPipeline, *displayVertexArray
                        );

                        const GLenum drawError = glGetError();
                        displayDrawReady = drawError == GL_NO_ERROR;
                        if (!displayDrawReady)
                            std::cerr << "Display draw OpenGL error: " << drawError << '\n';

                        displayPass->End();
                        displayPassReady = !displayPass->IsActive();
                    } else
                    {
                        std::cerr << "Display pass did not begin\n";
                    }
                }

                Limen::Renderer::EndFrame();
                if (scenePassReady)
                {
                    GLint previousActiveTexture = 0;
                    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
                    glActiveTexture(GL_TEXTURE0);

                    GLint previousTextureBinding = 0;
                    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTextureBinding);

                    GLint previousPackBuffer = 0;
                    GLint previousPackAlignment = 0;
                    GLint previousPackRowLength = 0;
                    GLint previousPackSkipPixels = 0;
                    GLint previousPackSkipRows = 0;
                    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPackBuffer);
                    glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
                    glGetIntegerv(GL_PACK_ROW_LENGTH, &previousPackRowLength);
                    glGetIntegerv(GL_PACK_SKIP_PIXELS, &previousPackSkipPixels);
                    glGetIntegerv(GL_PACK_SKIP_ROWS, &previousPackSkipRows);

                    // 测试专用同步读回。PBO 必须为 0，pixels 才是 CPU 地址。
                    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
                    glPixelStorei(GL_PACK_ALIGNMENT, 1);
                    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
                    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
                    glPixelStorei(GL_PACK_SKIP_ROWS, 0);

                    sceneFramebuffer->BindColorAttachment(0);
                    GLubyte pixels[16 * 16 * 4]{};
                    glGetTexImage(
                        GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels
                    );
                    const GLenum readError = glGetError();

                    GLubyte displayPixels[16 * 16 * 4]{};
                    GLenum displayReadError = GL_NO_ERROR;

                    if (displayPassReady)
                    {
                        displayFramebuffer->BindColorAttachment(0);
                        glGetTexImage(
                            GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, displayPixels
                        );
                        displayReadError = glGetError();
                    }

                    glBindTexture(
                        GL_TEXTURE_2D, static_cast<GLuint>(previousTextureBinding)
                    );
                    glActiveTexture(static_cast<GLenum>(previousActiveTexture));
                    glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
                    glPixelStorei(GL_PACK_ROW_LENGTH, previousPackRowLength);
                    glPixelStorei(GL_PACK_SKIP_PIXELS, previousPackSkipPixels);
                    glPixelStorei(GL_PACK_SKIP_ROWS, previousPackSkipRows);
                    glBindBuffer(
                        GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(previousPackBuffer)
                    );

                    if (readError != GL_NO_ERROR)
                    {
                        std::cerr << "Resolve readback OpenGL error: "
                                << readError << '\n';
                    } else
                    {
                        // 左下角不被中心 Quad 覆盖，仍应保留 Scene Pass 的清屏颜色。
                        const int red = pixels[0];
                        const int green = pixels[1];
                        const int blue = pixels[2];
                        const int alpha = pixels[3];

                        // Linear RGBA8：0.08、0.08、0.10、1.0 量化后约为 20、20、26、255。
                        resolvedBackgroundCorrect =
                                red >= 19 && red <= 21 &&
                                green >= 19 && green <= 21 &&
                                blue >= 25 && blue <= 27 &&
                                alpha == 255;

                        if (!resolvedBackgroundCorrect)
                        {
                            std::cerr << "Resolve background expected about "
                                    << "(20, 20, 26, 255), got ("
                                    << red << ", " << green << ", "
                                    << blue << ", " << alpha << ")\n";
                        }

                        // 中心像素位于 Quad 内部，验证实际绘制结果也经过了 Resolve。
                        constexpr int center = (8 * 16 + 8) * 4;
                        const int quadRed = pixels[center + 0];
                        const int quadGreen = pixels[center + 1];
                        const int quadBlue = pixels[center + 2];
                        const int quadAlpha = pixels[center + 3];

                        resolvedQuadCorrect =
                                quadRed == 255 &&
                                quadGreen == 0 &&
                                quadBlue == 0 &&
                                quadAlpha == 255;
                        constexpr int textureCenter = (8 * 16 + 13) * 4;
                        const int textureRed = pixels[textureCenter + 0];
                        const int textureGreen = pixels[textureCenter + 1];
                        const int textureBlue = pixels[textureCenter + 2];
                        const int textureAlpha = pixels[textureCenter + 3];

                        // sRGB 输入 128、64 经 GPU 解码并写入线性 RGBA8 后约为 55、13。
                        resolvedTextureCorrect =
                                textureRed >= 53 && textureRed <= 57 &&
                                textureGreen >= 11 && textureGreen <= 15 &&
                                textureBlue == 0 &&
                                textureAlpha == 255;

                        if (!resolvedTextureCorrect)
                        {
                            std::cerr << "Resolve texture expected about (55, 13, 0, 255), got ("
                                    << textureRed << ", " << textureGreen << ", "
                                    << textureBlue << ", " << textureAlpha << ")\n";
                        }
                    }

                    if (displayPassReady)
                    {
                        if (displayReadError != GL_NO_ERROR)
                        {
                            std::cerr << "Display readback OpenGL error: "
                                    << displayReadError << '\n';
                        } else
                        {
                            const auto checkDisplayPixel = [&displayPixels](
                                const char *label,
                                const int offset,
                                const int expectedRed,
                                const int expectedGreen,
                                const int expectedBlue
                            ) {
                                const int red = displayPixels[offset + 0];
                                const int green = displayPixels[offset + 1];
                                const int blue = displayPixels[offset + 2];
                                const int alpha = displayPixels[offset + 3];
                                const auto near = [](const int actual, const int expected) {
                                    return actual >= expected - 2 && actual <= expected + 2;
                                };
                                const bool correct =
                                    near(red, expectedRed) &&
                                    near(green, expectedGreen) &&
                                    near(blue, expectedBlue) &&
                                    alpha == 255;

                                if (!correct)
                                {
                                    std::cerr << label << " expected about ("
                                              << expectedRed << ", " << expectedGreen << ", "
                                              << expectedBlue << ", 255), got ("
                                              << red << ", " << green << ", "
                                              << blue << ", " << alpha << ")\n";
                                }
                                return correct;
                            };

                            // 线性 Scene 颜色经 DisplayEncode 编码；清屏绿色应被全屏绘制覆盖。
                            displayBackgroundCorrect = checkDisplayPixel(
                                "Display background", 0, 79, 79, 90
                            );
                            displayQuadCorrect = checkDisplayPixel(
                                "Display red quad", (8 * 16 + 8) * 4, 255, 0, 0
                            );
                            displayTextureCorrect = checkDisplayPixel(
                                "Display texture quad", (8 * 16 + 13) * 4, 128, 64, 0
                            );
                        }
                    }
                }
            }
        }

        // 上面的作用域已先销毁 RenderPass，再销毁它借用的 Framebuffer。
        Limen::Renderer::Shutdown();


        const GLenum error = glGetError();
        if (error != GL_NO_ERROR)
            std::cerr << "Renderer2D pipeline OpenGL error: " << error << '\n';

        success = scenePassReady &&
           resolvedBackgroundCorrect &&
           resolvedQuadCorrect &&
           resolvedTextureCorrect &&
           displayPassReady &&
           displayDrawReady &&
           displayBackgroundCorrect &&
           displayQuadCorrect &&
           displayTextureCorrect &&
           displayShaderReady &&
           displayPipelineReady &&
           displayGeometryReady &&
           error == GL_NO_ERROR;
        if (success)
            std::cout << "Renderer2D Scene/Display pipeline: PASS\n";
    }

    glfwMakeContextCurrent(nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    return success ? 0 : 1;
}
