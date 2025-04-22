#ifndef METADATA_INCLUDED
#define METADATA_INCLUDED

#include "../constants.hpp"
#include <SDL2/SDL.h>
#include "My/Vec.hpp"

using Tick = int64_t;
constexpr Tick NullTick = 0;
constexpr Tick InfinityTicks = INT64_MAX;

struct UpdateMetadata {
    double deltaTime;
    Uint64 currentPerfCount;
    Uint64 lastPerfCount;
    Uint32 targetUpdatesPerSecond;
    Uint32 updateCount;
    Uint64 timestamp; // time in miliseconds

    static constexpr int storedUpdateTimes = 60;
    My::Vec<double> updateTimeHistory = My::Vec<double>::WithCapacity(storedUpdateTimes); // in miliseconds
    double ups = 0.0; // updates per second
    int oldestTrackedUpdate = 0;

    Uint32 update() {
        lastPerfCount = currentPerfCount;
        currentPerfCount = GetPerformanceCounter();
        if (updateCount == 0) {
            deltaTime = 0.0;
        } else {
            deltaTime = (double)((currentPerfCount - lastPerfCount) * 
                1000.0/(double)GetPerformanceFrequency());
            if (updateTimeHistory.size < updateTimeHistory.capacity) {
                updateTimeHistory.push(deltaTime);
            } else {
                updateTimeHistory[oldestTrackedUpdate] = deltaTime;
                oldestTrackedUpdate = (oldestTrackedUpdate + 1) % updateTimeHistory.size;
            }
        }

        /* Calculate updates per second */
        double totalTime = 0.0;
        for (auto dt : updateTimeHistory) {
            totalTime += dt;
        }
        double averageDeltaTime = totalTime / updateTimeHistory.size;
        ups = 1000.0 / averageDeltaTime;

        timestamp = GetTicks();

        return ++updateCount;
    }

    UpdateMetadata(Uint32 targetUpdatesPerSecond)
    : deltaTime(NAN), currentPerfCount(GetPerformanceCounter()), lastPerfCount(GetPerformanceCounter()),
    targetUpdatesPerSecond(targetUpdatesPerSecond), updateCount(0) {}
};



/*
* Useful metadata information and utilities
*/
struct MetadataTracker {
    /*
    double _fps; // actual fps
    double targetFps; // Desired fps, currently unused
    double frameDeltaTime; // the change in milliseconds from the last frame to the current frame
    double tickDeltaTime; // the change in milliseconds from the last tick to the current tick
    Uint64 currentFramePerfCount; // performance count of the current frame
    Uint64 lastFramePerfCount; // performance count of the last finished frame
    */
    
    Uint64 startTicks; // The start time of the game in ticks (miliseconds since sdl_init)
    //Uint32 targetTps;

    UpdateMetadata frame;
    UpdateMetadata tick;

public:
    bool vsyncEnabled;

    MetadataTracker(Uint32 targetFps, Uint32 targetTps, bool vsync);

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

    Uint32 newFrame();
    Tick newTick();

    /*
    * Get the current estimated current number of frame updates per second
    * based on the deltatime between frames
    */
    double fps() const;

    /*
    * Get the current estimated current number of ticks per second
    * based on the deltatime between ticks
    */
    double tps() const;

    // Get the current tick / the number of ticks (incremented each time Metadata::tick() is called) since the start of the application
    Uint32 getFrame() const {
        return frame.updateCount;
    }

    Tick getTick() const {
        return tick.updateCount;
    }

    // get the time since initialization in miliseconds
    Uint64 miliseconds() const {
        return GetTicks() - startTicks;
    }

    // get the time since initialization in seconds
    double seconds() const {
        return (GetTicks() - startTicks) / 1000.0f;
    }

    // Get the start time (when start() was called) of the game in ticks.
    Uint64 getStartTicks() const;
};

extern const MetadataTracker* Metadata;

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
    return Metadata->getTick() + delay;
}

#endif