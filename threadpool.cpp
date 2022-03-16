// Copyright (c) 2022, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include <iostream>

#include "config.h"
#include "threadpool.h"

ThreadPool::ThreadPool()
    : mAcceptJobs(true)
    , mNumThreads(2 * std::thread::hardware_concurrency()) {

#if DEBUG_THREAD_POOL
    std::cout << "Constructing pool with " << mNumThreads << " threads." << std::endl;
#endif // DEBUG_THREAD_POOL
    for (int i = 0; i < mNumThreads; i++) {
        mPool.push_back(std::thread(&ThreadPool::thread_function, this, i));
    }
}

void ThreadPool::push(std::function<void()> job) {
    std::unique_lock<std::mutex> lock(mLock);
    mJobQueue.push(job);
    lock.unlock(); // when we send the notification immediately, the consumer will try to get the lock , so unlock asap
    mCond.notify_one();
}

void ThreadPool::done() {
    std::unique_lock<std::mutex> lock(mLock);
    mAcceptJobs = false;
    lock.unlock(); // when we send the notification immediately, the consumer will try to get the lock , so unlock asap
    mCond.notify_all();

    for (std::thread &thread : mPool) {
        thread.join();
    }
}

void ThreadPool::thread_function(int i) {
    std::function<void()> job;
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mLock);
            mCond.wait(lock, [this]() { return !mJobQueue.empty() || !mAcceptJobs; });
            if (!mAcceptJobs && mJobQueue.empty()) {
                break;
            }
            job = mJobQueue.front();
            mJobQueue.pop();
        }
        job();
    }
}

