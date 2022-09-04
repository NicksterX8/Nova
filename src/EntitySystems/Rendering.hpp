#ifndef RENDERING_SYSTEMS_INCLUDED
#define RENDERING_SYSTEMS_INCLUDED

#include <vector>
#include <tuple>
#include "../ECS/EntitySystem.hpp"
#include "../ECS/Query.hpp"
#include "../ECS/ECS.hpp"
#include "../Log.hpp"
#include "../Rendering/Drawing.hpp"
#include "../Debug.hpp"
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
 checkOpenGLErrors();
        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(0);
        // texture coord
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)((size_t)positionEntityVertexSize * entitiesPerBatch));
        glEnableVertexAttribArray(1);
        // rotation
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(GLfloat), (void*)(((size_t)positionEntityVertexSize + texCoordEntityVertexSize) * entitiesPerBatch));
        glEnableVertexAttribArray(2);
 checkOpenGLErrors();
        glBindVertexArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        void* elementBuffer = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        if (!elementBuffer) {
            Log.Error("bad things happened!!!");
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
            Log.Error("Oh no a bunch more bad stuff happened!");
            checkOpenGLErrors();
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
        checkOpenGLErrors();

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
        
        //Log.Info("entities rendererd: %d", entitiesRenderered);
        /*

        ForEach(ecs, [&](Entity entity){
            Vec2 position = ecs.Get<const EC::Position>(entity)->vec2();
            if (Vec2{position.x - camera.position.x, position.y - camera.position.y}.length() > 80.0f) {
                return;
            } 
            auto renderEC = ecs.Get<const EC::Render>(entity);
            auto size = *ecs.Get<const EC::Size>(entity);
            if (ecs.EntityHas<EC::Rotation>(entity)) {
                float angleDegrees = ecs.Get<const EC::Rotation>(entity)->degrees;
                shader.setFloat("angle", glm::radians(angleDegrees));
            }

            shader.setInt("texLayer", renderEC->texture);

            float texMaxX = (float)TextureData[renderEC->texture].width  / MY_TEXTURE_ARRAY_WIDTH;
            float texMaxY = (float)TextureData[renderEC->texture].height / MY_TEXTURE_ARRAY_HEIGHT;
            float w = size.width  / 2.0f;
            float h = size.height / 2.0f;
            glm::vec3 p = glm::vec3(position.x, position.y, renderEC->layer * 0.1f);
            const GLfloat vertices[20] = {
                p.x-w,   p.y-h, p.z,    0.0f,     0.0f,
                p.x-w,   p.y+h, p.z,    0.0f,     texMaxY,
                p.x+w,   p.y+h, p.z,    texMaxX,  texMaxY,
                p.x+w,   p.y-h, p.z,    texMaxX,  0.0f
            };

            glBindBuffer(GL_ARRAY_BUFFER, model.VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        });
        */
        /*
        forEachEntityInBounds(&chunkmap, 
            Vec2(gameViewport->world.x + gameViewport->world.w/2.0f, gameViewport->world.y + gameViewport->world.h/2.0f), 
            Vec2(gameViewport->world.w, gameViewport->world.h), 
        [&](EntityT<EC::Position> entity){
            if (Query::check(ecs.EntitySignature(entity))) {
                auto render = ecs.Get<EC::Render>(entity);
                SDL_FRect* renderDst = &render->destination;
                Vec2 size = ecs.Get<const EC::Size>(entity)->vec2();
                renderDst->w = size.x * TileWidth;
                renderDst->h = size.y * TileHeight;
                Vec2 position = ecs.Get<const EC::Position>(entity)->vec2();
                Vec2 dstPosition = gameViewport->worldToPixelPositionF(position);
                renderDst->x = dstPosition.x - renderDst->w * 0.5f;
                renderDst->y = dstPosition.y - renderDst->w * 0.5f;

                if (render->texture != NULL) {
                    renderLayers[render->layer].push_back({render->texture, &render->destination, render->rotation});
                    render->renderIndex = index++;
                }
            }
        });
*/
        /*


        forEachEntityInBounds(&chunkmap, 
            Vec2(gameViewport->world.x + gameViewport->world.w/2.0f, gameViewport->world.y + gameViewport->world.h/2.0f), 
            Vec2(gameViewport->world.w, gameViewport->world.h), 
        [&](EntityT<EC::Position> entity){
            if (Query::check(ecs.EntitySignature(entity))) {
                
            }
        });
        */

        /*
        ForEach(ecs, [&](Entity entity){
            SDL_FRect* renderDst = &ecs.Get<EC::Render>(entity)->destination;
            renderDst->w = ecs.Get<const EC::Size>(entity)->width * TileWidth;
            renderDst->h = ecs.Get<const EC::Size>(entity)->height * TileHeight;
        });

        ecs.ForEach<Query>([&](QueriedEntity entity){
            // copy position component to render component
            SDL_FRect* renderDst = &ecs.Get<EC::Render>(entity)->destination;
            Vec2 dstPosition = gameViewport->worldToPixelPositionF(ecs.Get<const EC::Position>(entity)->vec2());
            renderDst->x = dstPosition.x;
            renderDst->y = dstPosition.y;
        });

        ecs.ForEach<Query>([&](Entity entity){
            SDL_FRect* renderDst = &ecs.Get<EC::Render>(entity)->destination;
            renderDst->x -= renderDst->w * 0.5f;
            renderDst->y -= renderDst->h * 0.5f;
        });

        Uint32 index = 0;

        std::vector<RenderTarget> renderLayers[NUM_RENDER_LAYERS];
        ecs.ForEach<Query>([&](Entity entity){
            EC::Render* render = ecs.Get<EC::Render>(entity);
            if (render->texture != NULL) {
                const SDL_FRect* dest = &render->destination;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    
                    renderLayers[render->layer].push_back({render->texture, dest, render->rotation});
                    render->renderIndex = index++;
                }
            }
        });
        */

        /*
        int nRenderedEntities = 0;
        for (int l = 0; l < NUM_RENDER_LAYERS; l++) {
            std::vector<RenderTarget>& renderLayer = renderLayers[l];
            for (size_t r = 0; r < renderLayer.size(); r++) {
                SDL_RenderCopyExF(renderer, renderLayer[r].texture, NULL, renderLayer[r].destination, renderLayer[r].degreesRotation, NULL, SDL_FLIP_NONE);
                nRenderedEntities++;
            }
        }*/

        /*
        ecs.ForEach<ECS::EntityQuery<
            ECS::RequireComponents<EC::Render, EC::Health>
        >>([&](EntityT<EC::Health, EC::Render> entity){
            auto destRect = &ecs.Get<EC::Render>(entity)->destination;
            SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<const EC::Health>(&ecs)->health);
            auto dest = &_dest;
            if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<const EC::Health>(&ecs)->health);
            }
    
        });
        */

        if (Debug->settings.drawEntityIDs) {
            int nRenderedIDs = 0;
            ecs.ForEach<Query>([&](Entity entity){
                // TODO: TEXT RENDER
                /*
                SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%u", entity.id);
                auto dest = &_dest;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%u", entity.id);
                    nRenderedIDs++;
                }
                */
            });
            Log.Info("rendered %d ids", nRenderedIDs);
        }

        /*
        if (Debug->settings.drawEntityRects) {
            for (int l = 0; l < NUM_RENDER_LAYERS; l++) {
                std::vector<RenderTarget>& renderLayer = renderLayers[l];
                for (auto target : renderLayer) {
                    //Draw::debugRect(renderer, target.destination, scale);
                }
            }
        }
        */
    }
};
/*
// draw simple render entities
class RenderSystem : public EntitySystem {
public:
    SDL_Renderer* renderer = NULL;
    float scale = 1.0f;
    const GameViewport* gameViewport = NULL;
    ComponentFlags signature;

    struct RenderTarget {
        SDL_Texture* texture;
        const SDL_FRect* destination;
        float degreesRotation;
    };

    RenderSystem(ECS* ecs) : EntitySystem(ecs) {}

    RenderSystem(ECS* ecs, SDL_Renderer* renderer, const GameViewport* gameViewport) : EntitySystem(ecs) {
        this->renderer = renderer;
        this->gameViewport = gameViewport;
        using namespace EC;
        signature = componentSignature<Render>();

        using namespace ComponentAccess;
        sys.GiveAccess<Render>(ReadWrite);
        sys.GiveAccess<Size>(Read);
        sys.GiveAccess<Position>(Read);
    }

    bool Query(ComponentFlags entitySignature) const {
        if ((entitySignature & signature) == signature) {
            return true;
        }
        return false;
    }

    void Update() {
        ForEach<EC::Render, EC::Size>([&](auto entity){
            SDL_FRect* renderDst = &sys.GetReadWrite<EC::Render>(entity)->destination;
            renderDst->x -= renderDst->w * 0.5f;
            renderDst->y -= renderDst->h * 0.5f;
        });

        std::vector<RenderTarget> renderLayers[NUM_RENDER_LAYERS];
        ForEach([&](Entity entity){
            const EC::Render* render = sys.GetReadOnly<EC::Render>(entity);
            if (render->texture != NULL) {
                const SDL_FRect* dest = &render->destination;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    //(dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge() &&
                    //dest->y > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    
                    renderLayers[render->layer].push_back({render->texture, dest, render->rotation});
                }
            }
        });

        int nRenderedEntities = 0;
        for (int l = 0; l < NUM_RENDER_LAYERS; l++) {
            std::vector<RenderTarget>& renderLayer = renderLayers[l];
            for (size_t r = 0; r < renderLayer.size(); r++) {
                SDL_RenderCopyExF(renderer, renderLayer[r].texture, NULL, renderLayer[r].destination, renderLayer[r].degreesRotation, NULL, SDL_FLIP_NONE);
                nRenderedEntities++;
            }
        }

        ForEach<EC::Health, EC::Render>([&](EntityT<EC::Health, EC::Render> entity){   
            auto destRect = &sys.GetReadOnly<EC::Render>(entity)->destination;
            SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<EC::Health>()->healthValue);
            auto dest = &_dest;
            if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%.1f", entity.Get<EC::Health>()->healthValue);
            }
    
        });

        if (Debug->settings.drawEntityIDs) {
            int nRenderedIDs = 0;
            ForEach([&](Entity entity){
                auto destRect = &sys.GetReadOnly<EC::Render>(entity)->destination;
                SDL_Rect _dest = FC_GetBounds(FreeSans, destRect->x, destRect->y, FC_ALIGN_LEFT, FC_MakeScale(0.5, 0.5), "%u", entity.id);
                auto dest = &_dest;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    FC_DrawScale(FreeSans, renderer, destRect->x, destRect->y, FC_MakeScale(0.5, 0.5), "%u", entity.id);
                    nRenderedIDs++;
                }
            });
            Log("rendered %d ids", nRenderedIDs);
        }

        if (Debug->settings.drawEntityRects) {
            for (int l = 0; l < NUM_RENDER_LAYERS; l++) {
                std::vector<RenderTarget>& renderLayer = renderLayers[l];
                for (auto target : renderLayer) {
                    Draw::debugRect(renderer, target.destination, scale);
                }
            }
        }
    }
};

*/

#endif