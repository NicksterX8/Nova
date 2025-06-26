#include "threads.hpp"

namespace Threads {

int threadEntry(void* threadDataPtr) {
    ThreadData* threadData = (ThreadData*)threadDataPtr;
    while (1) {
        ThreadData::ManagingData data = threadData->data.load();
        uint32_t flags = data.flags;
        if (flags & ThreadData::FlagQuit) {
            break;
        }
        if (flags & ThreadData::FlagTaskAvailable && !(flags & ThreadData::FlagTaskComplete)) {
            int ret = data.function(data.userdata);
            data.flags |= ThreadData::FlagTaskComplete;
            data.functionReturn = ret;
            threadData->data.store(data);
        } else {
            SDL_Delay(0);
        }
    }
    return 0;
}

ThreadID ThreadManager::openThread(ThreadFunction function, void* userdata) {
    if (closedThreads.empty()) return NullThread;
    int unusedThread = closedThreads.back();
    closedThreads.pop_back();
    ThreadObject& thread = threads[unusedThread];
    thread.opened = true;
    ThreadData::ManagingData data{
        .flags = ThreadData::FlagTaskAvailable,
        .function = function,
        .userdata = userdata
    };
    thread.data->data.store(data);
    return threadIDs[unusedThread];
}

// same as wait thread but doesn't wait
void ThreadManager::closeThread(ThreadID threadID) {
    if (threadID == NullThread) return;
    int threadIndex = getThreadIndex(threadID);
    if (threadIndex == -1) {
        LogError("Failed to close thread with id %d!", threadID);
        return;
    }
    ThreadObject* thread = &threads[threadIndex];
    thread->opened = false;
    // don't think this is necessary. could be good for debugging tho
    // auto data = thread->data->data.load();
    // data.userdata = nullptr;
    // assert(data.flags | ThreadData::FlagTaskAvailable);
    // data.flags ^= ThreadData::FlagTaskAvailable;
    // thread->data->data.store(data);
    closedThreads.push_back(threadIndex);
}

int ThreadManager::waitThread(ThreadID threadID) {
    if (threadID == NullThread) {
        LogError("Tried to wait on null thread!");
        return -999;
    }
    int threadIndex = getThreadIndex(threadID);
    if (threadIndex == -1) {
        LogError("There is no thread with id %d!", threadID);
        return -998;
    }
    ThreadObject* thread = &threads[threadIndex];
    assert(thread->opened && "Thread wasn't open in the first place!");
    auto data = readThreadManagingData(*thread);
    assert(data.flags & ThreadData::FlagTaskAvailable);
    // spin lock until task complete
    int iterations = 0;
    while (!(data.flags & ThreadData::FlagTaskComplete)) {
        data = readThreadManagingData(*thread);
        if (++iterations > 100000000) {
            LogError("Never ending!");
            return -1;
        }
    }
    // task complete
    thread->opened = false;
    closedThreads.push_back(threadIndex);
    return data.functionReturn;
}

void ThreadManager::initThreads(int threadCount) {
    char nameBuf[64];
    for (int i = 0; i < threadCount; i++) {
        ThreadData* threadData = new ThreadData();
        snprintf(nameBuf, 64, "9991%zu", threads.size());
        SDL_Thread* sdlThread = SDL_CreateThread(threadEntry, nameBuf, threadData);
        // let the threads take care of destructing themselves
        threads.push_back({sdlThread, threadData, false});
        ThreadID id = threadIDCounter++;
        threadIDs.push_back(id);
        closedThreads.push_back(i);
    }

    LogInfo("Made %d threads", threadCount);
}

void ThreadManager::destroyThreads(ThreadObject* threadsToBeDestroyed, int count, bool destroyOpenedThreads) {
    for (int i = 0; i < count; i++) {
        auto& thread = threads[i];
        if (!destroyOpenedThreads && thread.opened) continue;
        auto data = thread.data->data.load();
        data.flags |= ThreadData::FlagQuit;
        thread.data->data.store(data);
    }

    for (int i = 0; i < count; i++) {
        auto& thread = threadsToBeDestroyed[i];
        if (!destroyOpenedThreads && thread.opened) continue;
        int retValue;
        SDL_WaitThread(thread.thread, &retValue);
        if (retValue != 0) {
            LogError("Return value is weird! %d", retValue);
        }
        if (thread.data) delete thread.data;
    }
}

void ThreadManager::destroy() {
    destroyThreads(threads.data(), threads.size());
    threads.clear();
    threadIDs.clear();
    closedThreads.clear();
}

}