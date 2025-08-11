#ifndef GLOBAL_INCLUDED
#define GLOBAL_INCLUDED

#include "constants.hpp"
#include "utils/Log.hpp"
#include "My/String.hpp"
#include "utils/vectors_and_rects.hpp"
#include "threads.hpp"
#include "memory/GlobalAllocators.hpp"

struct GlobalsType {
    unsigned int debugTexture = 0;
    glm::vec2 playerMovement = {NAN, NAN};
    bool mouseClickedOnGui = false;

    bool paused = false;

    bool multithreadingEnabled = USE_MULTITHREADING;
    ThreadManager threadManager;

    GlobalsType() {
        
    }
};

extern GlobalsType Global;

enum Mode : int {
    NullMode,
    Unstarted,
    Paused,
    Playing,
    Quit,
    Testing,
    NumModes
};
static constexpr Mode FirstMode = Unstarted;

inline const char* modeName(Mode mode) {
    switch (mode) {
    case Unstarted:
        return "unstarted";
    case Paused:
        return "paused";
    case Playing:
        return "playing";
    case Quit:
        return "quit";
    case Testing:
        return "testing";
    default:
        break;
    }
    return "null";
}

inline Mode modeID(const char* mode) {
    for (int i = NullMode; i < NumModes; i++) {
        if (My::streq(modeName((Mode)i), mode)) {
            return (Mode)i;
        }
    }
    return NullMode;
}


#define UNFINISHED_CRASH() assert(0 && "Unfinished code, should not be run!")

#endif