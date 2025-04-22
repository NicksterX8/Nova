#include "utils/Debug.hpp"

DebugClass::DebugClass() {
    /* Initialize all settings to 0 */
    settings.init();
}


const DebugClass* Debug = NULL;