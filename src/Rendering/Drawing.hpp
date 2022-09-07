#ifndef DRAWING_INCLUDED
#define DRAWING_INCLUDED

#include <SDL2/SDL.h>
#include "../GameState.hpp"
#include "../sdl.hpp"
#include "../Camera.hpp"
#include "utils.hpp"

namespace Draw {

    struct ColorVertex {
        glm::vec3 position;
        glm::vec4 color;
    };

    struct ColorVertex2D {
        glm::vec2 position;
        glm::vec4 color;
    };

    struct ColorQuadRenderBuffer {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;

        using Vertex = ColorVertex;
        using Vertex2D = ColorVertex2D;

        Vertex* vertexBuffer;

        inline Vertex* getBufferTop() {
            return &vertexBuffer[storedQuads * 4];
        }

        struct UniformQuad {
            glm::vec3 positions[4];
            glm::vec4 color;
        };

        struct UniformQuad2D {
            glm::vec2 positions[4];
            glm::vec4 color;
        };

        const GLuint maxQuadsPerBatch = 10000;
        const GLuint maxVerticesPerBatch = maxQuadsPerBatch*4;
        const GLuint eboIndexCount = maxQuadsPerBatch*6;

        GLuint storedQuads;

        ColorQuadRenderBuffer() {
            storedQuads = 0;

            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, maxVerticesPerBatch * sizeof(Vertex), NULL, GL_STREAM_DRAW);
            vertexBuffer = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboIndexCount * sizeof(GLuint), NULL, GL_STATIC_DRAW);
            GLuint* indices = static_cast<GLuint*>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

            for (GLuint i = 0; i < maxQuadsPerBatch; i++) {
                GLuint first = i*4;
                GLuint ind = i * 6;
                indices[ind+0] = first+0;
                indices[ind+1] = first+1;
                indices[ind+2] = first+3;
                indices[ind+3] = first+1;
                indices[ind+4] = first+2;
                indices[ind+5] = first+3; 
            }

            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

            const size_t stride = 7 * sizeof(GLfloat);

