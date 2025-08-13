#include "utils/Metadata.hpp"
#include <SDL3/SDL_timer.h>

UpdateMetadata::UpdateMetadata(Uint64 targetUpdatesPerSecond) : deltaTime(NAN), currentPerfCount(SDL_GetPerformanceCounter()), lastPerfCount(SDL_GetPerformanceCounter()),
targetUpdatesPerSecond(targetUpdatesPerSecond), updateCount(0) {
    
}

Tick UpdateMetadata::update() {
    lastPerfCount = currentPerfCount;
    currentPerfCount = SDL_GetPerformanceCounter();
    if (updateCount == 0) {
        deltaTime = 0.0;
    } else {
        deltaTime = (double)((currentPerfCount - lastPerfCount) * 
            1000.0/(double)SDL_GetPerformanceFrequency());
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

    timestamp = SDL_GetTicks();

    return ++updateCount;
}

MetadataTracker::MetadataTracker(Uint32 targetFps, Uint32 targetTps, bool vsync)
: frame(targetFps), tick(targetTps) {
    startTicks = 0; // just in case start() is never called.
    vsyncEnabled = vsync;
}

Uint64 MetadataTracker::start() {
    startTicks = SDL_GetTicks();
    return startTicks;
}

double MetadataTracker::end() {
    return seconds();
}

Tick MetadataTracker::newFrame() {
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

Uint64 MetadataTracker::miliseconds() const {
    return SDL_GetTicks() - startTicks;
}

double MetadataTracker::seconds() const {
    return (SDL_GetTicks() - startTicks) / 1000.0f;
}

// Get the start time (when start() was called) of the game in ticks.
Uint64 MetadataTracker::getStartTicks() const {
    return startTicks;
}

const MetadataTracker* Metadata = NULL;