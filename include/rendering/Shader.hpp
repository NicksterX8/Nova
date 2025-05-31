#ifndef SHADER_H
#define SHADER_H

#include "../sdl_gl.hpp"
//#include <glm/mat4x4.hpp>
#include <glm/detail/type_mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../utils/Log.hpp"

extern GLuint usedShader;

// read the contents of the file to a buffer that must be freed
char* readFileContents(FILE* file, size_t* outBytesRead=NULL);
// wrapper over readFileContents that opens and closes a file 
char* readFileContents(const char* filename, size_t* outBytesRead=NULL);

GLuint compileShader(GLenum shaderType, const char* sourcePath);

struct ShaderInfo {
    GLuint id = 0;
    std::string filename;

    ShaderInfo() {}

    ShaderInfo(GLuint id, const std::string& filename) : id(id), filename(filename) {}
};

struct ShaderProgram {
    GLuint programID = 0;

    // shaders
    ShaderInfo vertex;
    ShaderInfo fragment;
    ShaderInfo geometry;

    void destroy() {
        glDeleteShader(vertex.id);
        glDeleteShader(fragment.id);
        glDeleteShader(geometry.id);
        glDeleteProgram(programID);
    }
};

// By default generates new shader program, while if a program is provided the shaders will be linked to that program instead
ShaderProgram loadShaderProgram(const char* vertexPath, const char* fragmentPath, const char* geometryPath=nullptr, GLuint program = 0);

// An OpenGL Shader program wrapper/abstraction
struct Shader {
    GLuint id;

    Shader() : id(0) {}

    Shader(GLuint program) : id(program) {}

    void destroy() {
        glDeleteProgram(id);
    }

    // activate the shader
    // ------------------------------------------------------------------------
    void use() const { 
        if (id == 0) {
            LogError("Invalid shader");
            return;
        }

        // not sure if this check is actually worth doing.
        // It may depend on the implementation whether it takes a long time to check if a shader program is already being used or not
        // so it could save time to check ourselves.
        if (usedShader != id) {
            glUseProgram(id);
            usedShader = id;
        }
    }

    operator bool() const {
        return (id != 0);
    }

    GLint getUniformLocation(const char* name) const;

    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const char* name, bool value) {         
        glUniform1i(getUniformLocation(name), (GLint)value); 
    }
    // ------------------------------------------------------------------------
    void setInt(const char* name, int value) {
        glUniform1i(getUniformLocation(name), value); 
    }
    // ------------------------------------------------------------------------
    void setFloat(const char* name, float value) { 
        glUniform1f(getUniformLocation(name), value); 
    }

    void setVec2(const char* name, glm::vec2 vec2) {
        glUniform2f(getUniformLocation(name), vec2.x, vec2.y);
    }
    
    void setVec3(const char* name, glm::vec3 vec3) {
        glUniform3f(getUniformLocation(name), vec3.x, vec3.y, vec3.z);
    }

    void setVec4(const char* name, glm::vec4 vec4) {
        glUniform4f(getUniformLocation(name), vec4.x, vec4.y, vec4.z, vec4.w);
    }

    void setMat4(const char* name, const glm::mat4& mat4) {
        glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat4));
    }

    // getters

    GLint getInt(const char* name) const {
        GLint value;
        glGetUniformiv(id, getUniformLocation(name), &value);
        return value;
    }
};

struct ShaderData {
    ShaderProgram program;
    std::string name;
};

inline ShaderData loadShader(const char* name) {
    if (!name) return {};
    auto basePath = FileSystem.shaders.get(name);
    auto geometryPath = str_add(basePath, ".gs");
    auto geometryFile = SDL_RWFromFile(geometryPath, "r");
    ShaderProgram program = loadShaderProgram(str_add(basePath, ".vs"), str_add(basePath, ".fs"), geometryFile ? geometryPath : nullptr);
    return {program, std::string(name)};
}

int reloadShaderProgram(ShaderProgram* program);

#endif