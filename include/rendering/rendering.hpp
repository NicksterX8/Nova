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

RenderBuffer makeRenderBuffer(glm::ivec2 size);
void resizeRenderBuffer(RenderBuffer renderBuffer, glm::ivec2 newSize);
void destroyRenderBuffer(RenderBuffer renderBuffer);

void renderInit(RenderContext& ren, int screenWidth, int screenHeight);
void renderQuit(RenderContext& ren);
void render(RenderContext& ren, RenderOptions options, Gui* gui, GameState* state, Camera& camera, const PlayerControls& playerControls, Mode mode, bool renderWorld);

int setupShaders(RenderContext* ren);

#endif