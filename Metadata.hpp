#ifndef METADATA_INCLUDED
#define METADATA_INCLUDED

#include "constants.hpp"
#include <SDL2/SDL.h>

/*
* Useful metadata information and utilities
*/
class MetadataInfo {
    double _fps; // actual fps
    double targetFps; // Desired fps
    double deltaTime; // the change in miliseconds from the last tick to the current tick
    double perfCountCurrentTick; // performance count of the tick that has not yet finished
    double perfCountLastTick; // performance count of the last finished tick
    int _ticks; // the number of ticks that have passed
    Uint32 startTicks; // The start time of the game in ticks
public:
    bool vsyncEnabled;

    MetadataInfo(double targetFps, bool vsync);

    /*
    * Mark the start time of the game, for better tracking of various data (like start time)
    */ 
    Uint32 start();

    /* 
    * Tick to signify one update passing, allowing update time tracking
    * @return The deltaTime of latest tick
    */
    double tick();

    /*
    * Get the current estimated frames (more accurately, ticks) per second,
    * based on the deltatime between ticks
    */
    double fps();

    void setTargetFps(double newTargetFps);
    double getTargetFps();

    // Get the number of ticks that have passed
    int ticks();

    // Get the start time (when start() was called) of the game in ticks.
    Uint32 getStartTicks();

};

extern MetadataInfo Metadata;

#endif