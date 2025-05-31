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
#include "rendering/drawing.hpp"

void scaleAllFonts(FontManager& fontManager, float scale) {
    for (auto font : fontManager.fonts) {
        if (font.second) {
            font.second->scale(font.second->currentScale * scale / fontManager.fontScale);
        }
    }
    fontManager.fontScale = scale;
}

void renderWater(RenderContext& ren, const Camera& camera, Vec2 min, Vec2 max, float time) {
    auto shader = ren.shaders.use(Shaders::Water);
    shader.setFloat("iTime", time);
    //shader.setFloat("surface_y", 400.0f);
    int width,height;
    SDL_GL_GetDrawableSize(ren.window, &width, &height);
    //shader.setVec2("iResolution", Vec2(width, height));

    float z = getLayerHeight(RenderLayer::Water);
    glm::vec3 p1 = {min.x, min.y, z};
    glm::vec3 p2 = {max.x, max.y, z};
    glm::vec4 color = {1.0, 1.0, 1.0, 1.0};
    float scale = 2.0;
    shader.setFloat("scale", camera.worldScale());
    if (camera.worldScale() < 12.0f) {
        //scale *= 12.0f / camera.worldScale();
    }
    Vec2 timeMov = Vec2(time / 6.0f, time / 12.0f);
    glm::vec2 texMin = min / scale + timeMov;
    glm::vec2 texMax = max / scale + timeMov;

    static GlVertexAttribute attributes[] = {
        {3, GL_FLOAT, sizeof(GLfloat)},
        {4, GL_FLOAT, sizeof(GLfloat)},
        {2, GL_FLOAT, sizeof(GLfloat)},
    };
    static GlVertexFormat format = GlMakeVertexFormat(0, attributes);

    float vertices[6 * 9] = {
        p1.x, p1.y, p1.z, color.r, color.g, color.b, color.a, texMin.x, texMin.y, // bottom left
        p2.x, p1.y, p1.z, color.r, color.g, color.b, color.a, texMax.x, texMin.y, // bottom right
        p2.x, p2.y, p1.z, color.r, color.g, color.b, color.a, texMax.x, texMax.y, // top right
        p2.x, p2.y, p1.z, color.r, color.g, color.b, color.a, texMax.x, texMax.y, // top right
        p1.x, p2.y, p1.z, color.r, color.g, color.b, color.a, texMin.x, texMax.y, // top left
        p1.x, p1.y, p1.z, color.r, color.g, color.b, color.a, texMin.x, texMin.y, // bottom left
    };
    GlBuffer buffer = {6 * 9 * sizeof(float), vertices, GL_STREAM_DRAW};
    GlModel model = makeModel(buffer, format);
    model.bindAll();

    glDrawArrays(GL_TRIANGLES, 0, 6);

    model.destroy();
}

constexpr int numChunkVerts = CHUNKSIZE * CHUNKSIZE * 1;

TextureAtlas::Space TileTextureSpaces[TileTypes::Count];

static void renderChunk(RenderContext& ren, const Camera& camera, Chunk* chunk, ChunkCoord pos, glm::vec2* vertexPositions, GLushort* vertexTexCoords) {
    assert(vertexPositions && vertexTexCoords);

    if (!chunk) return;
    
    const auto* atlas = &ren.textureAtlas;
    
    Vec2 startPos = pos * CHUNKSIZE;

    constexpr float tileWidth = 1.0f;
    constexpr float tileHeight = 1.0f;

    // coords
    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            int index = row * CHUNKSIZE + col;
            float x = startPos.x + col * tileWidth;
            float y = startPos.y + row * tileHeight;
            vertexPositions[index] = {x, y};
        }
    }

    // backgrounds
    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            int index = (row * CHUNKSIZE + col) * 4;
            TileType type = (*chunk)[row][col].type;
            auto texSpace = TileTextureSpaces[type];

            vertexTexCoords[index + 0] = texSpace.min.x;
            vertexTexCoords[index + 1] = texSpace.min.y;
            vertexTexCoords[index + 2] = texSpace.max.x;
            vertexTexCoords[index + 3] = texSpace.max.y;
        }
    }

    // animations
    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            int index = (row * CHUNKSIZE + col) * 4;
            TileType type = (*chunk)[row][col].type;
            Animation& animation = TileTypeData[type].animation;
            if (!animation.texture) continue;
            auto texSpace = getAnimationFrameFromAtlas(*atlas, animation);

            vertexTexCoords[index + 0] = texSpace.min.x;
            vertexTexCoords[index + 1] = texSpace.min.y;
            vertexTexCoords[index + 2] = texSpace.max.x;
            vertexTexCoords[index + 3] = texSpace.max.y;
        }
    }
}

