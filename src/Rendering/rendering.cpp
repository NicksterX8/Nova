#include "rendering/rendering.hpp"
#include "rendering/systems/simple.hpp"
#include "Chunks.hpp"
#include "utils/Debug.hpp"
#include "utils/Log.hpp"
#include "utils/random.hpp"
#include "My/Vec.hpp"
#include "utils/vectors.hpp"
#include "llvm/ArrayRef.h"

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

static Shader loadShader(const char* name) {
    return Shader(compileShaderProgram(FileSystem.shaders.get(str_add(name, ".vs")), FileSystem.shaders.get(str_add(name, ".fs"))));
}

static int loadShaders(RenderContext& ren) {
    ren.entityShader = loadShader("entity");
    ren.tilemapShader = loadShader("tilemap");
    ren.pointShader = loadShader("point");
    ren.colorShader = loadShader("color");
    ren.textShader = loadShader("text");
    ren.sdfShader = loadShader("sdf");
    
    return 0;
}

void renderInit(RenderContext& ren) {
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Text);

    initFreetype();
    ren.font = Font(6.0f, 2.0f);
    ren.font.load(FileSystem.assets.get("fonts/FreeSans.ttf"), 128, true);
    ren.debugFont = Font(7.0f, 2.0f);
    ren.debugFont.load(FileSystem.assets.get("fonts/Ubuntu-Regular.ttf"), 32, false);

    glActiveTexture(GL_TEXTURE0 + TextureUnit::MyTextureArray);

    loadShaders(ren);
    ren.textureArray = makeTextureArray(FileSystem.assets.get());
    ren.tilemapShader.use();
    ren.tilemapShader.setInt("texArray", TextureUnit::MyTextureArray);
    ren.entityShader.use();
    ren.entityShader.setInt("texArray", TextureUnit::MyTextureArray);
    ren.textShader.use();
    ren.textShader.setInt("text", TextureUnit::Text);
    ren.sdfShader.use();
    ren.sdfShader.setInt("text", TextureUnit::Text);

    ren.textRenderer = TextRenderer::init(&ren.debugFont, ren.textShader);
    ren.textRenderer.defaultFormatting = TextFormattingSettings::Default();

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
}

void renderQuit(RenderContext& ren) {
    /* Text rendering systems quit */
    ren.font.unload();
    ren.debugFont.unload();
    ren.textRenderer.destroy();
    quitFreetype();

    /* Shaders quit */
    ren.entityShader.destroy();
    ren.tilemapShader.destroy();
    ren.pointShader.destroy();
    ren.textShader.destroy();
    ren.chunkModel.destroy();

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

    return std::move(quadPoints);
}

using Draw::ColorQuadRenderBuffer;

struct GuiRenderer {
    ColorQuadRenderBuffer* quadRenderer;
    TextRenderer* textRenderer;
    Rect screen;
    float z;
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
        quadRenderer->bufferUniform(1, &quad, z);
    }

    void textBox(FRect rect, const My::StringBuffer& textBuffer, My::Vec<glm::vec4> textColors) {
        if (textBuffer.empty()) return;
        const float padX = 20 * scale;
        const float padY = 30 * scale;
        static TextFormattingSettings formatting = {
            TextAlignment::BottomLeft
        };
        static TextRenderingSettings rendering = {
            glm::vec4{0, 0, 0, 1}
        };
        formatting.maxWidth = (float)rect.w;
        FRect textRect = textRenderer->renderStringBufferAsLinesReverse(textBuffer, glm::vec2(rect.x+padX, rect.y+padY), textColors, glm::vec2(scale), formatting);
        FRect textBox = {
            textRect.x - padX,
            textRect.y - padY,
            textRect.w + padX*2,
            textRect.h + padY*2
        };
        textRenderer->flush();
        this->colorRect(textBox, SDL_Color{50, 50, 50, 200});
        this->quadRenderer->flush();
    }

    void flush(Shader quadShader) {
        quadShader.use();
        quadRenderer->flush();
        textRenderer->flush();
    }
};

void renderTextBox(GuiRenderer& renderer) {
    
}

