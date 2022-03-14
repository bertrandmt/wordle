#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    ThreadPool();
    ThreadPool(bool b) { }

    void push(std::function<void()> job);
    void done();
    void thread_function(int);

    int num_threads() const { return mNumThreads; }

private:
    std::queue<std::function<void()>> mJobQueue;
    std::mutex mLock;
    std::condition_variable mCond;
    bool mAcceptJobs;

    int mNumThreads;
    std::vector<std::thread> mPool;
};
