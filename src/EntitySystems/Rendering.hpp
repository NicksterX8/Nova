#ifndef RENDERING_SYSTEMS_INCLUDED
#define RENDERING_SYSTEMS_INCLUDED

#include <vector>
#include <tuple>
#include "../ECS/EntitySystem.hpp"
#include "../ECS/Query.hpp"
#include "../ECS/ECS.hpp"
#include "../utils/Log.hpp"
#include "../Rendering/Drawing.hpp"
#include "../utils/Debug.hpp"
#include "../constants.hpp"

//using ECS::EntitySystem;

/*
class RenderSizeSystem : public EntitySystem {
public:
    const GameViewport* gameViewport;

    #define SYSTEM_ACCESS(component) (sys.componentAccess[getID<component>()])

    RenderSizeSystem(EntityWorld* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<EC::Render>(Read | Write);
        SYSTEM_ACCESS(EC::Size) = Read;
    }

    bool Query(ComponentFlags entitySignature) const {
        return ((entitySignature & componentSignature<EC::Size, EC::Render>()) == componentSignature<EC::Size, EC::Render>());
    }

    void Update() {
        ForEach([&](Entity entity){
            SDL_FRect* renderDst = &sys.GetReadWrite<EC::Render>(entity)->destination;
            renderDst->w = sys.GetReadOnly<EC::Size>(entity)->width * TileWidth;
            renderDst->h = sys.GetReadOnly<EC::Size>(entity)->height * TileHeight;
        });
    }

private:

};

class RenderPositionSystem : public EntitySystem {
public:
    const GameViewport* gameViewport;

    RenderPositionSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<EC::Position>(Read);
        sys.GiveAccess<EC::Render>(Read | Write);
    }

    bool Query(ComponentFlags entitySignature) const {
        return ((entitySignature & componentSignature<EC::Position, EC::Render>()) == componentSignature<EC::Position, EC::Render>());
    }

    void Update() {
        ForEach([&](Entity entity){
            // copy rotation to render component
            SDL_FRect* renderDst = &sys.GetReadWrite<EC::Render>(entity)->destination;
            Vec2 dstPosition = gameViewport->worldToPixelPositionF(*sys.GetReadOnly<EC::Position>(entity));
            renderDst->x = dstPosition.x;
            renderDst->y = dstPosition.y;
        });
    }

private:

};
*/


const GLuint quadIndices[6] = {
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
};

class RenderSystem {
public:
    typedef ECS::EntityQuery<
        ECS::RequireComponents<EC::Render, EC::Size, EC::Position>
    > Query;

    float scale = 1.0f;

    ModelData model;
    const GLuint entitiesPerBatch = 1000;
    const GLuint verticesPerEntity = 4;
    const GLuint indicesPerEntity = 6;
    const GLuint verticesPerBatch = entitiesPerBatch*verticesPerEntity;
    const GLuint indicesPerBatch = entitiesPerBatch*indicesPerEntity;
    const GLuint vertexSize = 7 * sizeof(GLfloat);
    const GLuint positionEntityVertexSize = verticesPerEntity * 3 * sizeof(GLfloat);
    const GLuint texCoordEntityVertexSize = verticesPerEntity * 3 * sizeof(GLfloat);
    const GLuint rotationEntityVertexSize = verticesPerEntity * 1 * sizeof(GLfloat);
    const size_t bufferSize = verticesPerBatch * vertexSize;

