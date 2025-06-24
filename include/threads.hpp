#ifndef THREADS_INCLUDED
#define THREADS_INCLUDED

#include <SDL3/SDL_thread.h>
#include <cassert>
#include <vector>
#include <atomic>
#include <functional>
#include "My/HashMap.hpp"
#include "utils/Log.hpp"

namespace Threads {

using ThreadFunction = int(*)(void*);

struct ThreadData {
    void* userdata = nullptr;
    struct ManagingData {
        uint32_t flags{0};
        ThreadFunction function = nullptr;
        int functionReturn = 0;
    };
    std::atomic<ManagingData> data;

    enum {
        FlagQuit = 1,
        FlagTaskAvailable = 2,
        FlagTaskComplete = 4
    };
};

struct ThreadObject {
    SDL_Thread* thread;
    ThreadData* data;
    bool opened;
};

struct ThreadTask {
    std::function<int(void*)> function;
};

int threadEntry(void* threadDataPtr);

using ThreadID = int;

struct ThreadManager {
    std::vector<ThreadObject> threads;
    static constexpr ThreadID NullThread = -1;
    std::vector<int> closedThreads; // indices of threads that are closed
    std::vector<ThreadID> threadIDs;

    ThreadID threadIDCounter = 1;
private:
    ThreadData::ManagingData readThreadManagingData(const ThreadObject& thread) {
        return thread.data->data.load();
    }

    void setThreadManagingData(ThreadObject& thread, const ThreadData::ManagingData& data) {
        thread.data->data.store(data);
    }

    ThreadObject* getThread(ThreadID id) {
        for (int i = 0; i < threads.size(); i++) {
            if (threadIDs[i] == id) {
                return &threads[i];
            }
        }
        return nullptr;
    }
public:
    ThreadManager() {}

    ThreadManager(int numThreads) {
        initThreads(numThreads);
    }

    ThreadID openThread(ThreadFunction function, void* userdata);

    void closeThread(ThreadID threadID);

    // returns the return of the function
    int waitThread(ThreadID threadID);

    void initThreads(int threadCount);

    void destroyThreads(ThreadObject* threadsToBeDestroyed, int count, bool destroyOpenedThreads = true);

    void destroyUnusedThreads() {
        destroyThreads(threads.data(), threads.size(), false);
    }

    // destroy thread manager and all associated threads
    void destroy();

    // returns the total number of threads initalized by the manager
    int totalThreads() const {
        return threads.size();
    }

    // returns the number of threads initialized but not being used
    int unusedThreads() const {
        return closedThreads.size();
    }
};

}

using Threads::ThreadManager;

#endif