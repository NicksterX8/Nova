#ifndef RENDERING_INCLUDED
#define RENDERING_INCLUDED

#include <vector>

#include <SDL2/SDL.h>
#include "../sdl_gl.hpp"
#include "../utils/random.hpp"
#include "../GameState.hpp"
#include "../Tiles.hpp"
#include "../utils/Metadata.hpp"
#include "../Textures.hpp"
#include "../utils/Debug.hpp"
#include "../GUI/GUI.hpp"
#include "../SDL2_gfx/SDL2_gfxPrimitives.h"
#include "../utils/Log.hpp"

#include "Drawing.hpp"
#include "../EntitySystems/Rendering.hpp"
#include "Shader.hpp"
#include "../Camera.hpp"
#include "utils.hpp"
#include "context.hpp"
#include "../My/Array.hpp"
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
void render(RenderContext& ren, float scale, GUI* gui, GameState* state, Camera& camera, Vec2 playerTargetPos);


#endif