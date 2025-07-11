#include "utils/system/signpost.hpp"
#include "utils/system/sysinfo.hpp"
#include "utils/Log.hpp"

#ifdef MACOS

#include <os/log.h>
#include <os/signpost.h>

static os_log_t macAppLog;



void createMacAppLog() {
    macAppLog = os_log_create("faketorio.app", "performance");
}

#define SIGNPOST(name)

// broken i think

// void mac_signpost(const char* name) {
//     static os_signpost_id_t lastSpid = OS_SIGNPOST_ID_NULL;
//     static std::string lastName;
//     if (lastSpid != OS_SIGNPOST_ID_NULL) {
//         const char* ch = lastName.c_str();
//         os_signpost_interval_end(macAppLog, lastSpid, "Signpost", "%s", lastName.c_str());
//     } else {
//         createMacAppLog();
//     }
//     os_signpost_id_t spid = os_signpost_id_generate(macAppLog);
//     lastSpid = spid;
//     lastName = name;

//     os_signpost_interval_begin(macAppLog, spid, "Signpost", "%s", name);
// }
#endif

int signpost(const char* name) {
#ifdef MACOS
    // mac_signpost(name);
#endif
    LogInfo("Starting %s", name);
    return 0;
}