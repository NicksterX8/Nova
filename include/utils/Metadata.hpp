#ifndef METADATA_INCLUDED
#define METADATA_INCLUDED

#include "My/Vec.hpp"
#include "utils/ints.hpp"

using Tick = Uint64;
constexpr Tick NullTick = 0;

struct UpdateMetadata {
    double deltaTime;
    Uint64 currentPerfCount;
    Uint64 lastPerfCount;
    Uint64 targetUpdatesPerSecond;
    Tick updateCount;
    Uint64 timestamp; // time in miliseconds

    static constexpr int storedUpdateTimes = 60;
    My::Vec<double> updateTimeHistory = My::Vec<double>::WithCapacity(storedUpdateTimes); // in miliseconds
    double ups = 0.0; // updates per second
    Tick oldestTrackedUpdate = 0;

    UpdateMetadata() {
        memset(this, 0, sizeof(*this));
    }

    UpdateMetadata(Uint64 targetUpdatesPerSecond);

    // track an update
    // return: new update count
    Tick update();
};


/*
* Useful metadata information and utilities
*/
struct MetadataTracker {
    Uint64 startTicks; // The start time of the game in ticks (miliseconds since sdl_init)

    UpdateMetadata frame;
    UpdateMetadata tick;

public:
    bool vsyncEnabled;

    MetadataTracker() {}

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

    Tick newFrame();
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

    // get the current frame number
    Tick getFrame() const {
        return frame.updateCount;
    }

    // Get the current tick number / the number of ticks (incremented each time Metadata::tick() is called) since the start of the application
    Tick getTick() const {
        return tick.updateCount;
    }

    // get the time since initialization in miliseconds
    Uint64 miliseconds() const;

    // get the time since initialization in seconds
    double seconds() const;

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