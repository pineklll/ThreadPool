# 详细解释 std::condition_variable

## 什么是条件变量？

`std::condition_variable` 是 C++11 引入的线程同步原语，用于线程间的通信。它允许线程在某些条件不满足时进入休眠状态，直到其他线程通知条件发生变化。

## 基本工作原理

条件变量总是与互斥锁（mutex）一起使用：
1. 线程获取互斥锁
2. 检查条件是否满足
3. 如果条件不满足，线程在条件变量上等待（自动释放锁）
4. 其他线程修改条件后，通知等待的线程
5. 等待的线程被唤醒，重新获取锁并继续执行

## 核心成员函数

### 1. **wait() - 等待条件**
```cpp
std::mutex mtx;
std::condition_variable cv;
bool ready = false;

// 基本wait
std::unique_lock<std::mutex> lock(mtx);
cv.wait(lock);  // 等待通知，可能虚假唤醒

// 带谓词的wait（推荐使用）
cv.wait(lock, []{ return ready; });  // 等价于：while (!ready) cv.wait(lock);
```

### 2. **notify_one() - 通知一个线程**
```cpp
{
    std::lock_guard<std::mutex> lock(mtx);
    ready = true;
}
cv.notify_one();  // 唤醒一个等待的线程
```

### 3. **notify_all() - 通知所有线程**
```cpp
cv.notify_all();  // 唤醒所有等待的线程
```

### 4. **wait_for() 和 wait_until() - 超时等待**
```cpp
// 等待最多1秒
if (cv.wait_for(lock, std::chrono::seconds(1), []{ return ready; })) {
    // 条件在超时前满足
} else {
    // 超时
}

// 等待到指定时间点
auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
cv.wait_until(lock, timeout, []{ return ready; });
```

## 详细示例

### 示例1：生产者-消费者模式（单生产者单消费者）
```cpp
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

std::queue<int> data_queue;
std::mutex mtx;
std::condition_variable cv;
const int MAX_SIZE = 10;

void producer() {
    for (int i = 0; i < 20; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::unique_lock<std::mutex> lock(mtx);
        
        // 等待队列不满
        cv.wait(lock, []{ return data_queue.size() < MAX_SIZE; });
        
        data_queue.push(i);
        std::cout << "Produced: " << i << std::endl;
        
        lock.unlock();
        cv.notify_one();  // 通知消费者
    }
}

void consumer() {
    for (int i = 0; i < 20; ++i) {
        std::unique_lock<std::mutex> lock(mtx);
        
        // 等待队列不空
        cv.wait(lock, []{ return !data_queue.empty(); });
        
        int data = data_queue.front();
        data_queue.pop();
        std::cout << "Consumed: " << data << std::endl;
        
        lock.unlock();
        cv.notify_one();  // 通知生产者
    }
}

int main() {
    std::thread p(producer);
    std::thread c(consumer);
    
    p.join();
    c.join();
    return 0;
}
```

### 示例2：线程池任务分发
```cpp
#include <vector>
#include <functional>
#include <atomic>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stop{false};
    
public:
    ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });
                        
                        if (stop && tasks.empty()) return;
                        
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    
                    task();
                }
            });
        }
    }
    
    template<class F>
    void enqueue(F&& task) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.emplace(std::forward<F>(task));
        }
        cv.notify_one();
    }
    
    ~ThreadPool() {
        stop = true;
        cv.notify_all();
        for (auto& worker : workers) {
            worker.join();
        }
    }
};
```

## 重要概念和注意事项

### 1. **虚假唤醒 (Spurious Wakeup)**
条件变量的 `wait()` 可能在没有收到 `notify` 的情况下返回。这是允许的行为，因此必须使用循环或带谓词的 `wait` 来检查条件。

```cpp
// 正确做法：使用循环检查
while (!condition) {
    cv.wait(lock);
}

// 更好做法：使用带谓词的wait（内部就是循环检查）
cv.wait(lock, []{ return condition; });
```

### 2. **lost wakeup 问题**
如果通知在等待开始前发生，通知可能丢失。正确的模式是：
```cpp
// 通知方
{
    std::lock_guard<std::mutex> lock(mtx);
    ready = true;  // 1. 修改条件（持有锁）
}
cv.notify_one();   // 2. 发送通知（释放锁后）

// 等待方
std::unique_lock<std::mutex> lock(mtx);
cv.wait(lock, []{ return ready; });  // 原子地：释放锁、等待、重新获取锁
```

