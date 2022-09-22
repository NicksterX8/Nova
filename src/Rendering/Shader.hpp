#ifndef SHADER_H
#define SHADER_H
  
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "../sdl_gl.hpp"
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../utils/Log.hpp"

extern GLuint usedShader;

struct Shader {
    GLuint id;

    Shader() : id(0) {}

    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath);

    // activate the shader
    // ------------------------------------------------------------------------
    void use() const { 
        // not sure if this check is actually worth doing.
        // It may depend on the implementation whether it takes a long time to check if a shader program is already being used or not
        // so it could save time to check ourselves.
        if (usedShader != id) {
            glUseProgram(id);
            usedShader = id;
        }
    }

    void destroy() {
        glDeleteProgram(id);
    }

    operator bool() const {
        return (id != 0);
    }

    GLint getLoc(const char* name) const;

    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const char* name, bool value) {         
        glUniform1i(getLoc(name), (GLint)value); 
    }
    // ------------------------------------------------------------------------
    void setInt(const char* name, int value) {
        glUniform1i(getLoc(name), value); 
    }
    // ------------------------------------------------------------------------
    void setFloat(const char* name, float value) { 
        glUniform1f(getLoc(name), value); 
    }

    void setVec3(const char* name, glm::vec3 vec3) {
        glUniform3f(getLoc(name), vec3.x, vec3.y, vec3.z);
    }

    void setVec2(const char* name, glm::vec2 vec2) {
        glUniform2f(getLoc(name), vec2.x, vec2.y);
    }

    void setMat4(const char* name, const glm::mat4& mat4) {
        glUniformMatrix4fv(getLoc(name), 1, GL_FALSE, glm::value_ptr(mat4));
    }

    // getters

    GLint getInt(const char* name) const {
        GLint value;
        glGetUniformiv(id, getLoc(name), &value);
        return value;
    }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, const char* type) {
        int success;
        int infoLogLenth;
        char infoLog[1024];
        if (strcmp(type, "Program") != 0) {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                LogError("Failed to compile %s shader! Shader info log: %s\n -------------------------------------------------", type, infoLog);
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                LogError("Failed to link shader program! Program info log: %s\n -------------------------------------------------", infoLog);
            }
        }
    }
};

#endif