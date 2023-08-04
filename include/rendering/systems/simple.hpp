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
        glm::vec2 velocity;
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
            {2, GL_FLOAT, sizeof(GLfloat)} // velocity
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
        glm::vec2* velocities = nullptr;
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
        buffer.velocities = (glm::vec2*)&buffer.texCoords[4 * verticesPerBatch];
        return buffer;
    }

    void unmapVertexBuffer(VertexDataArrays* buffer) {
        assert(buffer);
        assert(buffer->positions);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        *buffer = {};
    }

    using ECS_t = ComponentManager<EC::Render, const EC::Size, const EC::Position, const EC::Health, const EC::Rotation>;

    bool renderEntity(Entity entity, const ECS_t& ecs, const TextureAtlas* textureAtlas, RenderContext& ren, VertexDataArrays buffer, int& entityCounter) {
        /* Get necessary entity data */
        auto position =  ecs.Get<const EC::Position>(entity)->vec2();
        auto renderEC =  ecs.Get<const EC::Render>(entity);
        auto size     = *ecs.Get<const EC::Size>(entity);

        float rotation = 0.0f;
        if (ecs.EntityHas<EC::Rotation>(entity)) {
            rotation = ecs.Get<const EC::Rotation>(entity)->degrees;
        }

        TextureID texture = renderEC->texture;

        /* Render entity sprite */
        auto space = getTextureAtlasSpace(textureAtlas, texture);
        glm::vec<2, GLushort> texMin = space.min;
        glm::vec<2, GLushort> texMax = space.max;

        float w = size.width  / 2.0f;
        float h = size.height / 2.0f;

        glm::vec3 p = glm::vec3(position.x, position.y, getLayerHeight(renderEC->layer));
        
        glm::vec3 vertexPosition = {
            p.x,   p.y,   p.z,
        };
        const GLushort vertexTexCoords[4] = {
            texMin.x,  texMin.y,
            texMax.x,  texMax.y, 
        };
        glm::vec2 vertexSize = {
            w, h
        };
        const GLfloat vertexRotation = rotation;
        const GLubyte vertexTexture = texture;
        // TODO: fix
        glm::vec2 vertexVelocity = g.playerMovement;

        buffer.positions[entityCounter] = vertexPosition;
        memcpy(&buffer.texCoords[entityCounter * 4], vertexTexCoords, 4 * sizeof(GLfloat));
        buffer.sizes[entityCounter] = vertexSize;
        buffer.rotations[entityCounter] = vertexRotation;
        buffer.velocities[entityCounter] = vertexVelocity;
        
        entityCounter++;

        if (entityCounter == entitiesPerBatch) {
            unmapVertexBuffer(&buffer); // put vertex data into effect
            //glDrawElements(GL_TRIANGLES, entityCounter*indicesPerEntity, GL_UNSIGNED_INT, 0); // draw a batch of entities
            glDrawArrays(GL_POINTS, 0, entityCounter);
            entityCounter = 0;
            // have to remap vertex buffer after unmapping
            buffer = mapVertexBuffer();
        }

        /* Render misc entity details */
        if (Debug->settings.drawEntityRects) {
            // TODO: Render entity rectangles
            FRect entityRect = {
                p.x - w,
                p.y - h,
                w*2,
                h*2
            };
            constexpr SDL_Color rectColor = {255, 0, 255, 180};
            ren.worldGuiRenderer.rectOutline(entityRect, rectColor, 0.05f, 0.05f);
        }

        if (Debug->settings.drawEntityIDs) {
            char message[256];
            snprintf(message, 256, "%d", entity.id);
            ren.worldGuiRenderer.text->render(message, {p.x, p.y});
        }

        return true;
    }

    void Update(const ECS_t& ecs, const ChunkMap& chunkmap, RenderContext& ren, Camera& camera) {
        /*
        GLenum buffers[] = {
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1
        };
        glDrawBuffers(2, buffers);
        */

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

        int entityCounter = 0, entitiesRendered = 0;
        forEachEntityInBounds(ecs, &chunkmap, 
            camera.position,
            camera.maxCorner() - camera.minCorner(), 
        [&](EntityT<EC::Position> entity){
            if (Query::Check(ecs.EntitySignature(entity))) {
                entitiesRendered += (int)renderEntity(entity, ecs, textureAtlas, ren, buffer, entityCounter);
            }
        });

        // put vertex data into effect, if the buffer exists
        if (buffer.positions) {
            unmapVertexBuffer(&buffer);
        }

        // number of entities wasn't exactly divisible by entitiesPerBatch, so we have some left to do
        if (entityCounter > 0) {
            // draw a batch of entities
            glDrawArrays(GL_POINTS, 0, entityCounter);
        }

        if (Debug->settings.drawEntityIDs) {
            int nRenderedIDs = 0;
            ecs.ForEach<Query>([&](Entity entity){
                // TODO: Render entity ids with TEXT_RENDERING
            });
            LogInfo("rendered %d ids", nRenderedIDs);
        }

        /*
        GLenum buffers2[] = {
            GL_COLOR_ATTACHMENT0
        };
        glDrawBuffers(1, buffers2);
        */
    }
};