#include "constants.hpp"
#include "utils/Log.hpp"

#include <stdio.h>
#include <libproc.h>
#include <unistd.h>

#include "sdl_gl.hpp"

#include "GUI/Gui.hpp"
#include "utils/Debug.hpp"
#include "loadData.hpp"

#include "Game.hpp"
#include "utils/Debug.hpp"
#include "GameSave/main.hpp"
#include "sdl.hpp"
#include "global.hpp"

#include "memory.hpp"

#ifdef DEBUG
    //#include "Testing.hpp"
#endif

void logEntitySystemInfo() {
    LogInfo("Number of systems: %d", NUM_SYSTEMS);
    const char* systemsStr = TOSTRING((SYSTEMS));
    int length = strlen(systemsStr);
    char systemsStr2[length];
    strcpy(systemsStr2, systemsStr+1);
    systemsStr2[length-2] = '\0';

    char (systemsList[NUM_SYSTEMS])[256];
    int charIndex = 0;
    for (int i = 0; i < NUM_SYSTEMS; i++) {
        const char* start = &systemsStr2[charIndex];
        const char* chr = start;
        int size = 0;
        while (true) {
            if (*chr == ',') {
                break;
            }
            if (*chr == '\0') {
                // Log("failed to find comma");
                break;
            }
            chr++;
            size++;
        }
        strncpy(systemsList[i], start, size+1);
        systemsList[i][size] = '\0';
        charIndex += (size + 2);
    }

    for (int i = 0; i < NUM_SYSTEMS; i++) {
        LogInfo("ID: %d, System: %s.", i, systemsList[i]);
    }
}

#define GET_MACRO_STRING_LIST(macro) getMacroStringList(TOSTRING((macro)), sizeof(TOSTRING((macro))) - 2)

inline My::StringBuffer getMacroStringList(const char* macro, int length) {
    const char* const start = macro+1;
    const char* const end = macro + length;

    const char* c = start;

    auto outList = My::StringBuffer::WithCapacity(length+32);
    for (int i = 0; i < ECS_NUM_COMPONENTS; i++) {
        const char* nameStart = c;
        const char* nameEnd = c;
        while (true) {
            if (*nameEnd == ' ' || *nameEnd == '(') {
                if (nameEnd == nameStart) {
                    nameStart++;
                } else {
                    break;
                }
            }
            else if (nameEnd == end || *nameEnd == ',') {
                nameEnd++;
                break;
            }
            nameEnd++;
        }
        outList.pushUnterminatedStr(nameStart, nameEnd - nameStart - 1);
        c = nameEnd;
    }
    return outList;
}

void logEntityComponentInfo() {
    /*
    const char* componentsStr = TOSTRING((COMPONENT_NAMES));
    int length = strlen(componentsStr);
    char componentsStr2[length];
    strcpy(componentsStr2, componentsStr+1);
    componentsStr2[length-2] = '\0';

    char* buffer = (char*)malloc(sizeof(char) * 256 * NUM_COMPONENTS);

    char* componentsList;
    componentsList = buffer;
    int charIndex = 0;
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        const char* start = &componentsStr2[charIndex];
        const char* chr = start;
        int size = 0;
        while (true) {
            if (*chr == ',') {
                break;
            }
            if (*chr == '\0') {
                // Log("failed to find comma");
                break;
            }
            chr++;
            size++;
        }
        strlcpy(&componentsList[i * 256], start, size+1);
        componentsList[i * 256 + size] = '\0';
        charIndex += (size + 2);
    }

    for (int i = 0; i < NUM_COMPONENTS; i++) {
        //Log("ID: %d, Component: %s.", i, &componentsList[i * 256]);
        componentNames[i] = &componentsList[i * 256];
    }
    */
    auto nameBuffer = GET_MACRO_STRING_LIST(COMPONENTS);
    int i = 0;
    FOR_MY_STRING_BUFFER(componentName, nameBuffer, {
        componentNames[i] = componentName;
        LogInfo("Component Name: %s", componentName);
        i++;
    });
}

void initLogging() {
    SDL_LogSetOutputFunction(Logger::logOutputFunction, &gLogger);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_ERROR);
    SDL_LogSetPriority(LogCategory::Main, SDL_LOG_PRIORITY_INFO);
}

#define LOG LogInfo

void initPaths() {
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

    char* basePath = SDL_GetBasePath();

    pid_t pid = getpid();
    int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
    if (ret <= 0) {
        fprintf(stderr, "PID %d: proc_pidpath ();\n", pid);
        fprintf(stderr, "    %s\n", strerror(errno));
        printf("Error: Failed to get pid path!\n");
    } else {

        char upperPath[512]; strcpy(upperPath, pathbuf);
        upperPath[std::string(pathbuf).find_last_of('/')] = '\0';

        auto filePrefix = str_add(upperPath, "/resources/");

        FileSystem = FileSystemT(filePrefix);
        LOG("resources path: %s", FileSystem.resources.get());
    }

    LOG("Path to executable: %s\n", pathbuf);
    LOG("Base path: %s\n", basePath);
}

void tests() {
    Item item = Item();
    item.type = ItemTypes::Grenade;
    ItemStack stack(item);
    stack.quantity = 5;
    assert(stack.reduceQuantity(3) == 3);
    assert(stack.quantity == 2 && stack.item.type == ItemTypes::Grenade);
    assert(stack.reduceQuantity(items::ItemQuantityInfinity) == 2);
    assert(stack.quantity == 0 && stack.item.type == ItemTypes::None);
}

int main(int argc, char** argv) { 
    tests();

    gLogger.useEscapeCodes = true;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color-codes") == 0) {
            gLogger.useEscapeCodes = false;
        }
    }

    initLogging();
    initPaths();
    gLogger.init(FileSystem.save.get("log.txt"));

    LogInfo("argv[0]: %s\n", argv[0]);

    SDLContext sdlCtx = initSDL();

    logEntityComponentInfo();

    int screenWidth,screenHeight;
    SDL_GL_GetDrawableSize(sdlCtx.win, &screenWidth, &screenHeight);

    //load(sdlCtx.ren, sdlCtx.scale);

    Game* game = new Game(sdlCtx);
    Mem::init(
        [&]() { // need memory
            auto state = game->state;
            state->ecs.minmizeMemoryUsage();
        },
        [&]() { // failed to get enough memory, crash.
            logCrash(CrashReason::MemoryFail, "Out of memory!");
        }
    );

    game->init(screenWidth, screenHeight);
    game->start(); // start indefinitely long game loop
    // runs after the game loop has ended
    game->quit();
    game->destroy();

    gLogger.destroy();
    FileSystem.destroy();

    quitSDL(&sdlCtx);

    return 0;
}