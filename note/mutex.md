我来详细解释一下 C++ 中的 `std::mutex`（互斥锁）。

## 什么是 std::mutex？

`std::mutex`（互斥锁）是 C++11 标准库中提供的一种同步原语，用于保护共享数据，防止多个线程同时访问，从而避免数据竞争。

## 主要特性

### 1. **基本使用**
```cpp
#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx;
int shared_data = 0;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        mtx.lock();      // 获取锁
        ++shared_data;   // 临界区代码
        mtx.unlock();    // 释放锁
    }
}
```

### 2. **使用 RAII 包装器**
更安全的方式是使用 `std::lock_guard` 或 `std::unique_lock`：

```cpp
void safe_increment() {
    for (int i = 0; i < 100000; ++i) {
        std::lock_guard<std::mutex> lock(mtx);  // 自动加锁
        ++shared_data;                           // 自动解锁
    }
}
```

## 成员函数

### 主要方法：
```cpp
std::mutex mtx;

mtx.lock();      // 获取锁，如果已被其他线程持有则阻塞
mtx.unlock();    // 释放锁

bool success = mtx.try_lock();  // 尝试获取锁，立即返回
                                 // true: 成功获取；false: 锁已被占用
```

## 不同种类的互斥锁

C++ 提供了多种互斥锁：

### 1. **std::mutex** - 基本互斥锁
```cpp
std::mutex mtx;  // 最基本的互斥锁
```

### 2. **std::recursive_mutex** - 递归锁
允许同一线程多次获取锁
```cpp
std::recursive_mutex rmtx;

void recursive_func(int n) {
    std::lock_guard<std::recursive_mutex> lock(rmtx);
    if (n > 0) {
        recursive_func(n - 1);  // 同一线程可以重复获取锁
    }
}
```

### 3. **std::timed_mutex** - 定时锁
```cpp
std::timed_mutex tmtx;

if (tmtx.try_lock_for(std::chrono::seconds(1))) {
    // 1秒内成功获取锁
    tmtx.unlock();
}

if (tmtx.try_lock_until(
    std::chrono::steady_clock::now() + std::chrono::milliseconds(500))) {
    // 在指定时间点前获取锁
    tmtx.unlock();
}
```

### 4. **std::recursive_timed_mutex** - 递归定时锁
结合了递归和定时特性

## 最佳实践示例

### 示例 1：保护共享资源
```cpp
class ThreadSafeCounter {
private:
    mutable std::mutex mtx;
    int value = 0;

public:
    void increment() {
        std::lock_guard<std::mutex> lock(mtx);
        ++value;
    }
    
    int get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return value;
    }
};
```

### 示例 2：避免死锁（使用 std::lock）
```cpp
std::mutex mtx1, mtx2;

// 错误方式：可能导致死锁
// thread1: lock(mtx1) -> lock(mtx2)
// thread2: lock(mtx2) -> lock(mtx1)

// 正确方式：同时锁定多个互斥锁
void safe_operation() {
    std::lock(mtx1, mtx2);  // 同时锁定，避免死锁
    std::lock_guard<std::mutex> lock1(mtx1, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(mtx2, std::adopt_lock);
    // 操作共享资源
}
```

### 示例 3：使用 unique_lock 提供更多控制
```cpp
std::mutex mtx;
std::condition_variable cv;
bool data_ready = false;

void producer() {
    std::unique_lock<std::mutex> lock(mtx);
    // 生产数据
    data_ready = true;
    lock.unlock();  // 可以手动解锁
    cv.notify_one();
}

void consumer() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, []{ return data_ready; });
    // 消费数据
}
```

## 重要注意事项

### 1. **不要忘记解锁**
```cpp
// 错误示例
mtx.lock();
if (error_condition) {
    return;  // 忘记解锁！
}
mtx.unlock();
```

### 2. **不要双重锁定**
```cpp
// 错误示例
mtx.lock();
mtx.lock();  // 未定义行为（除非使用递归锁）
```

### 3. **不要跨线程解锁**
```cpp
// 错误示例
void thread_func() {
    mtx.lock();
}

// 另一个线程
mtx.unlock();  // 错误！不是锁的所有者
```

## 性能考虑

1. **锁粒度**：尽量减小临界区范围
2. **锁竞争**：过多的锁竞争会降低性能
3. **替代方案**：考虑使用原子操作或无锁数据结构

```cpp
// 如果可能，使用原子操作代替互斥锁
#include <atomic>
std::atomic<int> atomic_counter{0};
```

## 总结

`std::mutex` 是多线程编程中的重要工具，但需要谨慎使用：
- 优先使用 RAII 包装器（lock_guard/unique_lock）
- 注意避免死锁和竞态条件
- 根据需求选择合适的互斥锁类型
- 考虑性能影响，尽量减少锁的持有时间

正确使用 `std::mutex` 可以确保多线程程序的数据一致性和线程安全。