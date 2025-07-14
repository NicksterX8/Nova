#ifndef DEBUG_INCLUDED
#define DEBUG_INCLUDED

#include <string>
#include <unordered_map>
#include "../constants.hpp"

#if DEBUG_LEVEL >= 1
#define BOUNDS_CHECKS 1
#endif

namespace GUI {
    struct Console;
}

struct DebugSettings {
    std::unordered_map<std::string, bool> bools;

    void init() {
        bools.insert({"drawChunkBorders", false});
        bools.insert({"drawEntityCollisionBoxes", false});
        bools.insert({"drawEntityViewBoxes", false});
        bools.insert({"drawEntityIDs", false});
    }

    bool* get(std::string str) {
        auto it = bools.find(str);
        if (it != bools.end()) {
            return &it->second;
        }
        return nullptr;
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
    bool debugging = true;

    DebugSettings settings;

    GUI::Console* console = nullptr;

    DebugClass();

    // Reset all debug settings to default
    void resetSettings();
};

extern const DebugClass* Debug;

#endif