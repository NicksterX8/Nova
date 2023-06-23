#include "rendering/rendering.hpp"
#include "rendering/systems/simple.hpp"
#include "Chunks.hpp"
#include "utils/Debug.hpp"
#include "utils/Log.hpp"
#include "utils/random.hpp"
#include "My/Vec.hpp"
#include "utils/vectors.hpp"
#include "llvm/ArrayRef.h"
#include "global.hpp"

/*
void highlightTargetedEntity(SDL_Renderer* ren, float scale, const GameViewport* gameViewport, const Gui* gui, const GameState* state, Vec2 playerTargetPos) {
    Vec2 screenPos = gameViewport->worldToPixelPositionF(playerTargetPos.vfloor());
    // only draw tile marker if mouse is actually on the world, not on the Gui
    if (!gui->pointInArea({(int)screenPos.x, (int)screenPos.y})) {
        OptionalEntity<> targetedEntity = state->player.selectedEntity;
        if (targetedEntity.Exists(&state->ecs)) {
            if (targetedEntity.Has<EC::Render>(&state->ecs)) {
                SDL_FRect entityRect = targetedEntity.Get<EC::Render>(&state->ecs)->destination;
                SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
                Draw::thickRect(ren, &entityRect, round(scale * 2));
            }
        } else {
            *
            forEachEntityInRange(&state->ecs, &state->chunkmap, playerTargetPos, M_SQRT2, [&](EntityT<EC::Position> entity){
                if (entity.Has<EC::Size, EC::Render>()) {
                    Vec2 position = *state->ecs.Get<EC::Position>(entity);
                    auto sizeComponent = state->ecs.Get<EC::Size>(entity);
                    Vec2 size = {sizeComponent->width, sizeComponent->height};
                    if (
                        playerTargetPos.x > position.x && playerTargetPos.x < position.x + size.x &&
                        playerTargetPos.y > position.y && playerTargetPos.y < position.y + size.y) {
                        // player is targeting this entity
                        Vec2 destPos = gameViewport->worldToPixelPositionF(position);
                        SDL_FRect entityRect = entity.Get<EC::Render>()->destination;
                        SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
                        Draw::thickRect(ren, &entityRect, round(scale * 2));
                        return 1;
                    }
                }
                return 0;
            });
            *
            auto focusedEntity = findPlayerFocusedEntity(&state->ecs, state->chunkmap, playerTargetPos);
            if (focusedEntity != NullEntity) {
                SDL_FRect* entityRect = &focusedEntity.Get<EC::Render>(&state->ecs)->destination;
                SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
                Draw::thickRect(ren, entityRect, round(scale * 2));
            }
        }
    }      
}
*/

static void renderChunk(RenderContext& ren, ChunkData& chunkdata, ModelData& chunkModel) {
    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.VBO);
    void* vertexBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    float* chunkVerts = static_cast<float*>(vertexBuffer);
    
    Vec2 startPos = chunkdata.tilePosition();

    const float tileWidth = 1.0f;
    const float tileHeight = 1.0f;

    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            int index = row * CHUNKSIZE + col;
            TileType type = (*chunkdata.chunk)[row][col].type;
            if (type == TileTypes::Empty) continue;
            
            float x = startPos.x + col * tileWidth;
            float y = startPos.y + row * tileHeight;
            float x2 = x + tileWidth;
            float y2 = y + tileHeight;

            TextureID texture = TileTypeData[type].background;
            float texMaxX = ((float)TextureData[texture].width)  / MY_TEXTURE_ARRAY_WIDTH;
            float texMaxY = ((float)TextureData[texture].height) / MY_TEXTURE_ARRAY_HEIGHT;
            /*
            if (texMaxX < 0.9f) {
                texMaxX -= 0.00001f;
            }
            if (texMaxY < 0.9f) {
                texMaxY -= 0.00001f;
            }
            */
            float tex = static_cast<float>(texture);
            const float tileVerts[] = {
                x,  y,
                0.0f, 0.0f, tex,

                x,  y2,
                0.0f, texMaxY, tex,

                x2, y2,
                texMaxX, texMaxY, tex,

                x2, y,
                texMaxX, 0.0f, tex
            };
            // just in case padding or anything happens
            static_assert(sizeof(tileVerts) == 4 * sizeof(TilemapVertex), "incorrect number of floats in vertex data");

            memcpy(&chunkVerts[index * 20], tileVerts, sizeof(tileVerts));
        }
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glDrawElements(GL_TRIANGLES, Render::numChunkIndices, GL_UNSIGNED_SHORT, 0);
}