int renderTilemap(RenderContext& ren, const Camera& camera, ChunkMap* chunkmap) {
    assert(isValidEntityPosition(camera.position));

    Boxf maxBoundingArea = camera.maxBoundingArea();
    Vec2 minChunkRelativePos = maxBoundingArea[0] / (float)CHUNKSIZE;
    Vec2 maxChunkRelativePos = maxBoundingArea[1] / (float)CHUNKSIZE;

    const IVec2 minChunkPos = {(int)floor(minChunkRelativePos.x), (int)floor(minChunkRelativePos.y)};
    const IVec2 maxChunkPos = {(int)floor(maxChunkRelativePos.x), (int)floor(maxChunkRelativePos.y)};
    const IVec2 viewportChunkSize = {maxChunkPos.x - minChunkPos.x, maxChunkPos.y - minChunkPos.y};

    struct ChunkPosPair {
        Chunk* chunk;
        IVec2 chunkCoord;
    };

    llvm::SmallVector<ChunkPosPair> chunks;

    ren.shaders.use(Shaders::Tilemap);

    int numRenderedChunks = 0;
    int drawCalls = 0;

    auto& chunkModel = ren.chunkModel;
    glBindVertexArray(chunkModel.vao);

    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.positionVbo);
    glm::vec2* vertexPositions = (glm::vec2*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.texCoordVbo);
    GLushort* vertexTexCoords = (GLushort*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    
    for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
        for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
            if (!camera.rectIsVisible({x * CHUNKSIZE, y * CHUNKSIZE}, {(x+1) * CHUNKSIZE, (y+1) * CHUNKSIZE})) {
                continue;
            }
            ChunkData* chunkdata = chunkmap->get({x, y});
            if (!chunkdata) {
                ChunkData* newChunk = chunkmap->newChunkAt({x, y});
                if (newChunk) {
                    generateChunk(newChunk);
                    chunkdata = newChunk;
                } else {
                    LogError("Failed to create missing chunk at tile (%d,%d) for rendering", x*CHUNKSIZE, y*CHUNKSIZE);
                    // perhaps this should render some missing texture thing, but atleast for now just render nothing
                    continue;
                }
            }

            chunks.push_back({chunkdata->chunk, {x, y}});
        }
    }

    std::sort(chunks.begin(), chunks.end(), [](ChunkPosPair a, ChunkPosPair b){
        return a.chunk < b.chunk;
    });

    for (int i = 0; i < TileTypes::Count; i++) {
        TextureID texture = TileTypeData[i].background;
        TileTextureSpaces[i] = getTextureAtlasSpace(&ren.textureAtlas, texture);
    }

    int chunksBuffered = 0;
    for (auto chunkAndPos : chunks) {
        if (chunksBuffered >= ChunkBuffer::bufferChunkCapacity) {
            // render a full batch of chunks
            glBindBuffer(GL_ARRAY_BUFFER, chunkModel.positionVbo);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, chunkModel.texCoordVbo);
            glUnmapBuffer(GL_ARRAY_BUFFER);

            glDrawArrays(GL_POINTS, 0, chunksBuffered * numChunkVerts);

            glBindBuffer(GL_ARRAY_BUFFER, chunkModel.positionVbo);
            vertexPositions = (glm::vec2*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            glBindBuffer(GL_ARRAY_BUFFER, chunkModel.texCoordVbo);
            vertexTexCoords = (GLushort*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

            chunksBuffered = 0;
            drawCalls++;
        }

        size_t index = chunksBuffered;

        auto* chunkVertexPositions = vertexPositions + index * numChunkVerts; 
        auto* chunkVertexTexCoords = vertexTexCoords + index * numChunkVerts * 4;

        renderChunk(ren, camera, chunkAndPos.chunk, chunkAndPos.chunkCoord, chunkVertexPositions, chunkVertexTexCoords);

        chunksBuffered++;
    }

    
    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.positionVbo);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.texCoordVbo);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    
    if (chunksBuffered > 0) {
        glDrawArrays(GL_POINTS, 0, chunksBuffered * numChunkVerts);
        chunksBuffered = 0;
        drawCalls++;
    }
    

    glBindVertexArray(0);

    return numRenderedChunks;
}

