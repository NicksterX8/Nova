#include "utils/Metadata.hpp"
#include "constants.hpp"

MetadataTracker::MetadataTracker(double targetFps, bool vsync) {
    _fps = 0.0;
    this->targetFps = targetFps;
    deltaTime = 0.0f;
    _ticks = 0;
    startTicks = 0; // just in case start() is never called.
    vsyncEnabled = vsync;
}

Uint32 MetadataTracker::start() {
    startTicks = GetTicks();
    return startTicks;
}

double MetadataTracker::end() {
    return seconds();
}

double MetadataTracker::tick() {
    perfCountLastTick = perfCountCurrentTick;
    perfCountCurrentTick = GetPerformanceCounter();
    if (_ticks == 0) {
        deltaTime = 0.0;
        _fps = 0.0;
    } else {
        deltaTime = (double)((perfCountCurrentTick - perfCountLastTick) * 
            1000/(double)GetPerformanceFrequency());
        _fps = 1000.0 / deltaTime;
    }
    _ticks++;
    return deltaTime;
}

double MetadataTracker::fps() const {
    return _fps;
}

void MetadataTracker::setTargetFps(double newTargetFps) {
    targetFps = newTargetFps;
}
double MetadataTracker::getTargetFps() const {
    return targetFps;
}

// Get the number of ticks that have passed
int MetadataTracker::ticks() const {
    return _ticks;
}

// Get the start time (when start() was called) of the game in ticks.
Uint32 MetadataTracker::getStartTicks() const {
    return startTicks;
}

const MetadataTracker* Metadata = NULL;