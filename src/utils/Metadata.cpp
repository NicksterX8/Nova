#include "utils/Metadata.hpp"
#include "constants.hpp"

MetadataTracker::MetadataTracker(Uint32 targetFps, Uint32 targetTps, bool vsync)
: frame(targetFps), tick(targetTps) {
    startTicks = 0; // just in case start() is never called.
    vsyncEnabled = vsync;
}

Uint64 MetadataTracker::start() {
    startTicks = GetTicks();
    return startTicks;
}

double MetadataTracker::end() {
    return seconds();
}

Uint32 MetadataTracker::newFrame() {
    return frame.update();
}

Tick MetadataTracker::newTick() {
    return tick.update();
}

double MetadataTracker::fps() const {
    return frame.ups;
}

double MetadataTracker::tps() const {
    return tick.ups;
}

// Get the start time (when start() was called) of the game in ticks.
Uint64 MetadataTracker::getStartTicks() const {
    return startTicks;
}

const MetadataTracker* Metadata = NULL;