static GlModelSOA makeScreenModel() {
    constexpr int numVertices = 6;
    float z = -0.9f;
    const GLfloat vertexPositions[3 * numVertices] = {
        -1.0,   -1.0,   z, // bottom left
        -1.0,    1.0,   z, // top left
         1.0,    1.0,   z, // top right
         1.0,    1.0,   z, // top right
         1.0,   -1.0,   z, // bottom right
        -1.0,   -1.0,   z, // bottom left
    };
    const GLfloat vertexTexCoords[2 * numVertices] = {
        0.0f,  0.0f,
        0.0f,  1.0f,
        1.0f,  1.0f, 
        1.0f,  1.0f, 
        1.0f,  0.0f,
        0.0f,  0.0f
    };

    auto vertexFormat = GlMakeVertexFormat(0, {
        {3, GL_FLOAT, sizeof(GLfloat)}, // position
        {2, GL_FLOAT, sizeof(GLfloat)}, // tex coord
        {2, GL_FLOAT, sizeof(GLfloat)}, // velocity
    });

    const void* attributeVertices[] = {vertexPositions, vertexTexCoords, nullptr}; // order needs to be same as attribute order

    auto model = makeModelSOA(numVertices, attributeVertices, GL_STREAM_DRAW, vertexFormat);
    return model;
}

int setConstantShaderUniforms(RenderContext& ren) {
    auto& mgr = ren.shaders;

    auto entity = mgr.use(Shaders::Entity);
    entity.setInt("texAtlas", TextureUnit::MyTextureAtlas);
    entity.setVec2("texAtlasSize", ren.textureAtlas.size);
    glBindFragDataLocation(entity.id, 0, "FragColor");
    glBindFragDataLocation(entity.id, 1, "FragVelocity");

    auto tilemap = mgr.use(Shaders::Tilemap);
    tilemap.setInt("tex", TextureUnit::MyTextureAtlas);
    tilemap.setVec2("texSize", ren.textureAtlas.size);
    tilemap.setFloat("height", getLayerHeight(RenderLayer::Tilemap));
    
    mgr.use(Shaders::Text).setInt("text", TextureUnit::Font0);

    auto sdf = mgr.use(Shaders::SDF);
    sdf.setInt("text", TextureUnit::Font1);
    sdf.setFloat("thickness", 1.0f);
    sdf.setFloat("soft", 0.9f);
    sdf.setFloat("smoothing", 0.001f);

    mgr.use(Shaders::Texture).setInt("tex", TextureUnit::Random);

    auto quad = mgr.use(Shaders::Quad);
    quad.setInt("tex", TextureUnit::GuiAtlas);
    quad.setVec2("texSize", glm::vec2(ren.guiRenderer.guiAtlas.size));

    mgr.use(Shaders::Screen).setInt("tex", TextureUnit::Screen0);

    return GL::logErrors();
}

// returns number of errors
int createShaders(ShaderManager& mgr) {

    auto entity = mgr.setup(Shaders::Entity, "entity");
    auto tilemap = mgr.setup(Shaders::Tilemap, "tilemap");
    auto text = mgr.setup(Shaders::Text, "text");
    auto sdf = mgr.setup(Shaders::SDF, "sdf");
    auto texture = mgr.setup(Shaders::Texture, "texture");
    auto quad = mgr.setup(Shaders::Quad, "quad");
    auto point = mgr.setup(Shaders::Point, "point");
    auto screen = mgr.setup(Shaders::Screen, "screen");
    auto water = mgr.setup(Shaders::Water, "water");

    return GL::logErrors();
}

