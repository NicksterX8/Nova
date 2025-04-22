#ifndef DEBUG_INCLUDED
#define DEBUG_INCLUDED

#include <string>
#include <unordered_map>
#include "../constants.hpp"

namespace GUI {
    struct Console;
}

struct DebugSettings {
    /*
    bool drawChunkBorders;
    bool drawChunkCoordinates;
    bool drawChunkEntityCount;

    bool drawPlayerRect;
    bool drawEntityIDs;
    bool drawEntityViewBoxes;
    bool drawEntityCollisionBoxes;
    */

    std::unordered_map<std::string, bool> bools;

    void init() {

    }

    bool& operator[](const char* str) {
        return bools[std::string(str)];
    }

    bool operator[](const char* str) const {
        auto it = bools.find(std::string(str));
        if (it != bools.end()) return it->second;
        else return false;
    }

    bool& operator[](std::string str) {
        return bools[str];
    }

    bool operator[](std::string str) const {
        auto it = bools.find(str);
        if (it != bools.end()) return it->second;
        else return false;
    }
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