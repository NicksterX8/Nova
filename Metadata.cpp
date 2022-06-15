#include "Metadata.hpp"
#include "constants.hpp"

MetadataInfo::MetadataInfo(double targetFps, bool vsync) {
    _fps = 0.0;
    this->targetFps = targetFps;
    deltaTime = 0.0f;
    _ticks = 0;
    startTicks = 0; // just in case start() is never called.
    vsyncEnabled = vsync;
}

Uint32 MetadataInfo::start() {
    startTicks = GetTicks();
    return startTicks;
}

double MetadataInfo::end() {
    return (GetTicks() - startTicks) / 1000.0f;
}

double MetadataInfo::tick() {
    perfCountLastTick = perfCountCurrentTick;
    perfCountCurrentTick = GetPerformanceCount();
    deltaTime = (double)((perfCountCurrentTick - perfCountLastTick) * 
        1000/(double)GetPerformanceFrequency());
    _fps = 1000.0f / deltaTime;
    _ticks++;
    return deltaTime;
}

double MetadataInfo::fps() {
    return _fps;
}

void MetadataInfo::setTargetFps(double newTargetFps) {
    targetFps = newTargetFps;
}
double MetadataInfo::getTargetFps() {
    return targetFps;
}

// Get the number of ticks that have passed
int MetadataInfo::ticks() {
    return _ticks;
}

// Get the start time (when start() was called) of the game in ticks.
Uint32 MetadataInfo::getStartTicks() {
    return startTicks;
}

MetadataInfo Metadata = MetadataInfo(TARGET_FPS, ENABLE_VSYNC);