# 深入解析 `std::thread`

## 1. **什么是 `std::thread`？**

`std::thread` 是 C++11 引入的标准线程库，用于创建和管理操作系统线程。它是跨平台的线程抽象，替代了平台特定的 API（如 pthread、Windows Thread）。

## 2. **基本用法**

### 创建线程的几种方式

```cpp
#include <iostream>
#include <thread>
#include <chrono>

// 方式1：普通函数
void simple_task() {
    std::cout << "Thread ID: " << std::this_thread::get_id() << std::endl;
}

// 方式2：函数对象（仿函数）
class Functor {
public:
    void operator()() {
        std::cout << "Functor thread\n";
    }
};

// 方式3：Lambda 表达式
auto lambda = []() {
    std::cout << "Lambda thread\n";
};

// 方式4：成员函数
class Worker {
public:
    void do_work() {
        std::cout << "Member function thread\n";
    }
    
    static void static_work() {
        std::cout << "Static member function thread\n";
    }
};

int main() {
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    // 1. 普通函数
    std::thread t1(simple_task);
    
    // 2. 函数对象
    Functor func;
    std::thread t2(func);
    
    // 3. Lambda
    std::thread t3(lambda);
    
    // 4. 成员函数（需要对象实例）
    Worker worker;
    std::thread t4(&Worker::do_work, &worker);
    
    // 5. 静态成员函数
    std::thread t5(&Worker::static_work);
    
    // 6. 带参数的函数
    std::thread t6([](int x, const std::string& s) {
        std::cout << "Params: " << x << ", " << s << std::endl;
    }, 42, "hello");
    
    // 等待所有线程完成
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    
    return 0;
}
```

## 3. **线程的生命周期管理**

### 线程的三种状态
```cpp
#include <thread>
#include <iostream>

int main() {
    // 1. 构造即启动（默认立即开始执行）
    std::thread t([]() {
        std::cout << "Thread started\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Thread finished\n";
    });
    
    // 2. joinable() 检查
    if (t.joinable()) {
        std::cout << "Thread is joinable\n";
    }
    
    // 3. 必须 join() 或 detach() 一个 joinable 线程
    t.join();  // 等待线程结束
    
    // 4. 线程结束后
    if (!t.joinable()) {
        std::cout << "Thread is no longer joinable\n";
    }
    
    return 0;
}
```

## 4. **`join()` vs `detach()`**

```cpp
#include <thread>
#include <iostream>
#include <chrono>

void worker(int id) {
    std::cout << "Worker " << id << " started\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Worker " << id << " finished\n";
}

int main() {
    std::cout << "=== join() 示例 ===\n";
    {
        std::thread t1(worker, 1);
        std::thread t2(worker, 2);
        
        // join()：阻塞主线程，等待子线程完成
        t1.join();  // 等待 t1 完成
        t2.join();  // 等待 t2 完成
        
        std::cout << "All workers completed (joined)\n";
    }
    
    std::cout << "\n=== detach() 示例 ===\n";
    {
        std::thread t3(worker, 3);
        std::thread t4(worker, 4);
        
        // detach()：分离线程，使其在后台运行
        t3.detach();  // t3 现在独立运行
        t4.detach();  // t4 现在独立运行
        
        std::cout << "Workers detached, main continues\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // 注意：分离后无法再 join()
        // t3.join();  // 错误！线程已分离
    }  // 主线程结束，但分离的线程可能还在运行
    
    return 0;
}
```

## 5. **线程标识和当前线程操作**

```cpp
#include <thread>
#include <iostream>
#include <chrono>
#include <vector>

int main() {
    // 获取线程 ID
    std::thread::id main_id = std::this_thread::get_id();
    std::cout << "Main thread ID: " << main_id << std::endl;
    
    // 创建多个线程
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i]() {
            std::cout << "Thread " << i 
                      << " ID: " << std::this_thread::get_id() << std::endl;
            
            // 睡眠
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 让出 CPU
            std::this_thread::yield();
        });
    }
    
    // 获取硬件并发数
    unsigned int n = std::thread::hardware_concurrency();
    std::cout << "\nHardware concurrency: " << n << std::endl;
    
    // 等待所有线程
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

## 6. **传递参数给线程函数**

```cpp
#include <thread>
#include <iostream>
#include <string>
#include <memory>

void process_data(int id, const std::string& name, double value) {
    std::cout << "Thread " << id << ": " << name << " = " << value << std::endl;
}

void modify_data(int& data) {
    data *= 2;
    std::cout << "Modified data: " << data << std::endl;
}