### 3. **条件变量与锁的配合**
必须使用 `std::unique_lock` 而不是 `std::lock_guard`：
```cpp
std::unique_lock<std::mutex> lock(mtx);  // ✓ 正确
// std::lock_guard<std::mutex> lock(mtx);  // ✗ 错误！不能解锁
cv.wait(lock);
```

### 4. **多条件变量的使用**
```cpp
class BoundedBuffer {
    std::queue<int> buffer;
    const size_t capacity;
    std::mutex mtx;
    std::condition_variable not_full;   // 不满条件
    std::condition_variable not_empty;  // 不空条件
    
public:
    void put(int value) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [this]{ return buffer.size() < capacity; });
        buffer.push(value);
        lock.unlock();
        not_empty.notify_one();
    }
    
    int get() {
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [this]{ return !buffer.empty(); });
        int value = buffer.front();
        buffer.pop();
        lock.unlock();
        not_full.notify_one();
        return value;
    }
};
```

## 高级用法和模式

### 1. **读写锁模拟**
```cpp
class ReadWriteLock {
    std::mutex mtx;
    std::condition_variable cv;
    int readers = 0;
    bool writing = false;
    
public:
    void read_lock() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]{ return !writing; });
        ++readers;
    }
    
    void read_unlock() {
        std::lock_guard<std::mutex> lock(mtx);
        if (--readers == 0) {
            cv.notify_all();
        }
    }
    
    void write_lock() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]{ return !writing && readers == 0; });
        writing = true;
    }
    
    void write_unlock() {
        std::lock_guard<std::mutex> lock(mtx);
        writing = false;
        cv.notify_all();
    }
};
```

### 2. **倒计时门闩 (CountDownLatch)**
```cpp
class CountDownLatch {
    std::mutex mtx;
    std::condition_variable cv;
    int count;
    
public:
    explicit CountDownLatch(int initial) : count(initial) {}
    
    void await() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]{ return count == 0; });
    }
    
    void count_down() {
        std::lock_guard<std::mutex> lock(mtx);
        if (--count == 0) {
            cv.notify_all();
        }
    }
};
```

### 3. **屏障 (Barrier)**
```cpp
class Barrier {
    std::mutex mtx;
    std::condition_variable cv;
    const size_t thread_count;
    size_t waiting_count = 0;
    size_t generation = 0;
    
public:
    explicit Barrier(size_t count) : thread_count(count) {}
    
    void wait() {
        std::unique_lock<std::mutex> lock(mtx);
        size_t gen = generation;
        
        if (++waiting_count == thread_count) {
            ++generation;
            waiting_count = 0;
            cv.notify_all();
        } else {
            cv.wait(lock, [this, gen]{ return gen != generation; });
        }
    }
};
```

## 性能优化建议

1. **减少锁竞争**：尽量缩小临界区
2. **选择合适的通知方式**：
   - `notify_one()`：只唤醒一个线程，减少上下文切换
   - `notify_all()`：唤醒所有线程，适合广播场景
3. **避免过早唤醒**：确保条件真正满足再通知
4. **考虑使用 std::condition_variable_any**：
   ```cpp
   std::condition_variable_any cv;  // 可与任何满足BasicLockable的对象工作
   std::shared_mutex rw_mtx;
   std::unique_lock<std::shared_mutex> lock(rw_mtx);
   cv.wait(lock);
   ```

## 常见错误

### 错误1：忘记检查条件
```cpp
// 错误：可能虚假唤醒
cv.wait(lock);

// 正确：使用谓词检查
cv.wait(lock, []{ return ready; });
```

### 错误2：通知时未持有锁（可能导致竞争）
```cpp
// 可能有问题：通知和条件修改不是原子的
ready = true;       // 没有保护
cv.notify_one();    // 可能发生在等待线程检查条件之前

// 正确：在锁保护下修改条件
{
    std::lock_guard<std::mutex> lock(mtx);
    ready = true;
}
cv.notify_one();
```

### 错误3：使用 lock_guard
```cpp
std::lock_guard<std::mutex> lock(mtx);  // 错误！不能解锁
cv.wait(lock);  // 编译错误：wait需要能解锁

std::unique_lock<std::mutex> lock(mtx);  // 正确
cv.wait(lock);
```

## 总结

`std::condition_variable` 是多线程编程中的关键同步工具，它：
1. 提供了线程间高效通信的机制
2. 解决了忙等待（busy-waiting）的性能问题
3. 使得线程可以在条件不满足时休眠，节省CPU资源

使用时务必注意：
- 总是与互斥锁配合使用
- 使用 `std::unique_lock` 而非 `std::lock_guard`
- 使用带谓词的 `wait()` 避免虚假唤醒
- 确保条件修改和通知的正确顺序