static int loadShaders(RenderContext& ren) {
    auto& mgr = ren.shaders;

    auto entity = mgr.setup(Shaders::Entity, "entity");
    entity.setInt("texArray", TextureUnit::MyTextureArray);

    auto tilemap = mgr.setup(Shaders::Tilemap, "tilemap");
    tilemap.setInt("texArray", TextureUnit::MyTextureArray);

    auto text = mgr.setup(Shaders::Text, "text");
    text.setInt("text", TextureUnit::Text);

    auto sdf = mgr.setup(Shaders::SDF, "sdf");
    sdf.setInt("text", TextureUnit::Text);

    auto texture = mgr.setup(Shaders::Texture, "texture");
    texture.setInt("tex", TextureUnit::Single);

    auto color = mgr.setup(Shaders::Color, "color");
    
    auto point = mgr.setup(Shaders::Point, "point");



    /*
    ren.entityShader = loadShader("entity");
    ren.tilemapShader = loadShader("tilemap");
    ren.pointShader = loadShader("point");
    ren.colorShader = loadShader("color");
    ren.textShader = loadShader("text");
    ren.sdfShader = loadShader("sdf");
    ren.textureShader = loadShader("texture");
    */
    
    return 0;
}

void renderInit(RenderContext& ren) {
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Text);

    initFreetype();
    ren.font = Font(6.0f, 2.0f, FileSystem.assets.get("fonts/FreeSans.ttf"), 128, false);
    ren.debugFont = Font(7.0f, 2.0f, FileSystem.assets.get("fonts/Ubuntu-Regular.ttf"), 32, false);

    glActiveTexture(GL_TEXTURE0 + TextureUnit::MyTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ren.textureArray);

    GL::logErrors();

    ren.textureArray = makeTextureArray(FileSystem.assets.get());

    ren.shaders = {};

    loadShaders(ren);
    /*
    ren.tilemapShader.use();
    ren.tilemapShader.setInt("texArray", TextureUnit::MyTextureArray);
    ren.entityShader.use();
    ren.entityShader.setInt("texArray", TextureUnit::MyTextureArray);
    ren.textShader.use();
    ren.textShader.setInt("text", TextureUnit::Text);
    ren.sdfShader.use();
    ren.sdfShader.setInt("text", TextureUnit::Text);
    ren.textureShader.use();
    ren.textureShader.setInt("tex", TextureUnit::Single);

*/
    GL::logErrors();

    ren.textRenderer = TextRenderer::init(&ren.debugFont, ren.shaders.get(Shaders::Text), 0.0f);
    ren.textRenderer.defaultFormatting = TextFormattingSettings::Default();

    ren.quadRenderer = QuadRenderer(ren.shaders.get(Shaders::Color));

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glEnable(GL_PROGRAM_POINT_SIZE);

    GL::logErrors();

    const VertexAttribute attributes[] = {
        {2, GL_FLOAT, sizeof(GLfloat)},
        {3, GL_FLOAT, sizeof(GLfloat)}
    };
    ren.chunkModel = makeModel(
        Render::numChunkVerts * sizeof(TilemapVertex), NULL, GL_DYNAMIC_DRAW,
        Render::numChunkIndices * sizeof(GLushort), NULL, GL_DYNAMIC_DRAW,
        attributes, sizeof(attributes) / sizeof(VertexAttribute)
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ren.chunkModel.EBO);
    GLushort* chunkIndexBuffer = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    generateQuadVertexIndices(CHUNKSIZE * CHUNKSIZE, chunkIndexBuffer);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    GL::logErrors();
}

void renderQuit(RenderContext& ren) {
    /* Text rendering systems quit */
    ren.font.unload();
    ren.debugFont.unload();
    ren.textRenderer.destroy();
    quitFreetype();

    /* Shaders quit */
    /*
    ren.entityShader.destroy();
    ren.tilemapShader.destroy();
    ren.pointShader.destroy();
    ren.textShader.destroy();
    ren.textureShader.destroy();
    ren.chunkModel.destroy();
    ren.colorShader.destroy();
    */

    /* (Other) Textures quit */
    glDeleteTextures(1, &ren.textureArray);
}