void move_data(std::unique_ptr<int> ptr) {
    std::cout << "Moved data: " << *ptr << std::endl;
}

int main() {
    // 1. 值传递（默认）
    std::thread t1(process_data, 1, "test", 3.14);
    t1.join();
    
    // 2. 引用传递（需要使用 std::ref）
    int data = 42;
    std::thread t2(modify_data, std::ref(data));
    t2.join();
    std::cout << "Data after thread: " << data << std::endl;  // 84
    
    // 3. 移动语义传递
    auto ptr = std::make_unique<int>(100);
    std::thread t3(move_data, std::move(ptr));
    t3.join();
    // ptr 现在为空
    
    // 4. 传递数组（会退化为指针）
    int arr[] = {1, 2, 3, 4, 5};
    std::thread t4([](int* arr, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            std::cout << arr[i] << " ";
        }
        std::cout << std::endl;
    }, arr, 5);
    t4.join();
    
    // 5. 传递成员函数和对象
    class Task {
    public:
        void run(int x) {
            std::cout << "Task::run(" << x << ")\n";
        }
    };
    
    Task task;
    std::thread t5(&Task::run, &task, 999);
    t5.join();
    
    return 0;
}
```

## 7. **线程异常安全**

```cpp
#include <thread>
#include <iostream>
#include <stdexcept>
#include <mutex>

std::mutex mtx;

void might_throw(bool should_throw) {
    if (should_throw) {
        throw std::runtime_error("Thread exception!");
    }
    std::cout << "Thread completed normally\n";
}

void safe_thread_function() {
    try {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "Thread ID: " << std::this_thread::get_id() << " working\n";
        // 可能抛出异常的操作
        throw std::logic_error("Something went wrong!");
    } catch (const std::exception& e) {
        std::cerr << "Thread caught: " << e.what() << std::endl;
    }
}

int main() {
    try {
        // 线程内部的异常不会传播到主线程
        std::thread t(might_throw, true);
        t.join();
    } catch (const std::exception& e) {
        // 这里不会捕获线程中的异常！
        std::cerr << "Main caught: " << e.what() << std::endl;
    }
    
    std::cout << "\n=== 异常安全模式 ===\n";
    
    // 安全的线程包装器
    class ThreadGuard {
        std::thread t;
    public:
        template<typename Callable, typename... Args>
        ThreadGuard(Callable&& func, Args&&... args)
            : t(std::forward<Callable>(func), std::forward<Args>(args)...) {}
        
        ~ThreadGuard() {
            if (t.joinable()) {
                t.join();
            }
        }
        
        ThreadGuard(const ThreadGuard&) = delete;
        ThreadGuard& operator=(const ThreadGuard&) = delete;
    };
    
    {
        // 使用 RAII 确保线程被 join
        ThreadGuard guard(safe_thread_function);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }  // guard 析构时自动 join
    
    return 0;
}
```

## 8. **线程局部存储（Thread-Local Storage）**

```cpp
#include <thread>
#include <iostream>
#include <vector>

// 线程局部变量
thread_local int tls_var = 0;
thread_local int thread_id = 0;

void worker(int id) {
    tls_var = id * 100;
    thread_id = id;
    
    for (int i = 0; i < 3; ++i) {
        ++tls_var;
        std::cout << "Thread " << thread_id 
                  << ": tls_var = " << tls_var 
                  << ", &tls_var = " << &tls_var 
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    std::cout << "Main thread tls_var = " << tls_var 
              << ", &tls_var = " << &tls_var << std::endl;
    
    std::vector<std::thread> threads;
    for (int i = 1; i <= 3; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // 每个线程有自己的 tls_var 副本
    std::cout << "\nMain thread tls_var still = " << tls_var << std::endl;
    
    return 0;
}
```

## 9. **线程池中的 `std::thread` 使用**

```cpp
#include <thread>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>

class SimpleThreadPool {
private:
    std::vector<std::thread> workers;      // 工作线程
    std::queue<std::function<void()>> tasks;  // 任务队列
    
    std::mutex queue_mutex;               // 保护任务队列
    std::condition_variable condition;    // 线程同步
    bool stop = false;                    // 停止标志
    
public:
    // 构造函数创建线程
    SimpleThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });
                        
                        if (stop && tasks.empty()) {
                            return;
                        }
                        
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    
                    task();  // 执行任务
                }
            });
        }
    }
    
    // 添加任务
    template<typename F>
    void enqueue(F&& task) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(task));
        }
        condition.notify_one();
    }
    
    // 析构函数
    ~SimpleThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    // 禁止拷贝
    SimpleThreadPool(const SimpleThreadPool&) = delete;
    SimpleThreadPool& operator=(const SimpleThreadPool&) = delete;
};

