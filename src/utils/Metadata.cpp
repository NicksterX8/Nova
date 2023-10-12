#include "utils/Metadata.hpp"
#include "constants.hpp"

MetadataTracker::MetadataTracker(double targetFps, bool vsync) {
    _fps = 0.0;
    this->targetFps = targetFps;
    deltaTime = 0.0f;
    _frames = 0;
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

FrameCount MetadataTracker::newFrame() {
    perfCountLastTick = perfCountCurrentTick;
    perfCountCurrentTick = GetPerformanceCounter();
    if (_frames == 0) {
        deltaTime = 0.0;
        _fps = 0.0;
    } else {
        deltaTime = (double)((perfCountCurrentTick - perfCountLastTick) * 
            1000/(double)GetPerformanceFrequency());
        _fps = 1000.0 / deltaTime;
    }

    return ++_frames;
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
FrameCount MetadataTracker::currentFrame() const {
    return _frames;
}

// Get the start time (when start() was called) of the game in ticks.
Uint64 MetadataTracker::getStartTicks() const {
    return startTicks;
}

const MetadataTracker* Metadata = NULL;

Tick gTick = 0;