            // position
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
            glEnableVertexAttribArray(0);
            // color
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
            glEnableVertexAttribArray(1);
        }

        inline GLuint needBufferSpace(GLuint count) {
            if (storedQuads + count > maxQuadsPerBatch) {
                flush();
                return (count < maxQuadsPerBatch) ? count : maxQuadsPerBatch;
            }
            return count;
        }

        // fastest
        GLuint buffer(GLuint quadCount, const Vertex* quads) {
            quadCount = needBufferSpace(quadCount);
    
            memcpy(getBufferTop(), quads, quadCount * 4 * sizeof(Vertex));

            storedQuads += quadCount;
            return quadCount;
        }

        GLuint buffer(GLuint quadCount, const Vertex2D* quads, float z) {
            quadCount = needBufferSpace(quadCount);
    
            Vertex* top = getBufferTop();
            for (GLuint i = 0; i < quadCount*4; i++) {
                top[i] = {glm::vec3(quads[i].position, z), quads[i].color};
            }

            storedQuads += quadCount;
            return quadCount;
        }

        // slower to buffer
        GLuint bufferSOA(GLuint quadCount, const glm::vec3* vertexPositions, const glm::vec4* vertexColors) {
            quadCount = needBufferSpace(quadCount);

            Vertex* top = &vertexBuffer[storedQuads * 4];
            for (GLuint i = 0; i < quadCount*4; i++) {
                memcpy(&top[i].position, &vertexPositions[i], 3 * sizeof(GLfloat));
            }
            for (GLuint i = 0; i < quadCount*4; i++) {
                memcpy(&top[i].color, &vertexColors[i], 4 * sizeof(GLfloat));
            }

            storedQuads += quadCount;
            return quadCount;
        }

        GLuint bufferUniform(GLuint quadCount, const UniformQuad* quads) {
            quadCount = needBufferSpace(quadCount);

            Vertex* top = getBufferTop();
            for (GLuint i = 0; i < quadCount*4; i++) {
                top[i].position = quads[i/4].positions[i%4];
                top[i].color = quads[i/4].color;
            }

            storedQuads += quadCount;
            return quadCount;
        }

        GLuint bufferUniform(GLuint quadCount, const UniformQuad2D* quads, float z) {
            quadCount = needBufferSpace(quadCount);

            Vertex* top = getBufferTop();
            for (GLuint i = 0; i < quadCount*4; i++) {
                top[i].position = glm::vec3(quads[i/4].positions[i%4], z);
                top[i].color = quads[i/4].color;
            }

            storedQuads += quadCount;
            return quadCount;
        }

        
        Vertex* bufferManual(GLuint quadCount) {
            GLuint available = needBufferSpace(quadCount);
            if (available < quadCount) {
                return NULL;
            }

            Vertex* top = getBufferTop();
            storedQuads += quadCount;
            return top;
        }

        void flush() {
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glUnmapBuffer(GL_ARRAY_BUFFER);

            glDrawElements(GL_TRIANGLES, 6 * storedQuads, GL_UNSIGNED_INT, 0);

            storedQuads = 0;
            vertexBuffer = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        }

        void destroy() {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
        }
    };

    int chunkBorders(ColorQuadRenderBuffer& renderer, Camera& camera, glm::vec4 color, float pixelLineWidth, float z);

    struct ColoredPoint {
        glm::vec3 position;
        glm::vec4 color;
        float size;
    };

    static_assert(sizeof(ColorVertex) == sizeof(GLfloat) * 7, "no struct padding");
    static_assert(sizeof(ColoredPoint) == sizeof(GLfloat) * 8, "no struct padding");

    struct buffersVAOVBO {
        GLuint vao;
        GLuint vbo;
    };

    struct buffersVAOVBOEBO {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
    };

    inline buffersVAOVBO makePointVertexAttribArray() {
        unsigned int vbo,vao;
        glGenBuffers(1, &vbo);
        glGenVertexArrays(1, &vao);

        glBindVertexArray(vao);

        size_t stride = 8 * sizeof(GLfloat);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        // color
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        // point size
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);

        return {vao, vbo};
    }

    inline void coloredPoints(GLuint count, ColoredPoint* points) {
        static auto bufferObjects = makePointVertexAttribArray();

        glBindBuffer(GL_ARRAY_BUFFER, bufferObjects.vbo);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(ColoredPoint), points, GL_STREAM_DRAW);

        glBindVertexArray(bufferObjects.vao);
        glDrawArrays(GL_POINTS, 0, count);
    }

    inline void coloredQuads(ColorQuadRenderBuffer& renderer, GLuint count, ColorVertex* quads) {
       renderer.buffer(count, quads);
    }

    inline void thickLines(ColorQuadRenderBuffer& renderer, GLuint numLines, glm::vec3* points, glm::vec4* colors, GLfloat* lineWidths) {
        ColorVertex* quadPoints = renderer.bufferManual(numLines);
        for (GLuint i = 0; i < numLines; i++) {
            GLuint ind = i * 4;
            glm::vec2 p1 = points[i*2];
            glm::vec2 p2 = points[i*2+1];
            glm::vec2 d = p2 - p1;
            glm::vec2 offset = glm::vec2(-d.y, d.x) * lineWidths[i] / 2.0f;

            for (int j = 0; j < 4; j++) {
                quadPoints[ind+j].color = colors[i];
            }
            
            quadPoints[ind+0].position = {p1 + offset, points[ind+0].z};
            quadPoints[ind+1].position = {p1 - offset, points[ind+0].z};
            quadPoints[ind+2].position = {p2 - offset, points[ind+1].z};
            quadPoints[ind+3].position = {p2 + offset, points[ind+1].z};
        }
    }

    inline void thickLines(ColorQuadRenderBuffer& renderer, GLuint numLines, glm::vec3* points, glm::vec4 color, GLfloat lineWidth) {
        ColorVertex* quadPoints = renderer.bufferManual(numLines);
        for (GLuint i = 0; i < numLines; i++) {
            GLuint ind = i * 4;
            glm::vec2 p1 = points[i*2];
            glm::vec2 p2 = points[i*2+1];
            glm::vec2 d = p2 - p1;
            glm::vec2 offset = glm::normalize(glm::vec2(-d.y, d.x)) * lineWidth / 2.0f;

            for (int j = 0; j < 4; j++) {
                quadPoints[ind+j].color = color;
            }
            
            quadPoints[ind+0].position = {p1 + offset, points[i*2].z};
            quadPoints[ind+1].position = {p1 - offset, points[i*2].z};
            quadPoints[ind+2].position = {p2 - offset, points[i*2+1].z};
            quadPoints[ind+3].position = {p2 + offset, points[i*2+1].z};
        }
    }
    
}

#endif