#ifndef SHADER_H
#define SHADER_H
  
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "../gl.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../Log.hpp"

struct Shader {
    unsigned int id;

    Shader() : id(0) {}

    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath) {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        if (!vShaderFile) {
            Log.Error("Failed to open vertex shader for reading. Path: %s", vertexPath);
            return;
        }
        if (!fShaderFile) {
            Log.Error("Failed to open fragment shader for reading. Path: %s", fragmentPath);
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
    // activate the shader
    // ------------------------------------------------------------------------
    void use() { 
        glUseProgram(id); 
    }

    void destroy() {
        glDeleteProgram(id);
    }

    operator bool() const {
        return (id != 0);
    }

    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const char* name, bool value) const {         
        glUniform1i(glGetUniformLocation(id, name), (GLint)value); 
    }
    // ------------------------------------------------------------------------
    void setInt(const char* name, int value) const { 
        glUniform1i(glGetUniformLocation(id, name), value); 
    }
    // ------------------------------------------------------------------------
    void setFloat(const char* name, float value) const { 
        glUniform1f(glGetUniformLocation(id, name), value); 
    }

    void setVec3(const char* name, float x, float y, float z) {
        glUniform3f(glGetUniformLocation(id, name), x, y, z);
    }

    void setMat4(const char* name, const glm::mat4& mat4) {
        glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, glm::value_ptr(mat4));
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
                Log.Error("Failed to compile %s shader! Shader info log: %s\n -------------------------------------------------", type, infoLog);
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                Log.Error("Failed to link shader program! Program info log: %s\n -------------------------------------------------", infoLog);
            }
        }
    }
};

#endif