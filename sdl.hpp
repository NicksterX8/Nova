#ifndef SDL_HPP
#define SDL_HPP

#include <SDL2/SDL.h>

#define WINDOW_HIGH_DPI 1

typedef struct SDLContext {
    SDL_Window *win;
    SDL_Renderer* ren;
    SDL_GLContext gl;
    float scale;
} SDLContext;

inline SDLContext initSDL() {
    SDLSettings settings = DefaultSDLSettings;
    settings.vsync = ENABLE_VSYNC;
    settings.windowTitle = "Faketorio";
    settings.windowIconPath = "assets/bad-factorio-logo.png";
    settings.windowWidth = 1000;
    settings.windowHeight = 800;
    if (WINDOW_HIGH_DPI) {
        settings.allowHighDPI = true;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER)) {
        Log.Error("Failed initializing SDL. MSG: %s", SDL_GetError());
    } else {
        Log.Info("SDL Initialized.");
    }

    SDLContext context;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    const char* windowTitle = settings.windowTitle;
    if (!settings.windowTitle) {
        Log.Error("Window title setting is null.");
        windowTitle = "NULL";
    }

    int winFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    winFlags |= (SDL_WINDOW_ALLOW_HIGHDPI * settings.allowHighDPI);
    context.win = SDL_CreateWindow(
        windowTitle,
        settings.windowX, settings.windowY,
        settings.windowWidth, settings.windowHeight,
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

    int renFlags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
    renFlags |= (SDL_RENDERER_PRESENTVSYNC * settings.vsync);
    context.ren = SDL_CreateRenderer(
        context.win,
        -1,
        renFlags
    );
    if (!context.ren) {
        Log.Critical("Error creating renderer. Error: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    if (SDL_RenderTargetSupported(context.ren) == SDL_FALSE) {
        Log.Critical("Renderer does not support render targets!");
    }
    SDL_Log("SDL Renderer created.");
    

    SDL_RendererInfo driverInfo;
    SDL_GetRenderDriverInfo(0, &driverInfo);
    Log.Info("SDL Render driver: %s", driverInfo.name);

    SDL_GLContext glContext = SDL_GL_CreateContext(context.win);
    if (!glContext) {
        Log.Error("Failed creating OpenGL context! Error message: %s", SDL_GetError());
        //exit(EXIT_FAILURE);
    }

    /* set window icon */
    SDL_Surface* iconSurface = NULL;
    if (settings.windowIconPath) {
        iconSurface = IMG_Load(settings.windowIconPath);
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
        SDL_Log("Error creating window icon. Error: %s, Window icon path: %s\n", SDL_GetError(), settings.windowIconPath);
    } else {
        SDL_SetWindowIcon(context.win, iconSurface);
        SDL_FreeSurface(iconSurface);
    }
    
    context.scale = getRendererWindowScale(context.win, context.ren);
    SDLPixelScale = context.scale;

    SDL_Log("SDL Window Context initialized.");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    return context;
}

inline void quitSDL(SDLContext* ctx) {
    if (!ctx) return;

    if (ctx->ren) {
        SDL_DestroyRenderer(ctx->ren);
        ctx->ren = NULL;
    }
    if (ctx->win) {
        SDL_DestroyWindow(ctx->win);
        ctx->win = NULL;
    }
    if (ctx->gl) {
        SDL_GL_DeleteContext(ctx->gl);
        ctx->gl = NULL;
    }

    Log.Info("SDL quit!");
}

#endif