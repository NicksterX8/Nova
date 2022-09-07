#ifndef SDL_HPP
#define SDL_HPP

#include <SDL2/SDL.h>
#include "NC/SDLContext.h"
#include "constants.hpp"
#include "global.h"
#include "utils/MyString.h"
#include "sdl_gl.hpp"
#include "utils/Log.hpp"

#define WINDOW_HIGH_DPI 1

typedef struct SDLContext {
    SDL_Window *win;
    SDL_GLContext gl;
    float scale;
} SDLContext;

inline SDLContext initSDL() {
    bool vsync = true;
    const char* windowTitle = WINDOW_TITLE;
    auto windowIconPath = str_add(FilePaths::assets, "bad-factorio-logo.png");

    SDL_Rect windowRect = {
        0, 0,
        800, 800
    };

    bool allowHighDPI = false;
#ifdef WINDOW_HIGH_DPI
    if (WINDOW_HIGH_DPI) {
        allowHighDPI = true;
    }
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER)) {
        Log.Error("Failed initializing SDL. MSG: %s", SDL_GetError());
    } else {
        Log.Info("SDL Initialized.");
    }

    SDLContext context;

    int winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    winFlags |= (SDL_WINDOW_ALLOW_HIGHDPI * allowHighDPI);
    context.win = SDL_CreateWindow(
        windowTitle,
        windowRect.x, windowRect.y,
        windowRect.w, windowRect.h,
        winFlags
    );

    if (!context.win) {
        const char* sdlError = SDL_GetError();
        Log.Error("Failed creating window. Error: %s", sdlError);
        char errorMessage[512];
        if (sdlError) 
            snprintf(errorMessage, 512, "Failed to open a window. SDL Error message: %s", sdlError);
        else
            strcpy(errorMessage, "Failed to open a window, no error given from SDL.");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GAME_TITLE " - Crashed", errorMessage, NULL);
        exit(EXIT_FAILURE);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GLContext glContext = SDL_GL_CreateContext(context.win);
    if (!glContext) {
        Log.Error("Failed creating OpenGL context! Error message: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    /* set window icon */
    SDL_Surface* iconSurface = NULL;
    if (windowIconPath) {
        iconSurface = IMG_Load(windowIconPath);
    } else {
        SDL_Log("No window icon path set when initializing SDL. Defaulting to SDL logo.");
        iconSurface = SDL_CreateRGBSurfaceFrom(
            SDL_LOGO_PIXELDATA,
            16, 16, 16,
            16*2,
            0x0f00,0x00f0,0x000f,0xf000 
        );
    }
    if (!iconSurface) {
        SDL_Log("Error creating window icon. Error: %s, Window icon path: %s\n", SDL_GetError(), (char*)windowIconPath);
    } else {
        SDL_SetWindowIcon(context.win, iconSurface);
        SDL_FreeSurface(iconSurface);
    }
    
    context.scale = 1.0f;
    SDLPixelScale = 2.0f;

    SDL_Log("SDL Window Context initialized.");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    if (vsync)
        SDL_GL_SetSwapInterval(1);
    else
        SDL_GL_SetSwapInterval(0); // swap frames immediately

    return context;
}

inline void quitSDL(SDLContext* ctx) {
    if (!ctx) return;

    if (ctx->gl) {
        // for some reason this seg faults
        //SDL_GL_DeleteContext(ctx->gl);
        ctx->gl = NULL;
    }
    if (ctx->win) {
        SDL_DestroyWindow(ctx->win);
        ctx->win = NULL;
    }

    Log.Info("SDL quit!");
}

#endif