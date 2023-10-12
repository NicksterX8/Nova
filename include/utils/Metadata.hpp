#ifndef METADATA_INCLUDED
#define METADATA_INCLUDED

#include "../constants.hpp"
#include <SDL2/SDL.h>

using Tick = int64_t;
constexpr Tick NullTick = 0;
constexpr Tick InfinityTicks = INT64_MAX;

using FrameCount = Uint32;

/*
* Useful metadata information and utilities
*/
struct MetadataTracker {
    double _fps; // actual fps
    double targetFps; // Desired fps, currently unused
    double deltaTime; // the change in milliseconds from the last tick to the current tick
    double perfCountCurrentTick; // performance count of the tick that has not yet finished
    double perfCountLastTick; // performance count of the last finished tick
    FrameCount _frames; // the number of frames that have passed
    Uint64 startTicks; // The start time of the game in ticks
public:
    bool vsyncEnabled;

    MetadataTracker(double targetFps, bool vsync);

    /*
    * Mark the start time of the application, for better tracking of various data (like start time)
    * @return The current time in ticks.
    */ 
    Uint64 start();

    /*
    * Mark the end of the application's main loop.
    * @return The time elapsed in seconds.
    */
    double end();

    
    FrameCount newFrame();

    /*
    * Get the current estimated frames (more accurately, ticks) per second,
    * based on the deltatime between ticks
    */
    double fps() const;

    void setTargetFps(double newTargetFps);
    double getTargetFps() const;

    // Get the current tick / the number of ticks (incremented each time Metadata::tick() is called) since the start of the application
    FrameCount currentFrame() const;

    double seconds() const {
        return (GetTicks() - startTicks) / 1000.0f;
    }

    // Get the start time (when start() was called) of the game in ticks.
    Uint64 getStartTicks() const;
};

extern const MetadataTracker* Metadata;

extern Tick gTick;

inline Tick newTick() {
    return ++gTick;
}

inline Tick getTick() {
    return gTick;
}

inline Tick secondsToTicks(int seconds) {
    return seconds * 60;
}

enum TickDeltaEnum {
    TickBefore,
    TickAfter,
    TickEqual,
    TickInvalid
};

inline TickDeltaEnum tickDelta(Tick tick, Tick tick2) {
    if (tick == NullTick || tick2 == NullTick) return TickInvalid;

    Tick d = tick2 - tick;
    if (d > 0) return TickAfter;
    else if (d < 0) return TickBefore;
    else return TickEqual;
}

inline Tick tickDelay(Tick delay) {
    return getTick() + delay;
}

#endif