Font* makeFont(const char* name, const char* font_filename, FT_UInt height, bool sdf, TextureUnit texUnit, FontManager& fonts, const ShaderManager& shaders) {
    Font::FormattingSettings fontFormatting = {
        .lineSpacing = 1.0f,
        .tabSpaces = 5.0f
    };
    auto font = newFont(
        My::str_add(FileSystem.assets.get("fonts/"), font_filename), height, sdf, 
        ASCII_FIRST_STANDARD_CHAR, ASCII_LAST_STANDARD_CHAR+1,
        1.0f, fontFormatting,
        texUnit, sdf ? shaders.get(Shaders::SDF) : shaders.get(Shaders::Text));
    fonts.add(name, font);
    return font;
}

void initFonts(FontManager& fonts, const ShaderManager& shaders) {
    Font::FormattingSettings fontFormatting = {
        .lineSpacing = 1.0f,
        .tabSpaces = 5.0f
    };

    constexpr char firstChar = ASCII_FIRST_STANDARD_CHAR;
    constexpr char endChar = ASCII_LAST_STANDARD_CHAR+1;

    Font* defaultFont = newFont(
        FileSystem.assets.get("fonts/Ubuntu-Regular.ttf"),
        24,
        false,
        firstChar, endChar,
        1,
        fontFormatting,
        TextureUnit::Font0,
        shaders.get(Shaders::Text)
    );
    Font* debugFont = newFont(
        FileSystem.assets.get("fonts/Cascadia.ttf"),
        24, // size
        false, // use sdfs?
        firstChar, endChar, // first char (space), last char
        1, // scale
        fontFormatting, // formatting settings
        TextureUnit::Font1,
        shaders.get(Shaders::Text)
    );
    Font* worldFont = newFont(
        FileSystem.assets.get("fonts/Papyrus.ttf"),
        48,
        true,
        32, 127,
        1,
        fontFormatting,
        TextureUnit::Font2,
        shaders.get(Shaders::SDF)
    );
    Font* guiFont = newFont(
        FileSystem.assets.get("fonts/factorio-fonts/TitilliumWeb-Bold.ttf"),
        24,
        false,
        firstChar,
        endChar,
        1,
        fontFormatting,
        TextureUnit::Font3,
        shaders.get(Shaders::Text)
    );
    Font* screensdf = makeFont("tester", "Roboto-Regular.ttf", 24, true, TextureUnits::Font4, fonts, shaders);

    fonts.add("Default", defaultFont);
    fonts.add("Debug", debugFont);
    fonts.add("World", worldFont);
    fonts.add("Gui", guiFont);

    float fontScale = 1.0f;
    scaleAllFonts(fonts, SDL::pixelScale * fontScale);
}

