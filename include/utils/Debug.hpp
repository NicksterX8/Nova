#ifndef DEBUG_INCLUDED
#define DEBUG_INCLUDED

#include <string.h>
#include "../constants.hpp"

namespace GUI {
    struct Console;
}

struct DebugSettings {
    bool drawChunkBorders;
    bool drawChunkCoordinates;
    bool drawChunkEntityCount;

    bool drawPlayerRect;
    bool drawEntityIDs;
    bool drawEntityRects;
};

struct DebugClass {
public:
    bool debugging = false;

    DebugSettings settings;

    GUI::Console* console = nullptr;

    DebugClass();

    // Reset all debug settings to default
    void resetSettings();
};

extern const DebugClass* Debug;

#endif