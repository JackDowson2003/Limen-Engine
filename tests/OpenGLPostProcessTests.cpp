#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

std::string ReadShaderFile(const char* path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        std::cerr << "Cannot open shader: " << path << '\n';
        return {};
    }

    std::ostringstream content;
    content << input.rdbuf();

    if (input.bad())
    {
        std::cerr << "Cannot read shader: " << path << '\n';
        return {};
    }

    return content.str();
}

GLuint CompileShaderStage(GLenum stage, const std::string& source)
{
    const GLuint shader = glCreateShader(stage);
    if (shader == 0)
    {
        std::cerr << "Cannot create shader object\n";
        return 0;
    }

    const char* sourceText = source.c_str();
    glShaderSource(shader, 1, &sourceText, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE)
        return shader;

    GLchar log[2048]{};
    glGetShaderInfoLog(
        shader, static_cast<GLsizei>(sizeof(log)), nullptr, log
    );
    std::cerr << "Shader compilation failed ("
              << (stage == GL_VERTEX_SHADER ? "vertex" : "fragment")
              << "): " << log << '\n';

    glDeleteShader(shader);
    return 0;
}

GLuint LinkPostProcess(
    const std::string& vertexSource,
    const std::string& fragmentSource
)
{
    const GLuint vertex = CompileShaderStage(
        GL_VERTEX_SHADER, vertexSource
    );
    if (vertex == 0)
        return 0;

    const GLuint fragment = CompileShaderStage(
        GL_FRAGMENT_SHADER, fragmentSource
    );
    if (fragment == 0)
    {
        glDeleteShader(vertex);
        return 0;
    }

    const GLuint program = glCreateProgram();
    if (program == 0)
    {
        std::cerr << "Cannot create shader program\n";
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE)
    {
        GLchar log[2048]{};
        glGetProgramInfoLog(
            program, static_cast<GLsizei>(sizeof(log)), nullptr, log
        );
        std::cerr << "PostProcess linking failed: " << log << '\n';
    }

    glDetachShader(program, vertex);
    glDetachShader(program, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (linked != GL_TRUE)
    {
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

bool CheckPixel(
    const char* label,
    const GLubyte pixel[4],
    int expectedRed,
    int expectedGreen,
    int expectedBlue
)
{
    const int expected[4]{expectedRed, expectedGreen, expectedBlue, 255};
    bool passed = true;
    for (int channel = 0; channel < 4; ++channel)
    {
        // RGB 允许量化/驱动舍入误差；不透明 Alpha 必须精确为 255。
        const int tolerance = channel == 3 ? 0 : 2;
        const int difference = static_cast<int>(pixel[channel]) - expected[channel];
        if (std::abs(difference) > tolerance)
            passed = false;
    }

    if (passed)
    {
        std::cout << label << ": PASS\n";
        return true;
    }

    std::cerr << label << ": expected ("
              << expected[0] << ", " << expected[1] << ", "
              << expected[2] << ", " << expected[3] << "), got ("
              << static_cast<int>(pixel[0]) << ", "
              << static_cast<int>(pixel[1]) << ", "
              << static_cast<int>(pixel[2]) << ", "
              << static_cast<int>(pixel[3]) << ")\n";
    return false;
}

bool DrawAndCheckExposure(
    GLint exposureLocation,
    float exposure,
    const char* label,
    int expectedRed,
    int expectedGreen,
    int expectedBlue
)
{
    // 每次绘制前清成透明；漏掉绘制时不能误报 Alpha=255 通过。
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUniform1f(exposureLocation, exposure);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    GLubyte pixel[4]{};
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << label << ": OpenGL error " << error << '\n';
        return false;
    }

    return CheckPixel(
        label, pixel, expectedRed, expectedGreen, expectedBlue
    );
}

bool CheckPostProcessPixels(GLuint program)
{
    GLuint sceneTexture = 0;
    GLuint colorTexture = 0;
    GLuint framebuffer = 0;
    GLuint vertexArray = 0;
    glGenTextures(1, &sceneTexture);
    glGenTextures(1, &colorTexture);
    glGenFramebuffers(1, &framebuffer);
    glGenVertexArrays(1, &vertexArray);

    bool passed = false;
    if (sceneTexture == 0 || colorTexture == 0 ||
        framebuffer == 0 || vertexArray == 0)
    {
        std::cerr << "PostProcess test resource creation failed\n";
    }
    else
    {
        // 已知的线性 HDR 输入；源 Alpha=0.25 可检验最终输出是否强制不透明。
        const GLfloat hdrPixel[4]{3.0f, 1.0f, 0.0f, 0.25f};
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA16F, 1, 1, 0,
            GL_RGBA, GL_FLOAT, hdrPixel
        );

        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, nullptr
        );

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, colorTexture, 0
        );
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "PostProcess RGBA8 framebuffer incomplete\n";
        }
        else
        {
            const GLint sceneLocation = glGetUniformLocation(
                program, "u_SceneColor"
            );
            const GLint exposureLocation = glGetUniformLocation(
                program, "u_Exposure"
            );
            if (sceneLocation < 0 || exposureLocation < 0)
            {
                std::cerr << "PostProcess uniforms unavailable\n";
            }
            else
            {
                glViewport(0, 0, 1, 1);
                glDisable(GL_BLEND);
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_SCISSOR_TEST);
                glDisable(GL_CULL_FACE);
                glDisable(GL_DITHER);
                glDisable(GL_FRAMEBUFFER_SRGB);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

                glUseProgram(program);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, sceneTexture);
                glUniform1i(sceneLocation, 0);
                // Core Profile 即使只用 gl_VertexID 绘制，也要求绑定 VAO。
                glBindVertexArray(vertexArray);

                // Reinhard + Linear→sRGB 后量化为 RGBA8：3→225，1→188。
                const bool normalExposure = DrawAndCheckExposure(
                    exposureLocation, 1.0f, "exposure 1", 225, 188, 0
                );
                const bool zeroExposure = DrawAndCheckExposure(
                    exposureLocation, 0.0f, "exposure 0", 0, 0, 0
                );
                passed = normalExposure && zeroExposure;
            }
        }
    }

    // 所有测试资源都由本函数拥有，必须先于 GLFW Context 销毁。
    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (vertexArray != 0)
        glDeleteVertexArrays(1, &vertexArray);
    if (framebuffer != 0)
        glDeleteFramebuffers(1, &framebuffer);
    if (colorTexture != 0)
        glDeleteTextures(1, &colorTexture);
    if (sceneTexture != 0)
        glDeleteTextures(1, &sceneTexture);
    return passed;
}

