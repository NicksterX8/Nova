#include "utils/Debug.hpp"

DebugClass::DebugClass() {
    /* Initialize all settings to 0 */
    resetSettings();
}

void DebugClass::resetSettings() {
    char allzeros[sizeof(settings)] = {0};
    memcpy(&settings, allzeros, sizeof(settings));
}

const DebugClass* Debug = NULL;