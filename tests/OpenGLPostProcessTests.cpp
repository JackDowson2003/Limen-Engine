#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

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