static llvm::SmallVector<Draw::ColorVertex, 0> makeDemoQuads(SDL_Window* window) {
    int drawableWidth,drawableHeight;
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

    auto quadPoints = llvm::SmallVector<Draw::ColorVertex, 0>();
    quadPoints.reserve(500 * 4);
 
    for (int i = 0; i < 500; i++) {
        Vec2 min = {randomInt(-100, 100), randomInt(-100, 100)};
        Vec2 max = min + Vec2{randomInt(1, 3), randomInt(1, 3)};
        quadPoints.push_back({glm::vec3(min.x, min.y, 0.0f), glm::vec4(randomInt(0, 1), randomInt(0, 1), randomInt(0, 1), randomInt(1, 10) / 10.0f)});
        quadPoints.push_back({glm::vec3(min.x, max.y, 0.0f), glm::vec4(randomInt(0, 1), randomInt(0, 1), randomInt(0, 1), randomInt(1, 10) / 10.0f)});
        quadPoints.push_back({glm::vec3(max.x, max.y, 0.0f), glm::vec4(randomInt(0, 1), randomInt(0, 1), randomInt(0, 1), randomInt(1, 10) / 10.0f)});
        quadPoints.push_back({glm::vec3(max.x, min.y, 0.0f), glm::vec4(randomInt(0, 1), randomInt(0, 1), randomInt(0, 1), randomInt(1, 10) / 10.0f)});
    }

    return quadPoints;
}

using Draw::ColorQuadRenderBuffer;

struct GuiRenderer {
    ColorQuadRenderBuffer* quadRenderer;
    TextRenderer* textRenderer;
    Rect screen;
    float backgroundZ;
    float foregroundZ;
    float scale;

    #define CONST(name, val) const auto name = (val * scale)

    void colorRect(FRect rect, SDL_Color color) {
        ColorQuadRenderBuffer::UniformQuad2D quad;
        quad.color = colorConvert(color);
        glm::vec2* positions = quad.positions;
        positions[0] = {rect.x,        rect.y,      };
        positions[1] = {rect.x+rect.w, rect.y,      };
        positions[2] = {rect.x+rect.w, rect.y+rect.h};
        positions[3] = {rect.x,        rect.y+rect.h};
        quadRenderer->bufferUniform(1, &quad, backgroundZ);
    }

    void textBox(FRect rect, const My::StringBuffer& textBuffer, int maxLines, My::Vec<glm::vec4> textColors, float textScale = 1.0f) {
        if (textBuffer.empty()) return;
        const float padX = 20 * scale;
        const float padY = 30 * scale;
        const float minWidth = 50.0f;
        static TextFormattingSettings formatting = {
            TextAlignment::BottomLeft
        };
        static TextRenderingSettings rendering = {
            glm::vec4{0, 0, 0, 1}
        };
        formatting.maxWidth = (float)rect.w;

    
        /*
        FRect textBox = {
            textRect.x - padX,
            textRect.y - padY,
            textRect.w + padX*2,
            textRect.h + padY*2
        };
        this->colorRect(textBox, SDL_Color{50, 50, 50, 200});
        
        this->quadRenderer->flush();
        */
        colorRect({20.f, 20.f, 100.f, 100.f}, SDL_Color{50, 50, 50, 200});
        quadRenderer->flush();
        
        /*
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        */

        //glDisable(GL_BLEND);
        //glEnable(GL_DEPTH_TEST);
        //glDepthMask(GL_TRUE);

        FRect textRect = {rect.x + padX, rect.y + padY, minWidth, 0.0f};

        
        FRect textRect = textRenderer->renderStringBufferAsLinesReverse(textBuffer, 3, glm::vec2(rect.x+padX, rect.y+padY), textColors, glm::vec2(scale * textScale), formatting);
        textRenderer->flush();
        
        // DONT KNOW WHY TEXT IS STILL BEING BLENDED WITH THE QUAD
        // TODO : render everything in order 
    }

    void flush() {
        quadRenderer->flush();
        textRenderer->flush();
    }
};

