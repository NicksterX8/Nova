#include "ECS/EntityWorld.hpp"
#include "gl.hpp"
#include "rendering/utils.hpp"
#include "rendering/context.hpp"
#include "Chunks.hpp"
#include "utils/Debug.hpp"
#include "GameState.hpp"

template<typename T>
constexpr int mySizeof() {
    return (int)sizeof(T);
}

float getLayerHeight(int layer) {
    return (int)layer * 0.05f;
}

struct RenderSystem {
    typedef ECS::EntityQuery<
        ECS::RequireComponents<EC::Render, EC::Size, EC::Position>
    > Query;

    float scale = 1.0f;

    GlModel model;

    const static GLint entitiesPerBatch = 1000;
    const static GLint verticesPerEntity = 1;
    //const static GLint indicesPerEntity = 0;
    const static GLint verticesPerBatch = entitiesPerBatch*verticesPerEntity;
    //const static GLint indicesPerBatch = entitiesPerBatch*indicesPerEntity;
    const static GLint vertexSize = 3 * sizeof(GLfloat) + 2 * sizeof(GLushort) + 1 * sizeof(GLfloat) + 4 * sizeof(GLushort) + 1 * sizeof(GLubyte);
    const static size_t bufferSize = verticesPerBatch * vertexSize;

    RenderSystem() {
        model = GlGenModel();
        model.bindAll();

        glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STREAM_DRAW);
        //glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesPerBatch * sizeof(GLuint), NULL, GL_STATIC_DRAW);

        // for mental model only, not actually used in c++ code at this time


        constexpr GlVertexAttribute attributes[] = {
            {3, GL_FLOAT, sizeof(GLfloat)}, // pos
            {2, GL_FLOAT, sizeof(GLfloat)}, // size
            {1, GL_FLOAT, sizeof(GLfloat)}, // rotation
            {4, GL_UNSIGNED_SHORT, sizeof(GLushort)}, // texCoords (min.x, min.y, max.x, max.y)
            {1, GL_UNSIGNED_BYTE, sizeof(GLubyte)}, // texture
        };
        enableVertexAttribsSOA(&attributes[0], sizeof(attributes) / sizeof(GlVertexAttribute), verticesPerBatch);

