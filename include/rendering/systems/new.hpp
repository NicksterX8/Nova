#ifndef RENDERING_SYSTEMS_NEW_INCLUDED
#define RENDERING_SYSTEMS_NEW_INCLUDED

#include <array>
#include "ECS/system.hpp"
#include "world/components/components.hpp"
#include "world/functions.hpp"
#include "rendering/utils.hpp"
#include "rendering/context.hpp"
#include "rendering/gui.hpp"
#include "Chunks.hpp"
#include "utils/Debug.hpp"

struct MappedVertexData {
    int bytesUsed;

};

void mapVertexImpl(size_t vertexCount, void* currentBufferPos) {
    // do nothing
}

template<typename FirstAttribute, typename... Attributes>
void mapVertexImpl(size_t vertexCount, void* currentBufferPos, FirstAttribute*& first, Attributes*&... remaining) {
    size_t sizeOfAttribute = vertexCount * sizeof(FirstAttribute);
    first = (FirstAttribute*)currentBufferPos;
    currentBufferPos = (char*)currentBufferPos + sizeOfAttribute;
    mapVertexImpl(vertexCount, currentBufferPos, remaining...);
}

template<typename... Attributes>
GLvoid* mapVertex(size_t vertexCount, Attributes*&... attributePtrs) {
    GLvoid* vertexBuffer = (GLvoid*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if (!vertexBuffer) {
        LogError("Failed to map buffer!");
        return nullptr;
    }

    mapVertexImpl(vertexCount, vertexBuffer, attributePtrs...);
    return vertexBuffer;
}


namespace World {

namespace RenderSystems {

using namespace ECS::System;

struct GetEntityRotationsJob : IJobParallelFor {
    ComponentArray<const EC::Rotation> rotation;
    
    GLfloat* outRotation;

    GetEntityRotationsJob(JobGroup group, GLfloat* outRotation)
    : IJobParallelFor(group), rotation(this), outRotation(outRotation) {

    }

    void Execute(int N) {
        outRotation[N] = rotation[N].degrees;
    }
};

struct DrawEntityViewBoxJob : IJobSingleThreaded {
    const Box* dimensions;

    GuiRenderer& worldGuiRenderer;

    DrawEntityViewBoxJob(JobGroup group, const Box* dimensions, GuiRenderer& worldGuiRenderer)
    : IJobSingleThreaded(group), worldGuiRenderer(worldGuiRenderer) {}

    void Execute(int N) {
        constexpr SDL_Color rectColor = {255, 0, 255, 180};
        worldGuiRenderer.rectOutline(dimensions[N].rect(), rectColor, Vec2(0.05f), Vec2(0.05f));
    }
};

struct GetEntityDimensionsJob : IJobParallelFor {
    ComponentArray<const EC::Position> positions;
    ComponentArray<const EC::ViewBox> viewboxes;
    
    Box* dimensions;

    GetEntityDimensionsJob(JobGroup group, Box* outDimensions)
    : IJobParallelFor(group), positions(this), viewboxes(this), dimensions(outDimensions) {}

    void Execute(int N) {
        Vec2 pos = positions[N].vec2();
        Box viewbox = viewboxes[N].box;
        
        dimensions[N].min = pos + viewbox.min;
        dimensions[N].size = viewbox.size;
    }
};

struct EntityColorJob : IJobParallelFor {
    OptionalComponentArray<const EC::Health> maybeHealth;
    OptionalComponentArray<const EC::Selected> maybeSelected;

    glm::vec4* colors;

    EntityColorJob(JobGroup group, glm::vec4* colors)
    : IJobParallelFor(group),
    maybeHealth(this, &EntityColorJob::healthExecute), maybeSelected(this, &EntityColorJob::selectedExecute),
    colors(colors) {}

    // class laid out in execution order
    void Execute(int N) {
        colors[N] = {1.0, 1.0, 1.0, 1.0};
    }

    // if entity has health, it will execute this method in addition to the normal execute method.
    // Entities will always perform Execute first, then healthExecute. But, when healthExecute starts,
    // not all entities will have finished Execute. 
    void healthExecute(int N) {
        if (maybeHealth[N].timeDamaged != NullTick && Metadata->getTick() - maybeHealth[N].timeDamaged < 5) {
            blend(&colors[N], glm::vec4{1, 0, 0, 0.5});
        }
    }

    // if an entity has both health and selected, healthExecute will execute before selectedExecute,
    // because maybeHealth was initialized before maybeSelected
    void selectedExecute(int N) {
        blend(&colors[N], glm::vec4{0.5, 0.5, 1.0, 0.5});
    }

    void blend(glm::vec4* base, glm::vec4 fg) {
        float alpha = fg.a;
        float invAlpha = 1 - alpha;
        *base = fg * alpha + *base * invAlpha;
    }
};

using RenderSystem = ISystem;

struct RenderEntitySystem : RenderSystem {
    constexpr static GLint entitiesPerBatch = 512;
    constexpr static GLint verticesPerEntity = 1;
    constexpr static GLint verticesPerBatch = entitiesPerBatch*verticesPerEntity;

    constexpr static ComponentGroup<
        ReadOnly<EC::Render>,
        ReadOnly<EC::Position>,
        ReadOnly<EC::ViewBox>
    > minimalGroup;

    constexpr static ComponentGroup<
        ReadOnly<EC::Render>,
        ReadOnly<EC::Position>,
        ReadOnly<EC::ViewBox>,
        ReadOnly<EC::Rotation>
    > rotationGroup;

    float scale = 1.0f;

    GlModelSOA model;

    RenderContext& ren;
    const Camera& camera;
    const EntityWorld& ecs;
    const ChunkMap& chunkmap;

    struct VertexDataArrays {
        void* buffer = nullptr;

        // all are just offsets of buffer
        glm::vec3* positions = nullptr;
        glm::vec2* sizes = nullptr;
        GLfloat* rotations = nullptr;
        std::array<GLushort, 4>* texCoords = nullptr;
        glm::vec4* colors = nullptr;
    } vertices;

    // returns true on success, false on failure
    bool mapVertexArrays() {
        vertices.buffer = mapVertex(verticesPerBatch,
            vertices.positions, vertices.sizes, vertices.rotations,
            vertices.texCoords, vertices.colors);
        return vertices.buffer != nullptr;
    }

    void unmapVertexArrays() {
        assert(vertices.buffer);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        // set all to null
        memset(&vertices, 0, sizeof(vertices));
    }

    
    RenderEntitySystem(SystemManager& manager, RenderContext& renderContext, const Camera& camera, const EntityWorld& ecs, const ChunkMap& chunkmap)
    : RenderSystem(manager), ren(renderContext), camera(camera), ecs(ecs), chunkmap(chunkmap) {
        auto vertexFormat = GlMakeVertexFormat(0, {
            {3, GL_FLOAT, sizeof(GLfloat)}, // pos
            {2, GL_FLOAT, sizeof(GLfloat)}, // size
            {1, GL_FLOAT, sizeof(GLfloat)}, // rotation
            {4, GL_UNSIGNED_SHORT, sizeof(GLushort)}, // texCoords (min.x, min.y, max.x, max.y)
            {4, GL_FLOAT, sizeof(GLfloat)} // color
        });

        model = makeModelSOA(verticesPerBatch, nullptr, GL_STREAM_DRAW, vertexFormat);
    }

    void BeforeExecution() {
        auto camTransform = camera.getTransformMatrix();

        auto shader = ren.shaders.use(Shaders::Entity);
        shader.setMat4("transform", camTransform);

        // glBindVertexArray(model.model.vao);
        // glBindBuffer(GL_ARRAY_BUFFER, model.model.vbo);

        // if (!mapVertexArrays()) {
        //     LogError("Failed to map vertex arrays!");
        // }
        
        
    }

    void ScheduleJobsOld() {
        auto dimensions = makeTempGroupArray<Box>(minimalGroup);
        GetEntityDimensionsJob(&minimalGroup, dimensions);

        auto colors = makeTempGroupArray<glm::vec4>(minimalGroup);
        EntityColorJob(&minimalGroup, colors);

        auto rotations = makeTempGroupArray<GLfloat>(rotationGroup);
        GetEntityRotationsJob(&rotationGroup, rotations);

        if (Debug->settings["drawEntityViewBoxes"]) {
            DrawEntityViewBoxJob(&minimalGroup, dimensions, ren.guiRenderer);
        }
    }

    void ScheduleJobs() {

    }

    void AfterExecution() {
        auto camTransform = camera.getTransformMatrix();

        auto shader = ren.shaders.use(Shaders::Entity);
        shader.setMat4("transform", camTransform);

        glBindVertexArray(model.model.vao);
        glBindBuffer(GL_ARRAY_BUFFER, model.model.vbo);
        
        if (!mapVertexArrays()) {
            // can't render :(
            LogError("Failed to map entity vertex arrays!");
            return;
        }

        const auto* textureAtlas = &ren.textureAtlas;
        const auto* textureData = ren.textures.data.data;

        Boxf cameraBounds = camera.maxBoundingArea();

        My::Vec<Entity> entityList = My::Vec<Entity>::WithCapacity(16);

        World::forEachEntityInBounds(ecs, &chunkmap, cameraBounds, 
        [&](Entity entity){
            if (ecs.EntitySignature(entity).hasComponents<EC::Render, EC::ViewBox, EC::Position>()) {
                entityList.push(entity);
            }
        });

        My::Vec<Uint16> textureIndexList = My::Vec<Uint16>::Filled(entityList.size, 0);

        int realNumEntities = entityList.size;
        for (int e = 0; e < realNumEntities; e++) {
            Entity entity = entityList[e];
            EC::Render* renderComponent = ecs.Get<EC::Render>(entity);
            for (int i = 1; i < renderComponent->numTextures; i++) {
                entityList.push(entity);
                textureIndexList.push(i);
            }
        }

        int entitiesRendered = 0;
        while (entitiesRendered < entityList.size) {
            int batchSize = MIN(entityList.size - entitiesRendered, entitiesPerBatch);
            renderBatch(ArrayRef(entityList.data, entityList.size), ArrayRef(textureIndexList.data, textureIndexList.size), ecs, chunkmap, ren);
            entitiesRendered += batchSize;
        }

        // put vertex data into effect
        unmapVertexArrays();

        // number of entities wasn't exactly divisible by entitiesPerBatch, so we have some left to do
        if (entitiesRendered < entityList.size) {
            // draw a batch of entities
            glDrawArrays(GL_POINTS, 0, entityList.size - entitiesRendered);
        }

        textureIndexList.destroy();
        entityList.destroy();
    }

    void renderBatch(ArrayRef<Entity> entities, ArrayRef<Uint16> texIndices, const EntityWorld& ecs, const ChunkMap& chunkmap, RenderContext& ren) {
        namespace EC = World::EC;
        auto& buffer = vertices;
        
        int batchSize = entities.size();
    
        // position and size
        for (int e = 0; e < batchSize; e++) {
            Entity entity = entities[e];
            Vec2 pos = ecs.Get<EC::Position>(entity)->vec2();
            EC::ViewBox* viewbox = ecs.Get<EC::ViewBox>(entity);
            EC::Render* renderComponent = ecs.Get<EC::Render>(entity);
            auto& texture = renderComponent->textures[texIndices[e]];
            
            Vec2 viewMin = viewbox->box.min + texture.box.min * viewbox->box.size;
            Vec2 size = viewbox->box.size * texture.box.size * 0.5f;
            buffer.positions[e].x = pos.x + viewMin.x + size.x;
            buffer.positions[e].y = pos.y + viewMin.y + size.y;

            buffer.sizes[e] = size;
        }

        if (Debug->settings["drawEntityViewBoxes"]) {
            for (int e = 0; e < batchSize; e++) {
                Entity entity = entities[e];
                Vec2 pos = ecs.Get<EC::Position>(entity)->vec2();
                EC::ViewBox* viewbox = ecs.Get<EC::ViewBox>(entity);
                
                Vec2 viewMin = viewbox->box.min;
                Vec2 size = viewbox->box.size;
                Vec2 min = pos + viewMin;
                FRect entityRect = {
                    min.x,
                    min.y,
                    size.x,
                    size.y
                };
                constexpr SDL_Color rectColor = {255, 0, 255, 180};
                ren.worldGuiRenderer.rectOutline(entityRect, rectColor, Vec2(0.05f), Vec2(0.05f));
            }
        }

        if (Debug->settings["drawEntityCollisionBoxes"]) {
            for (int e = 0; e < batchSize; e++) {
                Entity entity = entities[e];
                Vec2 pos = ecs.Get<EC::Position>(entity)->vec2();
                auto* box = ecs.Get<EC::CollisionBox>(entity);
                if (!box) continue;
                
                Vec2 boxMin = box->box.min;
                Vec2 size = box->box.size;
                Vec2 min = pos + boxMin;
                FRect entityRect = {
                    min.x,
                    min.y,
                    size.x,
                    size.y
                };
                constexpr SDL_Color rectColor = {0, 255, 255, 180};
                ren.worldGuiRenderer.rectOutline(entityRect, rectColor, Vec2(0.05f), Vec2(0.05f));
            }
        }

        if (Debug->settings["drawEntityIDs"]) {
            for (int e = 0; e < batchSize; e++) {
                Entity entity = entities[e];
                Vec2 pos = ecs.Get<EC::Position>(entity)->vec2();
                EC::ViewBox* viewbox = ecs.Get<EC::ViewBox>(entity);

                Vec2 min = pos + viewbox->box.min;
                Vec2 max = pos + viewbox->box.max();

                char message[256];
                snprintf(message, 256, "%d", entity.id);
                ren.worldGuiRenderer.text->render(message,
                    {min.x, max.y},
                    TextFormattingSettings{.align = TextAlignment::TopLeft}, {}
                );
            }
        }

        // texture
        for (int e = 0; e < batchSize; e++) {
            Entity entity = entities[e];
            EC::Render* renderComponent = ecs.Get<EC::Render>(entity);
            auto& texture = renderComponent->textures[texIndices[e]];
            auto space = getTextureAtlasSpace(&ren.textureAtlas, texture.tex);
            float height = getEntityHeight(entity.id, texture.layer);
            buffer.positions[e].z = height;
            buffer.texCoords[e] = {space.min.x, space.min.y, space.max.x, space.max.y};
        }

        // animation
        for (int e = 0; e < batchSize; e++) {
            Entity entity = entities[e];
            EC::Render* renderComponent = ecs.Get<EC::Render>(entity);
            auto& texture = renderComponent->textures[texIndices[e]];
            auto animation = getAnimation(&ren.textures, texture.tex);
            if (!animation) continue;
            auto animationSpace = getTextureAtlasSpace(&ren.textureAtlas, texture.tex);
            auto space = getAnimationFrame(animationSpace, *animation, (int)floor(fmod(Metadata->getTick(), animation->frameCount * animation->updatesPerFrame) / animation->updatesPerFrame));
            buffer.positions[e].z = getEntityHeight(entity.id, texture.layer);
            buffer.texCoords[e] = {space.min.x, space.min.y, space.max.x, space.max.y};
        }

        // rotation
        for (int e = 0; e < batchSize; e++) {
            Entity entity = entities[e];
            auto* rotation = ecs.Get<EC::Rotation>(entity);
            buffer.rotations[e] = rotation ? rotation->degrees : 0.0f;
        }

        // color
        for (int e = 0; e < batchSize; e++) {
            Entity entity = entities[e];
            
            glm::vec4 colorShading = {1.0, 1.0, 1.0, 1.0};
            auto* health = ecs.Get<EC::Health>(entity);
            if (health) {
                if (health->timeDamaged != NullTick && Metadata->getTick() - health->timeDamaged < 5) {
                    blend(&colorShading, glm::vec4{1, 0, 0, 0.5});
                }
            }

            auto* selected = ecs.Get<EC::Selected>(entity);
            if (selected) {
                blend(&colorShading, glm::vec4{0.5, 0.5, 1.0, 0.5});
            }

            auto* render = ecs.Get<EC::Render>(entity);
            colorShading.a = render->textures[texIndices[e]].opacity;

            buffer.colors[e] = colorShading;
        }

        // draw a batch of entities
        glDrawArrays(GL_POINTS, 0, batchSize);
    }

    void blend(glm::vec4* base, glm::vec4 fg) {
        float alpha = fg.a;
        float invAlpha = 1 - alpha;
        *base = fg * alpha + *base * invAlpha;
    }
};

}

}

#endif