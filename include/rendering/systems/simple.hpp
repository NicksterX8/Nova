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
    return layer * 0.05f - 0.5f;
}

struct RenderSystem {
    typedef ECS::EntityQuery<
        ECS::RequireComponents<EC::Render, EC::Position, EC::ViewBox>
    > Query;

    float scale = 1.0f;

    GlModelSOA model;

    const static GLint entitiesPerBatch = 1000;
    const static GLint verticesPerEntity = 1;
    //const static GLint indicesPerEntity = 0;
    const static GLint verticesPerBatch = entitiesPerBatch*verticesPerEntity;
    //const static GLint indicesPerBatch = entitiesPerBatch*indicesPerEntity;
    struct Vertex {
        glm::vec3 pos;
        glm::vec2 size;
        GLfloat rotation;
        GLushort texCoords[4];
        glm::vec4 color;
    };
    //constexpr static GLint vertexSize = 3 * sizeof(GLfloat) + 2 * sizeof(GLfloat) + 1 * sizeof(GLfloat) + 4 * sizeof(GLushort) + 2 * sizeof(GLfloat);
    //static_assert(sizeof(Vertex) == vertexSize, "");
    //const static size_t bufferSize = verticesPerBatch * vertexSize;

    RenderSystem() {
        auto vertexFormat = GlMakeVertexFormat(0, {
            {3, GL_FLOAT, sizeof(GLfloat)}, // pos
            {2, GL_FLOAT, sizeof(GLfloat)}, // size
            {1, GL_FLOAT, sizeof(GLfloat)}, // rotation
            {4, GL_UNSIGNED_SHORT, sizeof(GLushort)}, // texCoords (min.x, min.y, max.x, max.y)
            {4, GL_FLOAT, sizeof(GLfloat)} // color
        });

        model = makeModelSOA(verticesPerBatch, nullptr, GL_STREAM_DRAW, vertexFormat);

        assert(vertexFormat.totalSize() == sizeof(Vertex));
    }

    using QueriedEntity = EntityT<EC::Render, EC::Size, EC::Position>;

    template<class EntityQueryT = Query>
    inline void ForEach(const ComponentManager<>& ecs, std::function<void(QueriedEntity entity)> callback) {
        ecs.ForEach<Query>(callback);
    }

    struct VertexDataArrays {
        glm::vec3* positions = nullptr;
        glm::vec2* sizes = nullptr;
        GLfloat* rotations = nullptr;
        GLushort* texCoords = nullptr;
        glm::vec4* colors = nullptr;
    };

    VertexDataArrays mapVertexBuffer() {
        GLvoid* vertexBuffer = (GLvoid*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        if (!vertexBuffer) {
            LogError("Oh no a bunch more bad stuff happened!");
            
            return {};
        }
        VertexDataArrays buffer;
        buffer.positions = (glm::vec3*)vertexBuffer;
        buffer.sizes     = (glm::vec2*)&buffer.positions[verticesPerBatch];
        buffer.rotations = (GLfloat*)&buffer.sizes[verticesPerBatch];
        buffer.texCoords = (GLushort*)&buffer.rotations[verticesPerBatch];
        buffer.colors = (glm::vec4*)&buffer.texCoords[4 * verticesPerBatch];
        return buffer;
    }

    void unmapVertexBuffer(VertexDataArrays* buffer) {
        assert(buffer);
        assert(buffer->positions);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        *buffer = {};
    }

    using ECS_t = ComponentManager<EC::Render, const EC::ViewBox, const EC::Health, const EC::Rotation>;

    float getEntityHeight(EntityID id, int renderLayer) {
        return getLayerHeight(renderLayer) + id * 0.0000001f;
    }

    void blend(glm::vec4* base, glm::vec4 fg) {
        float alpha = fg.a;
        float invAlpha = 1 - alpha;
        *base = fg * alpha + *base * invAlpha;
    }

    int renderBatch(ArrayRef<Entity> entities, ArrayRef<Uint16> texIndices, VertexDataArrays buffer, const ECS_t& ecs, const ChunkMap& chunkmap, RenderContext& ren) {
        int batchSize = entities.size();
    
        // position and size
        for (int e = 0; e < batchSize; e++) {
            Entity entity = entities[e];
            Vec2 pos = ecs.Get<EC::Position>(entity)->vec2();
            EC::ViewBox* viewbox = ecs.Get<EC::ViewBox>(entity);
            EC::Render* renderComponent = ecs.Get<EC::Render>(entity);
            auto& texture = renderComponent->textures[texIndices[e]];
            
            Vec2 viewMin = viewbox->box.min + texture.box.min;
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
                    pos.x,
                    pos.y,
                    size.x,
                    size.y
                };
                constexpr SDL_Color rectColor = {255, 0, 255, 180};
                ren.worldGuiRenderer.rectOutline(entityRect, rectColor, 0.05f, 0.05f);
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
                ren.worldGuiRenderer.rectOutline(entityRect, rectColor, 0.05f, 0.05f);
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
                    TextFormattingSettings(TextAlignment::TopLeft), {}
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
            buffer.texCoords[e * 4 + 0] = space.min.x;
            buffer.texCoords[e * 4 + 1] = space.min.y;
            buffer.texCoords[e * 4 + 2] = space.max.x;
            buffer.texCoords[e * 4 + 3] = space.max.y;
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
            buffer.texCoords[e * 4 + 0] = space.min.x;
            buffer.texCoords[e * 4 + 1] = space.min.y;
            buffer.texCoords[e * 4 + 2] = space.max.x;
            buffer.texCoords[e * 4 + 3] = space.max.y;
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
                blend(&colorShading, glm::vec4{0.0, 0.0, 1.0, 0.5});
            }

            auto* render = ecs.Get<EC::Render>(entity);
            colorShading.a = render->textures[texIndices[e]].opacity;

            buffer.colors[e] = colorShading;
        }

        // draw a batch of entities
        glDrawArrays(GL_POINTS, 0, batchSize);
    }

    void Update(const ECS_t& ecs, const ChunkMap& chunkmap, RenderContext& ren, Camera& camera) {
        auto camTransform = camera.getTransformMatrix();

        auto shader = ren.shaders.use(Shaders::Entity);
        shader.setMat4("transform", camTransform);

        glBindVertexArray(model.model.vao);
        glBindBuffer(GL_ARRAY_BUFFER, model.model.vbo);
        
        VertexDataArrays buffer = mapVertexBuffer();
        if (!buffer.positions) {
            // can't render :(
            return;
        }

        const auto* textureAtlas = &ren.textureAtlas;
        const auto* textureData = ren.textures.data.data;

        Boxf cameraBounds = camera.maxBoundingArea();

        My::Vec<Entity> entityList = My::Vec<Entity>::WithCapacity(16);

        forEachEntityInBounds(ecs, &chunkmap, cameraBounds, 
        [&](Entity entity){
            if (Query::Check(ecs.EntitySignature(entity))) {
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
            renderBatch(ArrayRef(entityList.data, entityList.size), ArrayRef(textureIndexList.data, textureIndexList.size), buffer, ecs, chunkmap, ren);
            entitiesRendered += batchSize;
        }

        // put vertex data into effect, if the buffer exists
        if (buffer.positions) {
            unmapVertexBuffer(&buffer);
        }

        // number of entities wasn't exactly divisible by entitiesPerBatch, so we have some left to do
        if (entitiesRendered < entityList.size) {
            // draw a batch of entities
            glDrawArrays(GL_POINTS, 0, entityList.size - entitiesRendered);
        }
    }
};