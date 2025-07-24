#include "rendering/Shader.hpp"
// for debug names
#include "rendering/shaders.hpp"

GLuint usedShader = 0;

GLuint compileShader(GLenum shaderType, const char* sourcePath) {
    const char* shaderName;
    switch (shaderType) {
    case GL_VERTEX_SHADER:
        shaderName = "vertex";
        break;
    case GL_FRAGMENT_SHADER:
        shaderName = "fragment";
        break;
    case GL_GEOMETRY_SHADER:
        shaderName = "geometry";
        break;
    default:
        shaderName = "";
        break;
    }

    if (!sourcePath) {
        LogError("Invalid %s shader path (null)", shaderName);
        return 0;
    }

    size_t sourceLength;
    char* source = (char*)SDL_LoadFile(sourcePath, &sourceLength);
    if (!source) {
        LogError("Failed to read %s shader file. Path: %s", shaderName, sourcePath);
        return 0;
    }

    GLuint shader = glCreateShader(shaderType);
    GLint iSourceLength = sourceLength;
    glShaderSource(shader, 1, &source, &iSourceLength);
    glCompileShader(shader);
    SDL_free(source); // no longer needed after giving it to opengl
    // check for compilation errors
    GLint success;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        LogError("Failed to compile %s shader!\nFile: %s . Shader info log: %s\n -------------------------------------------------", shaderName, sourcePath, infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

ShaderProgram loadShaderProgram(const char* vertexPath, const char* fragmentPath, const char* geometryPath, GLuint id) {
    // vertex and fragment shaders are required, geometry is optional

    ShaderProgram program = {.programID = id, .vertex = ShaderInfo(0, vertexPath), .fragment = ShaderInfo(0, fragmentPath), .geometry = ShaderInfo(0, geometryPath ? geometryPath : std::string{})};

    program.vertex.id = compileShader(GL_VERTEX_SHADER, vertexPath);
    if (!program.vertex.id) {
        LogError("Shader program compilation failed while compiling vertex shader.");
        return program;
    }

    program.fragment.id = compileShader(GL_FRAGMENT_SHADER, fragmentPath);
    if (!program.fragment.id) {
        LogError("Shader program compilation failed while compiling fragment shader.");
        return program;
    }

    if (geometryPath) {
        program.geometry.id = compileShader(GL_GEOMETRY_SHADER, geometryPath);
        if (!program.geometry.id) {
            LogError("Shader program compilation failed while compiling geometry shader.");
            return program;
        }
    }

    if (!id) id = glCreateProgram();
    program.programID = id;

    glAttachShader(id, program.vertex.id);
    glAttachShader(id, program.fragment.id);
    if (program.geometry.id)
        glAttachShader(id, program.geometry.id);
    glLinkProgram(id);
    GLint success;
    char infoLog[1024];
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 1024, NULL, infoLog);
        LogError("Failed to link shader program!\nVertex shader file: %s\nProgram info log: %s\n -------------------------------------------------", vertexPath, infoLog);
        glDeleteShader(program.vertex.id);
        glDeleteShader(program.fragment.id);
        glDeleteShader(program.geometry.id);
        glDeleteProgram(id);
        program.programID = 0;
    }

    return program;
}

int reloadShaderProgram(ShaderProgram* program) {
    if (!program) return -1;

    auto geometryFilename = program->geometry.filename;
    ShaderProgram newProgram = loadShaderProgram(program->vertex.filename.c_str(), program->fragment.filename.c_str(), !geometryFilename.empty() ? geometryFilename.c_str() : nullptr);
    if (newProgram.programID == 0) {
        LogError("Failed to reload shader!");
        return -1;
    } else {
        program->destroy();
        *program = newProgram;
        return 0;
    }
}

GLint Shader::getUniformLocation(const char* name) const {
    if (!this->id) {
        LogError("Attempted to use uninitialized shader!");
        return 0;
    }
    
    if (this->id != usedShader) {
        LogWarn("Used shader %d before calling shader.use()!. Switching shaders...", this->id);
        this->use();
    }

    GLint loc = glGetUniformLocation(id, name);
    if (loc == -1) {
        LogOnce(Warn, "Shader::%s - Failed to get uniform location for \"%s\"", getShaderName(getShaderIDFromProgramID(this->id)), name);
    }
    return loc;
}