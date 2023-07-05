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
GLuint makeShaderProgram(const char* vertexPath, const char* fragmentPath);
GLuint makeShaderProgram(const char* vertexPath, const char* fragmentPath, const char* geometryPath);


// An OpenGL Shader program wrapper/abstraction
struct Shader {
    GLuint id;

    Shader() : id(0) {}

    // constructor generates and compiles the shader program 
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath);

    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath);

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

    void setVec3(const char* name, glm::vec3 vec3) {
        glUniform3f(getUniformLocation(name), vec3.x, vec3.y, vec3.z);
    }

    void setVec2(const char* name, glm::vec2 vec2) {
        glUniform2f(getUniformLocation(name), vec2.x, vec2.y);
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

inline Shader loadShader(const char* name) {
    // check if geometry shader file is there 
    auto geometryShader = FileSystem.shaders.get(str_add(name, ".gs"));
    auto file = SDL_RWFromFile(geometryShader, "r");
    if (file) {
        // if so, include it in the shader program
        SDL_RWclose(file);
        return Shader(makeShaderProgram(FileSystem.shaders.get(str_add(name, ".vs")), FileSystem.shaders.get(str_add(name, ".fs")), geometryShader));
    }
    return Shader(makeShaderProgram(FileSystem.shaders.get(str_add(name, ".vs")), FileSystem.shaders.get(str_add(name, ".fs"))));
}

#endif