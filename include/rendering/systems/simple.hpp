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
    const static GLint verticesPerEntity = 4;
    const static GLint indicesPerEntity = 6;
    const static GLint verticesPerBatch = entitiesPerBatch*verticesPerEntity;
    const static GLint indicesPerBatch = entitiesPerBatch*indicesPerEntity;
    const static GLint positionEntityVertexSize = verticesPerEntity * 3 * (GLint)sizeof(GLfloat);
    const static GLint texCoordEntityVertexSize = verticesPerEntity * 3 * (GLint)sizeof(GLfloat);
    const static GLint rotationEntityVertexSize = verticesPerEntity * 1 * (GLint)sizeof(GLfloat);
    const static GLint vertexSize = positionEntityVertexSize + texCoordEntityVertexSize + rotationEntityVertexSize;
    const static size_t bufferSize = verticesPerBatch * vertexSize;

    RenderSystem() {
        model = GlGenModel();
        model.bindAll();

        glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesPerBatch * sizeof(GLuint), NULL, GL_STATIC_DRAW);

        // for mental model only, not actually used in c++ code at this time
        struct EntityVertex {
            GLvec3 pos;
            GLvec3 texCoord;
            GLvec3 rotation;
        };

        constexpr GlVertexAttribute attributes[] = {
            {3, GL_FLOAT, sizeof(GLfloat)}, // pos
            {3, GL_FLOAT, sizeof(GLfloat)}, // texCoord
            {3, GL_FLOAT, sizeof(GLfloat)}  // rotation
        };
        enableVertexAttribsSOA(&attributes[0], 3, verticesPerBatch);

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
    }

    using QueriedEntity = EntityT<EC::Render, EC::Size, EC::Position>;

    template<class EntityQueryT = Query>
    inline void ForEach(const ComponentManager<>& ecs, std::function<void(QueriedEntity entity)> callback) {
        ecs.ForEach<Query>(callback);
    }

    struct VertexDataArrays {
        GLfloat* positions;
        GLfloat* texCoords;
        GLfloat* rotations;
    };

    VertexDataArrays mapVertexBuffer() {
        GLfloat* vertexBuffer = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        if (!vertexBuffer) {
            LogError("Oh no a bunch more bad stuff happened!");
            
            return {NULL,NULL,NULL};
        }
        VertexDataArrays buffer;
        buffer.positions = vertexBuffer;
        buffer.texCoords = &vertexBuffer[3 * verticesPerBatch];
        buffer.rotations = &vertexBuffer[6 * verticesPerBatch];
        return buffer;
    }

    void unmapVertexBuffer(VertexDataArrays* buffer) {
        assert(buffer);
        assert(buffer->positions);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        buffer->positions = nullptr;
        buffer->rotations = nullptr;
        buffer->texCoords = nullptr;
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

                glm::ivec2 textureSize = textureData[renderEC->texture].size;
                glm::vec2 texMin = {0.5f, 0.5f};
                glm::vec2 texMax = glm::vec2(textureSize) - glm::vec2{0.5f, 0.5f};
                float w = size.width  / 2.0f;
                float h = size.height / 2.0f;
                float tex = static_cast<float>(renderEC->texture);
                glm::vec3 p = glm::vec3(position.x, position.y, getLayerHeight(renderEC->layer));
                
                const GLfloat vertexPositions[12] = {
                    p.x-w,   p.y-h,   p.z,
                    p.x-w,   p.y+h,   p.z,
                    p.x+w,   p.y+h,   p.z,
                    p.x+w,   p.y-h,   p.z,
                };
                const GLfloat vertexTexCoords[12] = {
                    texMin.x,  texMin.y,  tex,
                    texMin.x,  texMax.y,  tex,
                    texMax.x,  texMax.y,  tex,
                    texMax.x,  texMin.y,  tex
                };
                const GLfloat vertexRotations[12] = {
                    rotation, p.x, p.y,
                    rotation, p.x, p.y,
                    rotation, p.x, p.y,
                    rotation, p.x, p.y
                };

                memcpy(&buffer.positions[entityCounter * 12], vertexPositions, 12 * sizeof(GLfloat));
                memcpy(&buffer.texCoords[entityCounter * 12], vertexTexCoords, 12 * sizeof(GLfloat));
                memcpy(&buffer.rotations[entityCounter * 12], vertexRotations, 12 * sizeof(GLfloat));
                
                entityCounter++;
                entitiesRenderered++;

                if (entityCounter == entitiesPerBatch) {
                    unmapVertexBuffer(&buffer); // put vertex data into effect
                    glDrawElements(GL_TRIANGLES, entityCounter*indicesPerEntity, GL_UNSIGNED_INT, 0); // draw a batch of entities
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
            glDrawElements(GL_TRIANGLES, entityCounter*indicesPerEntity, GL_UNSIGNED_INT, 0); // draw a batch of entities
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