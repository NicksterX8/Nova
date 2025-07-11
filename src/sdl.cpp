#include "sdl.hpp"
#include "global.hpp"

namespace SDL {
    float pixelScale = 1.0f;
}

SDL_Window* createWindow(const char* windowTitle, SDL_Rect windowRect, const char* windowIconPath = nullptr, SDL_DisplayID display = 0) {
    if (display) {
        SDL_Rect displayBounds;
        SDL_GetDisplayBounds(display, &displayBounds);

        windowRect.x += displayBounds.x;
        windowRect.y += displayBounds.y;
    }
    // if no display provided just let sdl handle it by default

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, windowTitle);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, windowRect.x);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, windowRect.y);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, windowRect.w);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, windowRect.h);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, WINDOW_HIGH_DPI);

    SDL_Window* window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    SDL_SetWindowMinimumSize(window, 400, 400);

    /* set window icon */
    if (windowIconPath) {
        SDL_Surface* iconImage = IMG_Load(windowIconPath);
        if (!iconImage) {
            LogError("Failed creating window icon. Window icon path: %s. Error: %s", (char*)windowIconPath, SDL_GetError());
        } else {
            SDL_SetWindowIcon(window, iconImage);
            // once set the image can be freed
            SDL_DestroySurface(iconImage);
        }
    }

    return window;
}

SDLContext initSDL(const char* windowTitle, SDL_Rect windowRect) {
    bool vsync = ENABLE_VSYNC;
    bool useAdaptiveVsync = false; // if using vsync - not using rn because i'm not sure if its good on mac or not
    auto windowIconPath = FileSystem.assets.get("bad-factorio-logo.png");

    int sdlSubsystemsNeeded[5] = {
        SDL_INIT_VIDEO,
        SDL_INIT_AUDIO,
        SDL_INIT_EVENTS,
        SDL_INIT_GAMEPAD
    };
    Uint32 subsystemsFailed = 0;
    Uint32 essentialSystems = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    for (int i = 0; i < sizeof(sdlSubsystemsNeeded)/sizeof(int); i++) {
        if (SDL_InitSubSystem(sdlSubsystemsNeeded[i])) {

        } else {
            LogCritical("Failed to initialize SDL subsystem %d. %s", sdlSubsystemsNeeded[i], SDL_GetError());
            subsystemsFailed |= sdlSubsystemsNeeded[i];
        }
    }

    if (subsystemsFailed & essentialSystems) {
        LogCrash(CrashReason::SDL_Initialization, "Failed to initialize one or more essential SDL subsystems. %s", SDL_GetError());
    }

    LogInfo("SDL successfully initialized.");

    SDLContext context;

    SDL_DisplayID primaryWindowDisplay = 0;
    SDL_DisplayID secondaryWindowDisplay = 0;

    int displayCount = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&displayCount);
    
    if (displayCount > 1) {
        primaryWindowDisplay = displays[1];

        if (USE_SECONDARY_WINDOW) {
            secondaryWindowDisplay = displays[1];

            SDL_Rect windowBounds;
            SDL_GetDisplayBounds(secondaryWindowDisplay, &windowBounds);

            SDL_Rect secondaryWindowRect = {
                windowRect.w,
                0,
                windowBounds.w - windowRect.w,
                windowBounds.h
            };
            context.secondary.window = createWindow("Secondary window", secondaryWindowRect, windowIconPath, secondaryWindowDisplay);
            if (!context.secondary.window) {
                LogError("Failed to create secondary window. SDL error: %s", SDL_GetError());
            }
        }
    }
    context.primary.window = createWindow(windowTitle, windowRect, windowIconPath, primaryWindowDisplay);

    SDL_free(displays);

    if (!context.primary.window) {
        const char* sdlError = SDL_GetError();
        LogError("Failed creating primary window. Error: %s", sdlError);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GAME_TITLE " - Crashed", sdlError, NULL);
        exit(EXIT_FAILURE);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    context.primary.glContext = SDL_GL_CreateContext(context.primary.window);
    if (!context.primary.glContext) {
        LogError("Failed creating OpenGL context! %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    if (context.secondary.window) {
        context.secondary.glContext = SDL_GL_CreateContext(context.secondary.window);
        if (!context.secondary.glContext) {
            LogError("Failed to create OpenGL context for secondary window!");
        }
        SDL_GL_MakeCurrent(context.primary.window, context.primary.glContext);
    }
    
    if (vsync) {
        if (useAdaptiveVsync && SDL_GL_SetSwapInterval(-1)) {
            LogInfo("Using adaptive vsync");
        } else {
            LogInfo("Using static vsync");
            SDL_GL_SetSwapInterval(1); // wait till display update
        }
    } else {
        SDL_GL_SetSwapInterval(0); // swap frames immediately
    }

    SDL::pixelScale = SDL::getPixelScale(context.primary.window);

    SDL_Log("SDL Window Context initialized.");

    // if (vsync)
    //     SDL_GL_SetSwapInterval(1); 
    // else
    //     SDL_GL_SetSwapInterval(0); 

    return context;
}

void quitSDL(SDLContext* ctx) {
    if (!ctx) return;

    if (ctx->secondary.glContext) {
        // for some reason this seg faults
        //SDL_GL_DeleteContext(ctx->gl);
        ctx->secondary.glContext = NULL;
    }
    if (ctx->secondary.window) {
        SDL_DestroyWindow(ctx->secondary.window);
        ctx->secondary.window = NULL;
    }

    if (ctx->primary.glContext) {
        // for some reason this seg faults
        //SDL_GL_DeleteContext(ctx->gl);
        ctx->primary.glContext = NULL;
    }
    if (ctx->primary.window) {
        SDL_DestroyWindow(ctx->primary.window);
        ctx->primary.window = NULL;
    }

    LogInfo("SDL quit!");
}