    RenderSystem() {
        GLuint vbo,ebo,vao;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glGenVertexArrays(1, &vao);


        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesPerBatch * sizeof(GLuint), NULL, GL_STATIC_DRAW);
 
        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(0);
        // texture coord
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)((size_t)positionEntityVertexSize * entitiesPerBatch));
        glEnableVertexAttribArray(1);
        // rotation
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(GLfloat), (void*)(((size_t)positionEntityVertexSize + texCoordEntityVertexSize) * entitiesPerBatch));
        glEnableVertexAttribArray(2);
 
        glBindVertexArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        void* elementBuffer = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        if (!elementBuffer) {
            LogError("bad things happened!!!");
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

        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        model.VAO = vao;
        model.VBO = vbo;
        model.EBO = ebo;
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
        buffer.texCoords = vertexBuffer + 3 * verticesPerBatch;
        buffer.rotations = vertexBuffer + 6 * verticesPerBatch;
        return buffer;
    }

    void Update(const ComponentManager<EC::Render, const EC::Size, const EC::Position, const EC::Health, const EC::Rotation>& ecs, const ChunkMap& chunkmap, RenderContext& ren, Camera& camera) {
        Shader& shader = ren.entityShader;
        shader.use();
        auto camTransform = camera.getTransformMatrix();
        shader.setMat4("transform", camTransform);

        auto cameraMin = camera.minCorner();
        auto cameraMax = camera.maxCorner();
        Vec2 cameraSize = {camera.viewWidth(), camera.viewHeight()};

        static float shaderSetAngle = 0.0f;

        struct EntityVertex {
            glm::vec3 pos;
            glm::vec3 texCoord;
            float rotation;
        };

        glBindBuffer(GL_ARRAY_BUFFER, model.VBO);
        

        VertexDataArrays buffer = mapVertexBuffer();

        int entityCounter = 0, entitiesRenderered = 0;
        
        glBindVertexArray(model.VAO);
        forEachEntityInBounds(ecs, &chunkmap, 
            Vec2(camera.position.x, camera.position.y),
            cameraSize*2.0f, 
        [&](EntityT<EC::Position> entity){
            if (Query::check(ecs.EntitySignature(entity))) {
                auto position =  ecs.Get<const EC::Position>(entity)->vec2();
                auto renderEC =  ecs.Get<const EC::Render>(entity);
                auto size     = *ecs.Get<const EC::Size>(entity);

                float rotation = 0.0f;
                if (ecs.EntityHas<EC::Rotation>(entity)) {
                    rotation = ecs.Get<const EC::Rotation>(entity)->degrees;
                }

                float texMaxX = (float)TextureData[renderEC->texture].width  / MY_TEXTURE_ARRAY_WIDTH;
                float texMaxY = (float)TextureData[renderEC->texture].height / MY_TEXTURE_ARRAY_HEIGHT;
                float w = size.width  / 2.0f;
                float h = size.height / 2.0f;
                float tex = static_cast<float>(renderEC->texture);
                glm::vec3 p = glm::vec3(position.x, position.y, renderEC->layer * 0.1f);
                
                const GLfloat vertexPositions[12] = {
                    p.x-w,   p.y-h,   p.z,
                    p.x-w,   p.y+h,   p.z,
                    p.x+w,   p.y+h,   p.z,
                    p.x+w,   p.y-h,   p.z,
                };
                const GLfloat vertexTexCoords[12] = {
                    0.0f,     0.0f,     tex,
                    0.0f,     texMaxY,  tex,
                    texMaxX,  texMaxY,  tex,
                    texMaxX,  0.0f,     tex
                };
                const GLfloat vertexRotations[4] = {
                    rotation,
                    rotation,
                    rotation,
                    rotation
                };

                memcpy(&buffer.positions[entityCounter * 12], vertexPositions, 12 * sizeof(GLfloat));
                memcpy(&buffer.texCoords[entityCounter * 12], vertexTexCoords, 12 * sizeof(GLfloat));
                memcpy(&buffer.rotations[entityCounter *  4], vertexRotations,  4 * sizeof(GLfloat));
                
                entityCounter++;
                entitiesRenderered++;

                if (entityCounter == entitiesPerBatch) {
                    // 6 indices per entity
                    glUnmapBuffer(GL_ARRAY_BUFFER); // put vertex data into effect
                    glDrawElements(GL_TRIANGLES, entityCounter*indicesPerEntity, GL_UNSIGNED_INT, 0); // draw a batch of entities
                    entityCounter = 0;
                    // have to remap vertex buffer after unmapping
                    buffer = mapVertexBuffer();
                }
            }
        });

        glUnmapBuffer(GL_ARRAY_BUFFER); // put vertex data into effect

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

#endif