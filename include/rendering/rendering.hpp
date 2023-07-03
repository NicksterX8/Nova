#ifndef RENDERING_INCLUDED
#define RENDERING_INCLUDED

#include <SDL2/SDL.h>
#include "sdl_gl.hpp"
#include "GameState.hpp"
#include "utils/Metadata.hpp"
#include "textures.hpp"
#include "GUI/Gui.hpp"

#include "drawing.hpp"
#include "Shader.hpp"
#include "../Camera.hpp"
#include "utils.hpp"
#include "context.hpp"
#include "PlayerControls.hpp"

#include <glm/vec2.hpp>


struct TilemapVertex {
    glm::vec2 pos;
    glm::vec3 texCoord;
};


void renderInit(RenderContext& ren);
void renderQuit(RenderContext& ren);
void render(RenderContext& ren, RenderOptions options, Gui* gui, GameState* state, Camera& camera, Vec2 playerTargetPos, Mode mode);

#endif