void renderInit(RenderContext& ren, int screenWidth, int screenHeight) {
    createShaders(ren.shaders);

    /* Init textures */
    ren.textures = TextureManager(TextureIDs::NumTextureSlots);
    setTextureMetadata(&ren.textures);
    ren.textureArray = makeTextureArray({256, 256}, &ren.textures, TextureTypes::World, FileSystem.assets.get(), TextureUnit::MyTextureArray);
    ren.textureAtlas = makeTextureAtlas(&ren.textures, TextureTypes::World, FileSystem.assets.get(), GL_NEAREST, GL_NEAREST, TextureUnit::MyTextureAtlas);
    
    /* Init text stuff */
    initFreetype();
    initFonts(ren.fonts, ren.shaders);
    Fonts = &ren.fonts;

    ren.guiTextRenderer = TextRenderer::init(ren.fonts.get("Debug"));
    ren.worldTextRenderer = TextRenderer::init(ren.fonts.get("World"));
    ren.worldTextRenderer.defaultRendering.scale = Vec2(1/32.0f);
    GL::logErrors();

    /* Init misc. renderers */
    ren.guiQuadRenderer = QuadRenderer(0);
    ren.worldQuadRenderer = QuadRenderer(0);

    TextureAtlas guiAtlas = makeTextureAtlas(&ren.textures, TextureTypes::Gui | TextureTypes::World, FileSystem.assets.get(), GL_LINEAR, GL_LINEAR, TextureUnit::GuiAtlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    RenderOptions guiOptions = {
        .size = {screenWidth, screenHeight}, // won't render if not set later
        .scale = 1.0f
    };
    ren.guiRenderer = GuiRenderer(&ren.guiQuadRenderer, &ren.guiTextRenderer, guiAtlas, guiOptions);
    RenderOptions worldGuiOptions = {
        .size = {INFINITY, INFINITY},
        .scale = 1
    };
    ren.worldGuiRenderer = GuiRenderer(&ren.worldQuadRenderer, &ren.worldTextRenderer, guiAtlas, worldGuiOptions);
    GL::logErrors();

    /* Tilemap rendering setup */
    auto& chunkModel = ren.chunkModel;
    
    chunkModel.vao = setupVAO();

    auto vertCount = ChunkBuffer::bufferChunkCapacity * numChunkVerts;
    const auto positionVboFormat = GlMakeVertexFormat(0, {
        {2, GL_FLOAT, sizeof(GLfloat)},
    });
    chunkModel.positionVbo = setupVBO(positionVboFormat, vertCount, NULL, GL_STREAM_DRAW);

    const auto texCoordVboFormat = GlMakeVertexFormat(1, {
        {4, GL_UNSIGNED_SHORT, sizeof(GLushort)}
    });
    chunkModel.texCoordVbo = setupVBO(texCoordVboFormat, vertCount, NULL, GL_STREAM_DRAW);

    GL::logErrors();

    /* Set default opengl rendering options */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glEnable(GL_PROGRAM_POINT_SIZE);

    ren.screenModel = makeScreenModel();
    ren.framebuffer = makeRenderBuffer({screenWidth, screenHeight});
    //resizeRenderBuffer(ren.framebuffer, {screenWidth, screenHeight});

    ren.renderSystem = new RenderSystem();

    setConstantShaderUniforms(ren);
}

void renderQuit(RenderContext& ren) {
    /* Text rendering systems quit */
    ren.guiTextRenderer.destroy();
    ren.guiQuadRenderer.destroy();
    ren.guiRenderer.destroy();
    ren.worldQuadRenderer.destroy();
    ren.worldTextRenderer.destroy();
    ren.worldGuiRenderer.destroy();

    quitFreetype();

    /* Fonts quit */
    ren.fonts.destroy();

    /* Shaders quit */
    ren.shaders.destroy();

    /* Textures quit */
    ren.textures.destroy();
    ren.textureArray.destroy();

    ren.chunkModel.destroy();
    ren.screenModel.destroy();

    destroyRenderBuffer(ren.framebuffer);
}

static llvm::SmallVector<ColorVertex, 0> makeDemoQuads() {
    auto quadPoints = llvm::SmallVector<ColorVertex, 0>();
    quadPoints.reserve(500 * 4);
 
    for (int i = 0; i < 500; i++) {
        Vec2 min = {randomInt(-100, 100), randomInt(-100, 100)};
        Vec2 max = min + Vec2{randomInt(1, 3), randomInt(1, 3)};
        
        quadPoints.push_back({glm::vec3(min.x, min.y, 0.0f), randomColor()});
        quadPoints.push_back({glm::vec3(min.x, max.y, 0.0f), randomColor()});
        quadPoints.push_back({glm::vec3(max.x, max.y, 0.0f), randomColor()});
        quadPoints.push_back({glm::vec3(max.x, min.y, 0.0f), randomColor()});
    }

    return quadPoints;
}

static void renderTexture(Shader textureShader, GLuint texture) {
    if (!texture) return;

    glActiveTexture(GL_TEXTURE0 + TextureUnit::Random);
    glBindTexture(GL_TEXTURE_2D, texture);

    const auto p = glm::vec3{0, 0, 0.99f};
    const float w = 500;
    const float h = 500;
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

    auto vertexFormat = GlMakeVertexFormat(0, {
        {3, GL_FLOAT, sizeof(GLfloat)}, // position
        {2, GL_FLOAT, sizeof(GLfloat)}, // tex coord
        {1, GL_FLOAT, sizeof(GLfloat)} // rotation
    });

    const void* attributeVertices[] = {vertexPositions, vertexTexCoords, vertexRotations}; // order needs to be same as attribute order

    auto model = makeModelSOA(numVertices, attributeVertices, GL_STREAM_DRAW, vertexFormat);
    glBindVertexArray(model.model.vao);

    textureShader.use();

    glDrawArrays(GL_TRIANGLES, 0, numVertices);

    model.destroy();
}

static void renderWorld(RenderContext& ren, Camera& camera, GameState* state, Vec2 playerTargetPos) {
    renderTilemap(ren, camera, &state->chunkmap);

    auto maxBoundingArea = camera.maxBoundingArea();
    float seconds = Metadata->frame.timestamp / 1000.0f;
    renderWater(ren, camera, maxBoundingArea[0], maxBoundingArea[1], seconds);

    ren.renderSystem->Update(state->ecs, state->chunkmap, ren, camera);

    /*
    const static auto demoQuads = makeDemoQuads();
    ren.worldQuadRenderer.render(demoQuads);

    ren.worldGuiRenderer.colorRect({0.0f, 0.0f}, {1.0f, 1.0f}, {255, 0, 0, 255});
    ren.worldGuiRenderer.flush(ren.shaders, camera.getTransformMatrix());
    */
}

// Binds the newly made texture on the active texture unit
GLuint makeTexture(glm::vec2 size, const void* data, GLenum type, GLint internalFormat, GLenum format) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.x, size.y, 0, format, type, data);
    return texture;
}

