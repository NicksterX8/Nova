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
        if (flags & ThreadData::FlagTaskAvailable) {
            int ret = data.function(threadData->userdata);
            data.flags |= ThreadData::FlagTaskComplete;
            data.flags ^= ThreadData::FlagTaskAvailable; // task no longer available as it is completed
            data.functionReturn = ret;
            threadData->data.store(data);
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
    thread.data->userdata = userdata;
    ThreadData::ManagingData data{
        .flags = ThreadData::FlagTaskAvailable,
        .function = function
    };
    thread.data->data.store(data);
    return unusedThread;
}

void ThreadManager::closeThread(ThreadID threadID) {
    if (threadID == NullThread) return;
    ThreadObject* thread = getThread(threadID);
    if (!thread) {
        LogError("Failed to close thread with id %d!", threadID);
        return;
    }
    thread->opened = false;
    thread->data->userdata = nullptr;
    closedThreads.push_back(threadID);
}

int ThreadManager::waitThread(ThreadID threadID) {
    if (threadID == NullThread) {
        LogError("Tried to wait on null thread!");
        return -999;
    }
    ThreadObject* thread = getThread(threadID);
    if (!thread) {
        LogError("There is no thread with id %d!", threadID);
        return -998;
    }
    auto data = readThreadManagingData(*thread);
    assert(data.flags & ThreadData::FlagTaskAvailable);
    // spin lock until task complete
    while (!(data.flags & ThreadData::FlagTaskComplete)) {
        data = readThreadManagingData(*thread);
    }
    // task complete
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
        closedThreads.push_back(id);
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