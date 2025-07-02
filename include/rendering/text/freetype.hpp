#ifndef FREETYPE_INCLUDED
#define FREETYPE_INCLUDED

#include <ft2build.h>
#include <freetype/ftmodapi.h>
#include FT_FREETYPE_H

#include "utils/Log.hpp"

extern FT_Library freetype;

inline int initFreetype() {
    if (FT_Init_FreeType(&freetype)) {
        LogError("Failed to initialize FreeType library.");
        return -1;
    }

    #define FREETYPE_SDF_SPREAD 2
    FT_Int spread = FREETYPE_SDF_SPREAD;
    if (auto error = FT_Property_Set(freetype, "sdf", "spread", &spread)) {
        auto str = FT_Error_String(error);
        LogError("Failed to set freetype sdf spread property. Error: %s", str);
    }

    return 0;
}

inline void quitFreetype() {
    // seg faults for some reason
    FT_Done_FreeType(freetype);
}

#endif