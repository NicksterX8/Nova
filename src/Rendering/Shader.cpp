#include "Shader.hpp"

GLuint usedShader = 0;

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // open files
    vShaderFile.open(vertexPath);
    fShaderFile.open(fragmentPath);
    if (!vShaderFile) {
        LogError("Failed to open vertex shader for reading. Path: %s", vertexPath);
        LogError("str error: %s", strerror(errno));
        return;
    }
    if (!fShaderFile) {
        LogError("Failed to open fragment shader for reading. Path: %s", fragmentPath);
        return;
    }

    std::stringstream vShaderStream, fShaderStream;
    // read file's buffer contents into streams
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();
    // close file handlers
    vShaderFile.close();
    fShaderFile.close();
    // convert stream into string
    vertexCode   = vShaderStream.str();
    fragmentCode = fShaderStream.str();

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    // 2. compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "Vertex");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "Fragment");
    // shader Program
    id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    checkCompileErrors(id, "Program");
    // delete the shaders as they're linked into our program now and are no longer needed
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

GLint Shader::getLoc(const char* name) const {
    if (!id) {
        LogError("Attempted to use uninitialized shader!");
        return 0;
    }
    
    if (id != usedShader) {
        LogError("Attempted to use shader %d before calling shader.use()!. Switching shaders...", id);
        use();
    }

    auto loc = glGetUniformLocation(id, name);
    if (loc == -1) {
        LogError("Failed to get uniform location for \"%s\"", name);
    }
    return loc;
}