void render(RenderContext& ren, float scale, Gui* gui, GameState* state, Camera& camera, Vec2 playerTargetPos) {
    glActiveTexture(GL_TEXTURE0 + TextureUnit::MyTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ren.textureArray);

    /* Start Rendering */
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int drawableWidth,drawableHeight;
    SDL_GL_GetDrawableSize(ren.window, &drawableWidth, &drawableHeight);

    IVec2 screenMin = {0, 0}; (void)screenMin;
    IVec2 screenMax = {drawableWidth, drawableHeight};

    GL::logErrors();

    auto camTransform = camera.getTransformMatrix();
    ren.tilemapShader.use();
    ren.tilemapShader.setMat4("transform", camTransform);

    auto& chunkModel = ren.chunkModel;

    glBindVertexArray(chunkModel.VAO);

    Vec2 cameraMin = camera.minCorner();
    Vec2 cameraMax = camera.maxCorner();

    Vec2 minChunkRelativePos = {cameraMin.x / CHUNKSIZE, cameraMin.y / CHUNKSIZE};
    Vec2 maxChunkRelativePos = {cameraMax.x / CHUNKSIZE, cameraMax.y / CHUNKSIZE};
    IVec2 minChunkPos = {(int)floor(minChunkRelativePos.x), (int)floor(minChunkRelativePos.y)};
    IVec2 maxChunkPos = {(int)floor(maxChunkRelativePos.x), (int)floor(maxChunkRelativePos.y)};
    IVec2 viewportChunkSize = {maxChunkPos.x - minChunkPos.x, maxChunkPos.y - minChunkPos.y};

    int numRenderedChunks = (viewportChunkSize.x+1) * (viewportChunkSize.y+1);

    for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
        for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
            ChunkData* chunkdata = state->chunkmap.get({x, y});
            if (!chunkdata) {
                ChunkData* newChunk = state->chunkmap.newChunkAt({x, y});
                if (newChunk) {
                    LogError("Chunk wasn't generated while rendering at (%d,%d)!", x, y);
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

    static RenderSystem renderSystem = RenderSystem();
    renderSystem.Update(state->ecs, state->chunkmap, ren, camera);

    My::Vec<Draw::ColoredPoint> points(0);
    points.push({{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}, 15.0f});
    auto r1 = camera.minCorner();
    auto r3 = camera.maxCorner();
    points.push({glm::vec3(r1.x + 1, r1.y + 1, 0.0f), {0, 1, 0, 1}, 8.0f});
    points.push({glm::vec3(r3.x - 1, r3.y - 1, 0.0f), {0, 1, 0, 1}, 8.0f});
    points.push({glm::vec3(getMouseWorldPosition(camera), 0.0f), {0, 1, 1, 1}, 9.0f});

    static Draw::ColorQuadRenderBuffer quadRenderer = Draw::ColorQuadRenderBuffer();

    glm::mat4 quadTransform;
    glm::mat4 screenTransform = glm::ortho(0.0f, (float)drawableWidth, 0.0f, (float)drawableHeight);
    if (Debug->settings.drawChunkBorders) {
        quadTransform = screenTransform;
    } else {
        quadTransform = camTransform;
    }

    ren.colorShader.use();
    ren.colorShader.setMat4("transform", quadTransform);

    // only need blending for points and stuff, not entities or tilemap
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glDisable(GL_DEPTH_TEST);

    Draw::chunkBorders(quadRenderer, camera, {1, 0, 0, 1}, 4.0f, 0.5f);

    auto arr = makeDemoQuads(ren.window);

    quadRenderer.flush();

    GL::logErrors();

    glActiveTexture(GL_TEXTURE0 + TextureUnit::Text);

    auto textShader = ren.textShader;
    auto sdfShader = ren.sdfShader;
    textShader.use();
    textShader.setMat4("transform", screenTransform);

    sdfShader.use();
    sdfShader.setMat4("transform", screenTransform);
    sdfShader.setFloat("thickness", 0.5f);
    sdfShader.setFloat("soft", 0.01f);

    auto& textRenderer = ren.textRenderer;
    textRenderer.shader = textShader;

    textShader.use();

    glDepthMask(GL_FALSE);
    textRenderer.font = &ren.debugFont;
    TextFormattingSettings& textSettings = textRenderer.defaultFormatting;
    textSettings.maxWidth = INFINITY;
    textSettings.wrapOnWhitespace = false;
    textSettings.align = TextAlignment::MiddleCenter;
    auto textRenderSettings = textRenderer.defaultRendering;
    textRenderSettings.color = {0, 0, 0, 1};
    textRenderSettings.scale = {1.0f, 1.0f};
    textRenderer.render("Howdy pal!    ", glm::vec2(drawableWidth/2.0f, drawableHeight/2.0f + 150.0f));

    //textRenderer.render("Howdy pal! jdfkjasd", glm::vec2(drawableWidth/2.0f, drawableHeight/2.0f), otherSettings);
    //textRenderer.render("This is ubuntu with regular text!", glm::vec2{600, 500}, otherSettings2);
    //textRenderer.render("this is centered\nAt the top! THis is even more garbage text!!!", {drawableWidth/2.0f, drawableHeight-100.0f}, otherSettings2);
    //textRenderer.render("hello\nworld!\nWhats up guys  \n\n!", glm::vec2{100, 100}, otherSettings3);

    static char fpsCounter[128];
    if (Metadata->ticks() % 10 == 0) {
        snprintf(fpsCounter, 128, "FPS: %.1f", (float)Metadata->fps());
    }
    textRenderer.render(fpsCounter, {10, drawableHeight - 70}, 
        TextFormattingSettings(TextAlignment::TopLeft), 
        TextRenderingSettings(glm::vec4{1,0,0,1}, glm::vec2{2.0f}));
    textRenderer.flush();

    GuiRenderer guiRenderer;
    guiRenderer.quadRenderer = &quadRenderer;
    guiRenderer.textRenderer = &textRenderer;
    guiRenderer.scale = 1.0f;
    guiRenderer.screen = {0, 0, drawableWidth, drawableHeight};
    guiRenderer.z = 0.0f;
    guiRenderer.textBox({50, 50, 200, 100}, gui->console.text, gui->console.textColors);
    ren.colorShader.use();
    ren.colorShader.setMat4("transform", screenTransform);
    guiRenderer.flush(ren.colorShader);

    glDepthMask(GL_TRUE);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    SDL_GL_SwapWindow(ren.window);

    GL::logErrors();
}