RenderBuffer makeRenderBuffer(glm::ivec2 size) {
    /* Make fbo */
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    /* Make color attachment (texture) */
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Screen0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);  

    /* Make texture for storing pixel velocities */
    // Old pixel position is stored for each pixel
    /*
    GLuint velocityTexture;
    glGenTextures(1, &velocityTexture);
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Screen1);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, velocityTexture, 0);  
    */

    /* Make depth attachment with render buffer */
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, size.x, size.y);

    /* fin? */
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LogError("frame buffer not complete!");
    }

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL::logErrors();

    return RenderBuffer{fbo, rbo, texture, 0};
}

void resizeRenderBuffer(RenderBuffer renderBuffer, glm::ivec2 newSize) {
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Screen0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, newSize.x, newSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    /*
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Screen1);
    glBindTexture(GL_TEXTURE_2D, renderBuffer.velocityTexture);
    char* zeroes = (char*)malloc(newSize.x * newSize.y * 24);
    //memset(zeroes, 0, newSize.x * newSize.y * 24);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, newSize.x, newSize.y, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
    free(zeroes);
    */

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, newSize.x, newSize.y);
    glActiveTexture(GL_TEXTURE0);

    GL::logErrors();

    LogInfo("Resized render buffer!");
}

void destroyRenderBuffer(RenderBuffer renderBuffer) {
    glDeleteRenderbuffers(1, &renderBuffer.rbo);
    glDeleteTextures(1, &renderBuffer.colorTexture);
    glDeleteTextures(1, &renderBuffer.velocityTexture);
    glDeleteFramebuffers(1, &renderBuffer.fbo);
}

