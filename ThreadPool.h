//
// THREAD POOL IMPLEMENTATION
// This header-only library provides a simple thread pool for executing tasks concurrently.
//
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>              // For storing worker threads
#include <queue>               // For task queue
#include <memory>              // For shared_ptr and smart pointers
#include <thread>              // For std::thread
#include <mutex>               // For mutual exclusion locks
#include <condition_variable>  // For thread synchronization
#include <future>              // For std::future and std::promise
#include <functional>          // For std::function and std::bind
#include <stdexcept>           // For std::runtime_error

/*
 * ThreadPool class
 * Manages a fixed number of worker threads that execute tasks from a shared queue.
 * Tasks are submitted using the enqueue() method and executed asynchronously.
 */
class ThreadPool {
public:
    /*
     * Constructor
     * @param threads Number of worker threads to create
     * Creates the specified number of worker threads that wait for tasks
     */
    ThreadPool(size_t threads);
    
    /*
     * Template method to enqueue a task
     * @param f Function to execute
     * @param args Arguments to pass to the function
     * @return std::future containing the result of the function when it completes
     */
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    /*
     * Destructor
     * Ensures all worker threads are properly joined before destruction
     */
    ~ThreadPool();

private:
    // Vector to store the worker threads
    std::vector< std::thread > workers;
    
    // Queue to store tasks (as function objects) waiting to be executed
    std::queue< std::function<void()> > tasks;
    
    // Mutex to protect access to the task queue
    std::mutex queue_mutex;
    
    // Condition variable to signal when tasks are available
    std::condition_variable condition;
    
    // Flag indicating whether the pool should stop accepting new tasks
    bool stop;
};
 
/*
 * ThreadPool Constructor Implementation
 * Creates the specified number of worker threads
 * Each worker thread runs an infinite loop processing tasks from the queue
 */
inline ThreadPool::ThreadPool(size_t threads)
    :   stop(false)  // Initially not stopping
{
    // Create the requested number of worker threads
    for(size_t i = 0; i < threads; ++i)
        workers.emplace_back(
            // Lambda function that defines the work each thread does, this stand for ThreadPool
            [this]
            {
                // Infinite loop - each thread keeps running until told to stop
                for(;;)
                {
                    // Variable to hold the task to execute
                    std::function<void()> task;

                    // Critical section - access shared task queue safely
                    {
                        // Acquire lock on the mutex to access shared data
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        
                        // Wait until either:
                        // 1. There's a task in the queue, OR
                        // 2. The pool is stopping
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        
                        // Exit condition: if pool is stopping AND queue is empty
                        if(this->stop && this->tasks.empty())
                            return;  // Exit the worker thread
                        
                        // Get the next task from the queue
                        task = std::move(this->tasks.front());
                        this->tasks.pop();  // Remove the task from the queue
                    }  // Release lock here - critical section ends

                    // Execute the task outside the critical section
                    // This allows other threads to access the queue while this task runs
                    task();
                }
            }
        );
}

/*
 * Enqueue Method Implementation
 * Adds a new task to the queue and returns a future for the result
 * 
 * This method:
 * 1. Wraps the function and arguments in a packaged_task
 * 2. Gets a future that will contain the result
 * 3. Adds the task to the queue
 * 4. Signals a waiting worker thread
 * 5. Returns the future
 */
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    // Determine the return type of the function
    using return_type = typename std::result_of<F(Args...)>::type;

    // Create a packaged_task that wraps the function call
    // packaged_task allows us to get a future for the result
    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...) 
        );
        
    // Get the future that will contain the result of the function
    std::future<return_type> res = task->get_future();
    
    // Critical section - add task to the queue safely
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // Check if the pool is still accepting tasks
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        // Add the task to the queue
        // The lambda captures the shared_ptr to the packaged_task
        tasks.emplace([task](){ (*task)(); });
    }  // Release lock - critical section ends
    
    // Signal one waiting worker thread that a task is available
    condition.notify_one();
    
    // Return the future so the caller can retrieve the result later
    return res;
}

/*
 * ThreadPool Destructor Implementation
 * Gracefully shuts down all worker threads
 * 
 * This method:
 * 1. Sets the stop flag to true
 * 2. Wakes up all waiting worker threads
 * 3. Joins all threads to wait for them to finish
 */
inline ThreadPool::~ThreadPool()
{
    // Critical section - set stop flag safely
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;  // Tell threads to stop accepting new tasks
    }
    
    // Wake up all waiting worker threads so they can notice the stop flag
    condition.notify_all();
    
    // Wait for all worker threads to finish their current tasks and exit
    for(std::thread &worker: workers)
        worker.join();
}

#endif
