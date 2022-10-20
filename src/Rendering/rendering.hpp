#ifndef RENDERING_INCLUDED
#define RENDERING_INCLUDED

#include <SDL2/SDL.h>
#include "../sdl_gl.hpp"
#include "../GameState.hpp"
#include "../utils/Metadata.hpp"
#include "textures.hpp"
#include "../GUI/Gui.hpp"

#include "Drawing.hpp"
#include "Shader.hpp"
#include "../Camera.hpp"
#include "utils.hpp"
#include "context.hpp"
#include "../PlayerControls.hpp"

#include <glm/vec2.hpp>

struct TilemapVertex {
    glm::vec2 pos;
    glm::vec3 texCoord;
};

namespace Render {
    constexpr int numChunkVerts = CHUNKSIZE * CHUNKSIZE * 4;
    constexpr int numChunkFloats = numChunkVerts * sizeof(TilemapVertex) / sizeof(float);
    constexpr int numChunkIndices = 6 * CHUNKSIZE * CHUNKSIZE;
}

void renderInit(RenderContext& ren);
void renderQuit(RenderContext& ren);
void render(RenderContext& ren, float scale, Gui* gui, GameState* state, Camera& camera, Vec2 playerTargetPos);


#endif