void renderWorldRenderBuffer(RenderContext& ren, const Camera& camera) {
    glBindVertexArray(ren.screenModel.model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ren.screenModel.model.vbo);
    auto screenShader = ren.shaders.use(Shaders::Screen);

    glm::vec2 corners[4] = {
        camera.BottomLeftWorldPos(),
        camera.BottomRightWorldPos(),
        camera.TopRightWorldPos(),
        camera.TopLeftWorldPos()
    };

    static glm::vec2 oldCorners[4] = {
        corners[0], corners[1], corners[2], corners[3]
    };
    
    glm::vec2 velocities[4];
    for (int i = 0; i < 4; i++) {
        glm::vec2 pixelVelocity = glm::vec2(camera.worldToPixel(corners[i]) - camera.worldToPixel(oldCorners[i]));
        velocities[i] = glm::vec2(pixelVelocity.x / camera.pixelWidth, pixelVelocity.y / camera.pixelHeight);
    }

    memcpy(oldCorners, corners, sizeof(corners));

    glm::vec2 vertexVelocities[6] = {
        velocities[0], velocities[3], velocities[2],
        velocities[2], velocities[1], velocities[0]
    };

    size_t offset = ren.screenModel.attributeOffsets[2];
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vertexVelocities), vertexVelocities);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render(RenderContext& ren, RenderOptions options, Gui* gui, GameState* state, Camera& camera, const PlayerControls& controls, Mode mode, bool doRenderWorld) {
    GL::logErrors();    
    auto& shaders = ren.shaders;

    assert(camera.worldScale() > 0.0f);

    const glm::mat4 screenTransform = glm::ortho(0.0f, (float)camera.pixelWidth, 0.0f, (float)camera.pixelHeight);
    const IVec2 screenMin = {0, 0}; (void)screenMin;
    const IVec2 screenMax = {camera.pixelWidth, camera.pixelHeight};
    
    const glm::mat4 worldTransform = camera.getTransformMatrix(); // or camTransform
    const Vec2 cameraMin = camera.minCorner();
    const Vec2 cameraMax = camera.maxCorner();
    const Boxf maxBoundingArea = camera.maxBoundingArea();

    ren.shaders.use(Shaders::Quad).setMat4("transform", screenTransform);
    ren.shaders.use(Shaders::Tilemap).setMat4("transform", worldTransform);
    ren.shaders.use(Shaders::Water).setMat4("transform", worldTransform);
    auto textureShader = ren.shaders.use(Shaders::Texture);
    textureShader.setMat4("transform", screenTransform);

    /* Start Rendering */
    /* First pass */
    if (true) {
        glBindFramebuffer(GL_FRAMEBUFFER, ren.framebuffer.fbo);

        //glDrawBuffer(GL_COLOR_ATTACHMENT0)

        glDepthMask(GL_TRUE);
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        renderWorld(ren, camera, state, controls.mouseWorldPos);
    }

    /* Second pass */
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // only need blending for points and stuff, not entities or tilemap
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    // can't use blending and depth tests at once
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    renderWorldRenderBuffer(ren, camera);

    /* GUI rendering */

    auto quadShader = shaders.use(Shaders::Quad);
    quadShader.setMat4("transform", worldTransform);
    if (Debug->settings["drawChunkBorders"]) {
        Draw::chunkBorders(ren.guiQuadRenderer, camera, SDL_Color{255, 0, 255, 155}, 8.0f, 0.5f);
    }

    auto holyTree = Entities::findNamedEntity("Holy tree", &state->ecs);
 
    //ren.worldGuiRenderer.text->render("HI", {5, 5});
    float offset = 50.0f;
    /*
    for (int i = 0; i < 20; i++) {
        ren.fonts.get("Debug")->scale(i * 0.2f);
        auto renderResult = ren.guiRenderer.text->render("Testing !", {5, offset},
            TextFormattingSettings(TextAlignment::TopLeft),
            TextRenderingSettings({255, 0, 0, 255}, glm::vec2(0.5)));
        offset+=renderResult.rect.h;
    }
    */

    ren.guiRenderer.text->render("default font", {250, 250});
    auto settings = TextRenderingSettings({255, 0, 0, 255});
    settings.font = ren.fonts.get("World");
    ren.guiRenderer.text->render("different font", {500, 250}, settings);
    ren.guiRenderer.flush(ren.shaders, screenTransform);


    ren.worldGuiRenderer.flush(ren.shaders, worldTransform);
    ren.worldGuiRenderer.text->render("HI", {5, 5});
    ren.worldGuiRenderer.text->render("This is roboto", {-5, -5},
        TextRenderingSettings(Fonts->get("tester"), {255, 255, 0, 255}));
    
    //ren.worldTextRenderer.flush(ren.shaders.get(Shaders::Text), worldTransform);

    ren.shaders.use(Shaders::Quad).setMat4("transform", screenTransform);
    ren.guiQuadRenderer.flush(quadShader, worldTransform, ren.guiRenderer.guiAtlas.unit);
    
    GL::logErrors();

    // for text, depth mask off, dept

    // GL_BLEND is required for text, if text are boxes, that's probably why
    //glDisable(GL_BLEND);

    ren.guiRenderer.options = options;

    Draw::drawGui(ren, camera, screenTransform, gui, state, controls);
    GL::logErrors();

    //renderTexture(ren.shaders.get(Shaders::Texture), ren.font.atlasTexture);

    if (mode == Testing) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
    }

    /* Done rendering */
    SDL_GL_SwapWindow(ren.window);

    GL::logErrors();
}