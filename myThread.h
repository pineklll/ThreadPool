#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class MyThread {
    public:
        MyThread(size_t threads);

        template<class F, class... Args>
        auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

        ~MyThread();

    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
};

inline MyThread::~MyThread() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread& worker: workers)
        worker.join();
}

#endif