int main()
{
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

    GLFWwindow* window = glfwCreateWindow(
        16, 16, "Limen OpenGL test", nullptr, nullptr
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
    }
    else
    {
        GLint major = 0;
        GLint minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);

        success = major > 4 || (major == 4 && minor >= 1);
        if (success)
            std::cout << "OpenGL " << major << '.' << minor
                      << " context ready\n";
        else
            std::cerr << "OpenGL 4.1 required; got "
                      << major << '.' << minor << '\n';

        if (success)
        {
            const std::string vertexSource = ReadShaderFile(
                LIMEN_POSTPROCESS_VERT_PATH
            );
            const std::string fragmentSource = ReadShaderFile(
                LIMEN_POSTPROCESS_FRAG_PATH
            );

            if (vertexSource.empty() || fragmentSource.empty())
            {
                std::cerr << "PostProcess shader source is empty\n";
                success = false;
            }
            else
            {
                const GLuint program = LinkPostProcess(
                    vertexSource, fragmentSource
                );
                success = program != 0;

                if (success)
                {
                    std::cout << "PostProcess shader linked\n";
                    success = CheckPostProcessPixels(program);
                    glDeleteProgram(program);
                }
            }
        }
    }

    // 今后若创建 GPU 资源，须先释放资源，再销毁其 Context。
    glfwMakeContextCurrent(nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    return success ? 0 : 1;
}