void renderTextStuff(RenderContext& ren, const Camera& camera, const glm::mat4& screenTransform, const GUI::Gui* gui) {

    auto& textRenderer = ren.textRenderer;
    auto textShader = ren.textRenderer.shader;
    textShader.use();
    textShader.setMat4("transform", glm::translate(screenTransform, glm::vec3{0.0f, 0.0f, 0.9f}));

    textRenderer.font = &ren.debugFont;
    TextFormattingSettings& textSettings = textRenderer.defaultFormatting;
    textSettings.maxWidth = INFINITY;
    textSettings.wrapOnWhitespace = false;
    textSettings.align = TextAlignment::MiddleCenter;
    auto textRenderSettings = textRenderer.defaultRendering;
    textRenderSettings.color = {0, 0, 0, 1};
    textRenderSettings.scale = {1.0f, 1.0f};
    textRenderer.render("Howdy pal!    ", glm::vec2(camera.pixelWidth/2.0f, camera.pixelHeight/2.0f + 150.0f));
    
    /*
    textRenderer.render("Howdy pal! jdfkjasd", glm::vec2(camera.pixelWidth/2.0f, camera.pixelHeight/2.0f), textSettings, textRenderSettings);
    textRenderer.render("This is ubuntu with regular text!", glm::vec2{600, 500}, textSettings, textRenderSettings);
    textRenderer.render("this is centered\nAt the top! THis is even more garbage text!!!", {camera.pixelWidth/2.0f, camera.pixelHeight-100.0f}, textSettings, textRenderSettings);
    textRenderer.render("hello\nworld!\nWhats up guys  \n\n!", glm::vec2{100, 100}, textSettings, textRenderSettings);
    */
    
    static char fpsCounter[128];
    if (Metadata->ticks() % 10 == 0) {
        snprintf(fpsCounter, 128, "FPS: %.1f", (float)Metadata->fps());
    }
    
    textRenderer.render(fpsCounter, {10, camera.pixelWidth - 70}, 
        TextFormattingSettings(TextAlignment::TopLeft), 
        TextRenderingSettings(glm::vec4{1,0,0,1}, glm::vec2{2.0f}));
    textRenderer.flush();

    auto colorShader = ren.quadRenderer.shader;
    colorShader.use();
    colorShader.setMat4("transform", screenTransform);
    
    GuiRenderer guiRenderer;
    guiRenderer.quadRenderer = &ren.quadRenderer;
    guiRenderer.textRenderer = &ren.textRenderer;
    guiRenderer.scale = 1.0f;
    // not sure how to convert float to int here, rounding for now. Should pixelWidth/height even be anything other than a whole number?
    guiRenderer.screen = {0, 0, (int)round(camera.pixelWidth), (int)round(camera.pixelHeight)};
    guiRenderer.backgroundZ = 0.8f;
    guiRenderer.foregroundZ = 0.5f;
    textSettings.align = TextAlignment::BottomLeft;
    auto textRect = textRenderer.render(gui->console.activeMessage.c_str(), glm::vec2{20, 20}, textSettings, textRenderSettings);
    guiRenderer.textBox({0, textRect.h + 10, 300, 6942069}, gui->console.text, 3, gui->console.textColors, 0.5f);
    guiRenderer.flush();
}

static void renderTexture(Shader textureShader, GLuint texture) {
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Single);
    glBindTexture(GL_TEXTURE_2D, texture);

    const auto p = glm::vec3{0, 0, 0.99f};
    const float w = 100;
    const float h = 100;
    const float rotation = 0.0f;

    constexpr int numVertices = 6;
    const GLfloat vertexPositions[3 * numVertices] = {
        p.x,     p.y,     p.z, // bottom left
        p.x,     p.y+h,   p.z, // top left
        p.x+w,   p.y+h,   p.z, // top right
        p.x+w,   p.y+h,   p.z, // top right
        p.x+w,   p.y,     p.z, // bottom right
        p.x,     p.y,     p.z  // bottom left
    };
    const GLfloat vertexTexCoords[2 * numVertices] = {
        0.0f,  0.0f,
        0.0f,  1.0f,
        1.0f,  1.0f, 
        1.0f,  1.0f, 
        1.0f,  0.0f,
        0.0f,  0.0f
    };
    const GLfloat vertexRotations[1 * numVertices] = {
        rotation,
        rotation,
        rotation,
        rotation,
        rotation,
        rotation
    };

    constexpr VertexAttribute attributes[] = {
        {3, GL_FLOAT, sizeof(GLfloat)}, // position
        {2, GL_FLOAT, sizeof(GLfloat)}, // tex coord
        {1, GL_FLOAT, sizeof(GLfloat)} // rotation
    };

    const void* attributeVertices[] = {vertexPositions, vertexTexCoords, vertexRotations}; // order needs to be same as attribute order

    auto model = makeModelSOA(numVertices, attributeVertices, GL_STREAM_DRAW, attributes, 3);
    glBindVertexArray(model.VAO);

    textureShader.use();

    glDrawArrays(GL_TRIANGLES, 0, numVertices);

    model.destroy();
}

