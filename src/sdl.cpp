#include "sdl.hpp"

namespace SDL {
    float pixelScale = 1.0f;
}

SDLContext initSDL() {
    bool vsync = true;
    const char* windowTitle = WINDOW_TITLE;
    auto windowIconPath = FileSystem.assets.get("bad-factorio-logo.png");

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

    int sdlSubsystemsNeeded[4] = {SDL_INIT_VIDEO, SDL_INIT_AUDIO, SDL_INIT_EVENTS, SDL_INIT_TIMER};
    bool sdlSubsystemsInitialized[4] = {0};
    for (int i = 0; i < sizeof(sdlSubsystemsNeeded)/sizeof(int); i++) {
        if (SDL_InitSubSystem(sdlSubsystemsNeeded[i])) {
            LogCritical("Failed to initialize SDL subsystem %d. %s", sdlSubsystemsNeeded[i], SDL_GetError());
        } else {
            sdlSubsystemsInitialized[i] = true;
        }
    }

    if (!sdlSubsystemsInitialized[0] || !sdlSubsystemsInitialized[2]) {
        LogCritical("Failed to initialize one or more required SDL subsystems. %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    LogInfo("SDL successfully initialized.");

    int imgSubsystemsNeeded = IMG_INIT_PNG | IMG_INIT_JPG;
    int imgSubsystemsInitialized = IMG_Init(imgSubsystemsNeeded);
    if (imgSubsystemsInitialized != imgSubsystemsNeeded) {
        LogCritical("Failed to initialize one or more required SDL_Image subsystems. Missing subsystems flags: %d. SDL_Image error message: %s",
            imgSubsystemsNeeded ^ imgSubsystemsInitialized, SDL_GetError());
        exit(EXIT_FAILURE);
    }

    LogInfo("SDL_Image successfully initialized.");

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
        LogError("Failed creating window. Error: %s", sdlError);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GAME_TITLE " - Crashed", sdlError, NULL);
        exit(EXIT_FAILURE);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GLContext glContext = SDL_GL_CreateContext(context.win);
    if (!glContext) {
        LogError("Failed creating OpenGL context! %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    /* set window icon */
    if (windowIconPath) {
        SDL_Surface* iconImage = IMG_Load(windowIconPath);
        if (!iconImage) {
            LogError("Failed creating window icon. Window icon path: %s. Error: %s", (char*)windowIconPath, SDL_GetError());
        } else {
            SDL_SetWindowIcon(context.win, iconImage);
            // once set the image can be freed
            SDL_FreeSurface(iconImage);
        }
    }
    
    context.scale = SDL::getPixelScale(context.win).x;
    SDL::pixelScale = context.scale;

    SDL_Log("SDL Window Context initialized.");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    if (vsync)
        SDL_GL_SetSwapInterval(1); // wait till display update
    else
        SDL_GL_SetSwapInterval(0); // swap frames immediately

    return context;
}

void quitSDL(SDLContext* ctx) {
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

    LogInfo("SDL quit!");
}