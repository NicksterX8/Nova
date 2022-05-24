#ifndef DEBUG_INCLUDED
#define DEBUG_INCLUDED

#include <string.h>

struct DebugSettings {
    bool drawChunkBorders;
    bool drawChunkCoordinates;
    bool drawPlayerRect;
    bool drawEntityIDs;
    bool drawEntityRects;
};

class DebugClass {
public:
    DebugSettings settings;

    DebugClass();

    // Reset all debug settings to default
    void resetSettings();
};

extern DebugClass Debug;

#endif