int main() {
    SimpleThreadPool pool(4);
    
    // 提交任务
    for (int i = 0; i < 10; ++i) {
        pool.enqueue([i]() {
            std::cout << "Task " << i 
                      << " executed by thread " 
                      << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    return 0;
}
```

## 10. **`std::thread` 的移动语义**

```cpp
#include <thread>
#include <iostream>
#include <vector>

void task(int id) {
    std::cout << "Task " << id 
              << " in thread " << std::this_thread::get_id() << std::endl;
}

int main() {
    // 1. 默认构造（不表示任何线程）
    std::thread empty_thread;
    std::cout << "Empty thread joinable? " << empty_thread.joinable() << std::endl;
    
    // 2. 移动构造
    std::thread t1(task, 1);
    std::cout << "t1 joinable? " << t1.joinable() << std::endl;
    
    std::thread t2 = std::move(t1);  // 移动构造
    std::cout << "After move:\n";
    std::cout << "t1 joinable? " << t1.joinable() << std::endl;  // false
    std::cout << "t2 joinable? " << t2.joinable() << std::endl;  // true
    
    // 3. 移动赋值
    std::thread t3;
    t3 = std::move(t2);  // 移动赋值
    std::cout << "\nAfter move assignment:\n";
    std::cout << "t2 joinable? " << t2.joinable() << std::endl;  // false
    std::cout << "t3 joinable? " << t3.joinable() << std::endl;  // true
    
    // 4. 存储在容器中
    std::vector<std::thread> threads;
    threads.reserve(5);
    
    for (int i = 0; i < 5; ++i) {
        // emplace_back 可以直接构造
        threads.emplace_back(task, i);
    }
    
    // 必须移动，因为 thread 不可拷贝
    std::thread t4(task, 99);
    threads.push_back(std::move(t4));
    
    // 等待所有线程
    t3.join();
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    return 0;
}
```

## 11. **平台特定的线程句柄**

```cpp
#include <thread>
#include <iostream>
#include <pthread.h>  // POSIX 线程
#include <windows.h>  // Windows 线程

void native_handle_demo() {
    std::thread t([]() {
        std::cout << "Thread running\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });
    
    // 获取原生线程句柄
    #ifdef __linux__
        pthread_t pthread_id = t.native_handle();
        std::cout << "POSIX thread ID: " << pthread_id << std::endl;
        
        // 可以设置 POSIX 线程属性
        int policy;
        sched_param param;
        pthread_getschedparam(pthread_id, &policy, &param);
        std::cout << "Scheduling policy: " << policy << std::endl;
        
    #elif _WIN32
        HANDLE win_handle = t.native_handle();
        std::cout << "Windows thread handle: " << win_handle << std::endl;
        
        // 可以设置 Windows 线程优先级
        SetThreadPriority(win_handle, THREAD_PRIORITY_NORMAL);
    #endif
    
    t.join();
}

int main() {
    native_handle_demo();
    return 0;
}
```

## 12. **常见错误和最佳实践**

### 错误示例：忘记 join/detach
```cpp
// 错误：线程对象析构时仍 joinable
void wrong_example() {
    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    });
    // 忘记调用 t.join() 或 t.detach()
    // 程序终止时会调用 std::terminate()
}  // t 析构时，如果仍 joinable，会终止程序

// 正确：使用 RAII
class ThreadRAII {
    std::thread t;
    bool joined = false;
public:
    template<typename... Args>
    ThreadRAII(Args&&... args) : t(std::forward<Args>(args)...) {}
    
    ~ThreadRAII() {
        if (t.joinable()) {
            if (!joined) {
                t.detach();  // 或 t.join()，根据需求
            }
        }
    }
    
    void join() {
        if (t.joinable()) {
            t.join();
            joined = true;
        }
    }
    
    // 禁止拷贝
    ThreadRAII(const ThreadRAII&) = delete;
    ThreadRAII& operator=(const ThreadRAII&) = delete;
};
```

### 最佳实践总结
```cpp
// 1. 总是确保线程被 join 或 detach
std::thread t([]{ /*...*/ });
// 必须调用以下之一：
t.join();    // 等待完成
// 或
t.detach();  // 分离运行

// 2. 使用 RAII 管理线程生命周期
{
    ThreadRAII t([]{ /*...*/ });
    // ...
}  // 自动处理

// 3. 传递引用时使用 std::ref
int data = 42;
std::thread t([](int& d) { d = 100; }, std::ref(data));
t.join();

// 4. 避免数据竞争
std::mutex mtx;
int shared_data = 0;
std::thread t1([&]() {
    std::lock_guard<std::mutex> lock(mtx);
    ++shared_data;
});

// 5. 使用 thread_local 替代全局变量
thread_local int local_data = 0;

// 6. 考虑线程亲和性（对性能敏感的应用）
// 可以使用 native_handle() 设置 CPU 亲和性
```

## 13. **性能考虑和线程数量**

```cpp
#include <thread>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

void benchmark_threads() {
    const size_t num_elements = 1000000;
    std::vector<int> data(num_elements);
    std::generate(data.begin(), data.end(), []() {
        return rand() % 100;
    });
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 串行处理
    long long sum_serial = 0;
    for (int val : data) {
        sum_serial += val;
    }
    
    auto mid = std::chrono::high_resolution_clock::now();
    
    // 并行处理
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 2;
    
    std::vector<std::thread> threads;
    std::vector<long long> partial_sums(num_threads, 0);
    
    size_t chunk_size = data.size() / num_threads;
    
    for (unsigned int i = 0; i < num_threads; ++i) {
        size_t start_idx = i * chunk_size;
        size_t end_idx = (i == num_threads - 1) ? data.size() : start_idx + chunk_size;
        
        threads.emplace_back([&data, &partial_sums, i, start_idx, end_idx]() {
            long long sum = 0;
            for (size_t j = start_idx; j < end_idx; ++j) {
                sum += data[j];
            }
            partial_sums[i] = sum;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    long long sum_parallel = 0;
    for (long long partial : partial_sums) {
        sum_parallel += partial;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration_serial = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
    auto duration_parallel = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);
    
    std::cout << "Hardware concurrency: " << num_threads << std::endl;
    std::cout << "Serial sum: " << sum_serial << ", time: " << duration_serial.count() << "μs" << std::endl;
    std::cout << "Parallel sum: " << sum_parallel << ", time: " << duration_parallel.count() << "μs" << std::endl;
    std::cout << "Speedup: " << static_cast<double>(duration_serial.count()) / duration_parallel.count() << "x" << std::endl;
}

int main() {
    benchmark_threads();
    return 0;
}
```

## 14. **C++20 的新特性**

```cpp
// C++20 引入了 std::jthread（joining thread）
#include <thread>
#include <iostream>
#include <stop_token>

void cpp20_features() {
    // 1. std::jthread：自动 join 的线程
    std::jthread jt([]() {
        std::cout << "jthread running\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "jthread finished\n";
    });
    // 不需要手动 join，析构时自动 join
    
    // 2. 协程支持
    auto coroutine_example = []() -> std::jthread {
        std::cout << "Thread with coroutine support\n";
        co_await std::suspend_always{};
        co_return;
    };
    
    // 3. 停止令牌（stop token）
    std::jthread stoppable([](std::stop_token token) {
        while (!token.stop_requested()) {
            std::cout << "Working...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << "Stopped by request\n";
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    stoppable.request_stop();  // 请求停止
}
```

## 15. **总结**

`std::thread` 是 C++ 标准库中的核心并发组件：

### 关键特性：
1. **跨平台**：统一了不同操作系统的线程 API
2. **类型安全**：编译时检查，避免 C 风格线程 API 的错误
3. **移动语义**：支持移动构造和移动赋值，可以存储在容器中
4. **灵活的参数传递**：支持值传递、引用传递（需要 `std::ref`）、移动语义
5. **RAII 支持**：可以与其他 RAII 类型结合使用

### 重要概念：
- **`join()`**：等待线程完成
- **`detach()`**：分离线程，使其独立运行
- **`joinable()`**：检查线程是否可 join
- **`get_id()`**：获取线程标识符
- **`hardware_concurrency()`**：获取硬件支持的并发线程数
- **`native_handle()`**：获取平台特定的线程句柄

### 在线程池中的应用：
在你的 ThreadPool 代码中，`std::thread` 用于创建工作线程：
```cpp
workers.emplace_back([this] {
    // 工作线程的主循环
    while (true) {
        // 等待并执行任务
    }
});
```

**最佳实践**：总是使用 RAII 或确保线程在销毁前被正确 join/detach，避免程序意外终止。