void render(RenderContext& ren, float scale, Gui* gui, GameState* state, Camera& camera, Vec2 playerTargetPos) {
    GL::logErrors();    

    //LogInfo("drawable width: %d; camera pixel width: %f", drawableWidth, camera.pixelWidth);

    const glm::mat4 screenTransform = glm::ortho(0.0f, (float)camera.pixelWidth, 0.0f, (float)camera.pixelHeight);
    const IVec2 screenMin = {0, 0}; (void)screenMin;
    const IVec2 screenMax = {camera.pixelWidth, camera.pixelHeight};
    
    const glm::mat4 worldTransform = camera.getTransformMatrix(); // or camTransform
    const Vec2 cameraMin = camera.minCorner();
    const Vec2 cameraMax = camera.maxCorner();

    ren.shaders.use(Shaders::Color).setMat4("transform", screenTransform);
    ren.shaders.use(Shaders::Tilemap).setMat4("transform", worldTransform);
    auto textureShader = ren.shaders.use(Shaders::Texture);
    textureShader.setMat4("transform", screenTransform);
    textureShader.setInt("tex", TextureUnit::Text);

    /* Start Rendering */
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render tilemap
    {
        const Vec2 minChunkRelativePos = {cameraMin.x / CHUNKSIZE, cameraMin.y / CHUNKSIZE};
        const Vec2 maxChunkRelativePos = {cameraMax.x / CHUNKSIZE, cameraMax.y / CHUNKSIZE};
        const IVec2 minChunkPos = {(int)floor(minChunkRelativePos.x), (int)floor(minChunkRelativePos.y)};
        const IVec2 maxChunkPos = {(int)floor(maxChunkRelativePos.x), (int)floor(maxChunkRelativePos.y)};
        const IVec2 viewportChunkSize = {maxChunkPos.x - minChunkPos.x, maxChunkPos.y - minChunkPos.y};

        auto& chunkModel = ren.chunkModel;
        glBindVertexArray(chunkModel.VAO);
        GL::logErrors();

        ren.shaders.use(Shaders::Tilemap);

        int numRenderedChunks = (viewportChunkSize.x+1) * (viewportChunkSize.y+1);
        for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
            for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
                ChunkData* chunkdata = state->chunkmap.get({x, y});
                if (!chunkdata) {
                    // LogInfo("Chunk wasn't generated while rendering at (%d,%d)!", x, y);
                    ChunkData* newChunk = state->chunkmap.newChunkAt({x, y});
                    if (newChunk) {
                        generateChunk(newChunk);
                        chunkdata = newChunk;
                    } else {
                        LogError("Failed to create missing chunk at tile (%d,%d) for rendering", x*CHUNKSIZE, y*CHUNKSIZE);
                        // perhaps this should render some missing texture thing, but atleast for now just render nothing
                        continue;
                    }
                    
                }
                renderChunk(ren, *chunkdata, chunkModel);
            }
        }

        int numRenderedTiles = numRenderedChunks * CHUNKSIZE*CHUNKSIZE;
        //LogInfo("num rendered tiles: %d", numRenderedTiles);
        (void)numRenderedTiles;
    }

    static RenderSystem renderSystem = RenderSystem();
    renderSystem.Update(state->ecs, state->chunkmap, ren, camera);

    GL::logErrors();

    // only need blending for points and stuff, not entities or tilemap
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    auto quadShader = ren.quadRenderer.shader;
    quadShader.use();
    quadShader.setMat4("transform", worldTransform);
    Draw::chunkBorders(ren.quadRenderer, camera, {1, 0, 0, 1}, 4.0f, 0.5f);
    
    static auto arr = makeDemoQuads(ren.window);
    ren.quadRenderer.buffer(arr.size() / 4, arr.data());
    
    ren.quadRenderer.flush();
    
    GL::logErrors();

    // for text, depth mask off, dept

    // GL_BLEND is required for text, if text are boxes, that's probably why
    //glDisable(GL_BLEND);

    //glActiveTexture(GL_TEXTURE0 + TextureUnit::Text);

    renderTextStuff(ren, camera, screenTransform, gui);

    //renderTexture(ren.shaders.get(Shaders::Texture), g.textTexture);

    /* Done rendering */
    SDL_GL_SwapWindow(ren.window);

    GL::logErrors();
}