        /*
        void* elementBuffer = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        if (!elementBuffer) {
            LogError("Failed to map ebo!");
            return;
        }
        GLuint* indices = static_cast<GLuint*>(elementBuffer);

        for (GLuint i = 0; i < entitiesPerBatch; i++) {
            GLuint ind = i * 6;
            GLuint first = i*4;
            indices[ind+0] = first+0;
            indices[ind+1] = first+1;
            indices[ind+2] = first+3;
            indices[ind+3] = first+1;
            indices[ind+4] = first+2;
            indices[ind+5] = first+3;
        }
        */
    }

    using QueriedEntity = EntityT<EC::Render, EC::Size, EC::Position>;

    template<class EntityQueryT = Query>
    inline void ForEach(const ComponentManager<>& ecs, std::function<void(QueriedEntity entity)> callback) {
        ecs.ForEach<Query>(callback);
    }

    struct VertexDataArrays {
        GLfloat* positions = nullptr;
        GLfloat* sizes = nullptr;
        GLfloat* rotations = nullptr;
        GLushort* texCoords = nullptr;
        GLubyte* textures = nullptr;
    };

    VertexDataArrays mapVertexBuffer() {
        GLvoid* vertexBuffer = (GLvoid*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        if (!vertexBuffer) {
            LogError("Oh no a bunch more bad stuff happened!");
            
            return {};
        }
        VertexDataArrays buffer;
        buffer.positions = (GLfloat*)vertexBuffer;
        buffer.sizes     = (GLfloat*)&buffer.positions[3 * verticesPerBatch];
        buffer.rotations = (GLfloat*)&buffer.sizes[2 * verticesPerBatch];
        buffer.texCoords = (GLushort*)&buffer.rotations[1 * verticesPerBatch];
        buffer.textures  = (GLubyte*)&buffer.texCoords[4 * verticesPerBatch];
        return buffer;
    }

    void unmapVertexBuffer(VertexDataArrays* buffer) {
        assert(buffer);
        assert(buffer->positions);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        *buffer = {};
    }

    void Update(const ComponentManager<EC::Render, const EC::Size, const EC::Position, const EC::Health, const EC::Rotation>& ecs, const ChunkMap& chunkmap, RenderContext& ren, Camera& camera) {
        auto camTransform = camera.getTransformMatrix();

        auto shader = ren.shaders.use(Shaders::Entity);
        shader.setMat4("transform", camTransform);

        glBindVertexArray(model.vao);
        glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
        VertexDataArrays buffer = mapVertexBuffer();
        if (!buffer.positions) {
            // can't render :(
            return;
        }

        auto textureArraySize = glm::vec2(ren.textureArray.size);
        auto* textureData = ren.textures.data.data;

        int entityCounter = 0, entitiesRenderered = 0;
        forEachEntityInBounds(ecs, &chunkmap, 
            camera.position,
            camera.maxCorner() - camera.minCorner(), 
        [&](EntityT<EC::Position> entity){
            if (Query::Check(ecs.EntitySignature(entity))) {
                auto position =  ecs.Get<const EC::Position>(entity)->vec2();
                auto renderEC =  ecs.Get<const EC::Render>(entity);
                auto size     = *ecs.Get<const EC::Size>(entity);

                float rotation = 0.0f;
                if (ecs.EntityHas<EC::Rotation>(entity)) {
                    rotation = ecs.Get<const EC::Rotation>(entity)->degrees;
                }

                TextureID texture = renderEC->texture;
                glm::ivec2 textureSize = textureData[texture].size;
                
                glm::vec<2, GLushort> texMax = textureSize;
                float w = size.width  / 2.0f;
                float h = size.height / 2.0f;
                glm::vec3 p = glm::vec3(position.x, position.y, getLayerHeight(renderEC->layer));
                
                const GLfloat vertexPositions[3] = {
                    p.x,   p.y,   p.z,
                };
                const GLushort vertexTexCoords[4] = {
                    0, 0,
                    texMax.x,  texMax.y, 
                };
                const GLfloat vertexSize[2] = {
                    w, h
                };
                const GLfloat vertexRotation = rotation;
                const GLubyte vertexTexture = renderEC->texture;

                memcpy(&buffer.positions[entityCounter * 3], vertexPositions, 3 * sizeof(GLfloat));
                memcpy(&buffer.texCoords[entityCounter * 4], vertexTexCoords, 4 * sizeof(GLfloat));
                memcpy(&buffer.sizes[entityCounter * 2], vertexSize, sizeof(vertexSize));
                buffer.textures[entityCounter] = vertexTexture;
                buffer.rotations[entityCounter] = vertexRotation;
                
                entityCounter++;
                entitiesRenderered++;

                if (entityCounter == entitiesPerBatch) {
                    unmapVertexBuffer(&buffer); // put vertex data into effect
                    //glDrawElements(GL_TRIANGLES, entityCounter*indicesPerEntity, GL_UNSIGNED_INT, 0); // draw a batch of entities
                    glDrawArrays(GL_POINTS, 0, entityCounter);
                    entityCounter = 0;
                    // have to remap vertex buffer after unmapping
                    buffer = mapVertexBuffer();
                }
            }
        });

        // put vertex data into effect, if the buffer exists
        if (buffer.positions) {
            unmapVertexBuffer(&buffer);
        }

        // number of entities wasn't exactly divisible by entitiesPerBatch, so we have some left to do
        if (entityCounter > 0) {
            //glDrawElements(GL_TRIANGLES, entityCounter*indicesPerEntity, GL_UNSIGNED_INT, 0); // draw a batch of entities
            glDrawArrays(GL_POINTS, 0, entityCounter);
        }

        if (Debug->settings.drawEntityIDs) {
            int nRenderedIDs = 0;
            ecs.ForEach<Query>([&](Entity entity){
                // TODO: Render entity ids with TEXT_RENDERING
            });
            LogInfo("rendered %d ids", nRenderedIDs);
        }

        if (Debug->settings.drawEntityRects) {
            // TODO: Render entity rectangles
        }
    }
};