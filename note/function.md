# 深入解析 std::function

## 什么是 std::function？

`std::function` 是 C++11 引入的函数包装器，它是一个通用的、类型安全的函数对象容器，可以存储、复制和调用任何可调用对象（函数、lambda表达式、函数对象、绑定表达式等）。

## 基本概念

### 1. **基本语法**
```cpp
#include <functional>
#include <iostream>

// 函数签名模板：std::function<返回类型(参数类型列表)>
std::function<int(int, int)> func;  // 可以存储任何返回int，接受两个int的函数

// 存储普通函数
int add(int a, int b) { return a + b; }
func = add;
std::cout << func(2, 3) << std::endl;  // 输出: 5

// 存储lambda表达式
func = [](int a, int b) { return a * b; };
std::cout << func(2, 3) << std::endl;  // 输出: 6

// 存储函数对象（仿函数）
struct Multiply {
    int operator()(int a, int b) const { return a * b; }
};
Multiply mult;
func = mult;
std::cout << func(2, 3) << std::endl;  // 输出: 6
```

### 2. **存储不同类型可调用对象**

```cpp
#include <functional>
#include <string>
#include <vector>
#include <algorithm>

// 1. 普通函数
void print_int(int x) { std::cout << x << " "; }

// 2. 函数对象（带状态）
class Printer {
    std::string prefix;
public:
    Printer(const std::string& p) : prefix(p) {}
    void operator()(int x) const {
        std::cout << prefix << x << " ";
    }
};

// 3. Lambda表达式
auto square = [](int x) { return x * x; };

// 4. 成员函数
class Calculator {
public:
    int add(int a, int b) { return a + b; }
    static int multiply(int a, int b) { return a * b; }
};

// 5. 使用std::bind
using namespace std::placeholders;  // for _1, _2, _3...

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5};
    
    // 存储普通函数指针
    std::function<void(int)> f1 = print_int;
    
    // 存储函数对象
    Printer printer("Value: ");
    std::function<void(int)> f2 = printer;
    
    // 存储lambda
    std::function<int(int)> f3 = square;
    
    // 存储静态成员函数
    std::function<int(int, int)> f4 = Calculator::multiply;
    
    // 存储绑定表达式
    Calculator calc;
    std::function<int(int, int)> f5 = std::bind(&Calculator::add, &calc, _1, _2);
    
    // 存储带额外参数的绑定
    std::function<int(int)> f6 = std::bind(&Calculator::add, &calc, _1, 10);
    
    // 使用示例
    std::for_each(nums.begin(), nums.end(), f1);  // 输出: 1 2 3 4 5
    std::cout << std::endl;
    
    std::for_each(nums.begin(), nums.end(), f2);  // 输出: Value: 1 Value: 2 ...
    std::cout << std::endl;
    
    for (int n : nums) {
        std::cout << f3(n) << " ";  // 输出: 1 4 9 16 25
    }
    std::cout << std::endl;
    
    std::cout << "Multiply: " << f4(3, 4) << std::endl;  // 输出: 12
    std::cout << "Add: " << f5(3, 4) << std::endl;       // 输出: 7
    std::cout << "Add 10: " << f6(5) << std::endl;       // 输出: 15
    
    return 0;
}
```

## 核心特性

### 1. **类型擦除机制**

`std::function` 使用类型擦除技术，允许存储不同类型的可调用对象：

```cpp
#include <functional>
#include <iostream>
#include <memory>

// std::function内部大致实现原理
template<typename Signature>
class my_function;  // 简化版本

template<typename R, typename... Args>
class my_function<R(Args...)> {
private:
    // 抽象基类
    struct callable_base {
        virtual ~callable_base() = default;
        virtual R invoke(Args... args) = 0;
        virtual std::unique_ptr<callable_base> clone() const = 0;
    };
    
    // 具体存储类模板
    template<typename F>
    struct callable_impl : callable_base {
        F f;
        callable_impl(F&& func) : f(std::forward<F>(func)) {}
        
        R invoke(Args... args) override {
            return f(std::forward<Args>(args)...);
        }
        
        std::unique_ptr<callable_base> clone() const override {
            return std::make_unique<callable_impl>(f);
        }
    };
    
    std::unique_ptr<callable_base> impl;
    
public:
    // 构造函数模板
    template<typename F>
    my_function(F&& f) : 
        impl(std::make_unique<callable_impl<std::decay_t<F>>>(std::forward<F>(f))) {}
    
    // 调用操作符
    R operator()(Args... args) const {
        return impl->invoke(std::forward<Args>(args)...);
    }
    
    // 检查是否为空
    explicit operator bool() const { return static_cast<bool>(impl); }
};

// 使用示例
int main() {
    my_function<int(int, int)> my_func = [](int a, int b) { return a + b; };
    std::cout << my_func(10, 20) << std::endl;  // 输出: 30
    return 0;
}
```

### 2. **空状态和bool转换**

```cpp
#include <functional>
#include <iostream>

std::function<void()> task;

// 检查function是否为空
if (!task) {  // 或者 if (task == nullptr)
    std::cout << "task is empty\n";
}

// 赋值
task = []() { std::cout << "Hello from task!\n"; };

// 再次检查
if (task) {
    task();  // 输出: Hello from task!
}

// 重置为空
task = nullptr;
// 或者 task = std::function<void()>();  // 默认构造
// 或者 task = {};  // 空花括号初始化

// 尝试调用空的function会抛出异常
try {
    task();  // 抛出 std::bad_function_call
} catch (const std::bad_function_call& e) {
    std::cout << "Caught exception: " << e.what() << std::endl;
}

// 安全的调用方式
if (task) {
    task();
} else {
    std::cout << "No task to execute\n";
}
```

### 3. **复制和移动语义**

```cpp
#include <functional>
#include <iostream>
#include <string>

class CallCounter {
    int count = 0;
    std::string name;
public:
    CallCounter(const std::string& n) : name(n) {}
    
    void operator()() {
        ++count;
        std::cout << name << " called " << count << " times\n";
    }
    
    // 禁用复制
    CallCounter(const CallCounter&) = delete;
    CallCounter& operator=(const CallCounter&) = delete;
    
    // 允许移动
    CallCounter(CallCounter&&) = default;
    CallCounter& operator=(CallCounter&&) = default;
};

int main() {
    // std::function存储可移动对象
    std::function<void()> func1;
    {
        CallCounter counter("Lambda");
        func1 = std::move(counter);  // 移动构造
    }
    // counter已销毁，但func1仍然有效
    func1();  // 输出: Lambda called 1 times
    func1();  // 输出: Lambda called 2 times
    
    // 复制std::function
    auto func2 = func1;  // 复制func1，内部会克隆可调用对象
    func2();  // 输出: Lambda called 1 times（新的计数器）
    func1();  // 输出: Lambda called 3 times（原始的计数器）
    
    // 移动std::function
    auto func3 = std::move(func1);  // 移动func1到func3
    // func1现在为空
    if (!func1) {
        std::cout << "func1 is now empty\n";
    }
    func3();  // 输出: Lambda called 4 times
    
    return 0;
}
```

## 实际应用场景

### 场景1：回调函数系统

```cpp
#include <functional>
#include <vector>
#include <iostream>
#include <algorithm>

class EventDispatcher {
private:
    std::vector<std::function<void(int)>> listeners;
    
public:
    // 注册监听器
    void add_listener(std::function<void(int)> callback) {
        listeners.push_back(std::move(callback));
    }
    
    // 触发事件
    void dispatch_event(int event_id) {
        for (auto& listener : listeners) {
            if (listener) {  // 检查是否有效
                listener(event_id);
            }
        }
    }
    
    // 移除所有监听器
    void clear() {
        listeners.clear();
    }
};

// 使用示例
void global_handler(int id) {
    std::cout << "Global handler: Event " << id << "\n";
}

class EventHandler {
    std::string name;
public:
    EventHandler(const std::string& n) : name(n) {}
    
    void handle(int id) {
        std::cout << name << " handler: Event " << id << "\n";
    }
};

int main() {
    EventDispatcher dispatcher;
    
    // 注册普通函数
    dispatcher.add_listener(global_handler);
    
    // 注册lambda
    dispatcher.add_listener([](int id) {
        std::cout << "Lambda: Processing event " << id << "\n";
    });
    
    // 注册成员函数
    EventHandler handler1("Object1");
    EventHandler handler2("Object2");
    
    using std::placeholders::_1;
    dispatcher.add_listener(std::bind(&EventHandler::handle, &handler1, _1));
    dispatcher.add_listener(std::bind(&EventHandler::handle, &handler2, _1));
    
    // 触发事件
    dispatcher.dispatch_event(100);
    dispatcher.dispatch_event(200);
    
    return 0;
}
```

### 场景2：命令模式实现

```cpp
#include <functional>
#include <vector>
#include <memory>
#include <iostream>
#include <stack>

// 命令接口
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

// 使用std::function的具体命令
class FunctionCommand : public Command {
    std::function<void()> execute_func;
    std::function<void()> undo_func;
    
public:
    FunctionCommand(std::function<void()> exec, std::function<void()> undo = nullptr)
        : execute_func(std::move(exec)), undo_func(std::move(undo)) {}
    
    void execute() override {
        if (execute_func) {
            execute_func();
        }
    }
    
    void undo() override {
        if (undo_func) {
            undo_func();
        }
    }
};

// 带参数的命令模板
template<typename... Args>
class ParamCommand {
    std::function<void(Args...)> command;
    std::function<void(Args...)> undo_command;
    
public:
    ParamCommand(std::function<void(Args...)> cmd, 
                 std::function<void(Args...)> undo = nullptr)
        : command(std::move(cmd)), undo_command(std::move(undo)) {}
    
    void execute(Args... args) {
        if (command) {
            command(std::forward<Args>(args)...);
        }
    }
    
    void undo(Args... args) {
        if (undo_command) {
            undo_command(std::forward<Args>(args)...);
        }
    }
};

// 使用示例：文本编辑器
class TextEditor {
    std::string text;
    std::stack<std::function<void()>> undo_stack;
    
public:
    void insert_text(size_t pos, const std::string& str) {
        // 保存当前状态用于undo
        std::string old_text = text;
        
        auto execute = [this, pos, str]() {
            text.insert(pos, str);
            std::cout << "Inserted: " << str << "\n";
            std::cout << "Text: " << text << "\n";
        };
        
        auto undo = [this, old_text]() {
            text = old_text;
            std::cout << "Undo insert\n";
            std::cout << "Text: " << text << "\n";
        };
        
        execute();
        undo_stack.push(undo);
    }
    
    void delete_text(size_t pos, size_t len) {
        std::string deleted = text.substr(pos, len);
        std::string old_text = text;
        
        auto execute = [this, pos, len]() {
            text.erase(pos, len);
            std::cout << "Deleted " << len << " chars\n";
            std::cout << "Text: " << text << "\n";
        };
        
        auto undo = [this, pos, deleted, old_text]() {
            text = old_text;
            std::cout << "Undo delete, restored: " << deleted << "\n";
            std::cout << "Text: " << text << "\n";
        };
        
        execute();
        undo_stack.push(undo);
    }
    
    void undo() {
        if (!undo_stack.empty()) {
            auto undo_func = undo_stack.top();
            undo_stack.pop();
            if (undo_func) {
                undo_func();
            }
        }
    }
    
    const std::string& get_text() const { return text; }
};

int main() {
    TextEditor editor;
    
    editor.insert_text(0, "Hello");
    editor.insert_text(5, " World");
    editor.delete_text(5, 6);
    editor.undo();  // 恢复删除
    editor.undo();  // 恢复插入" World"
    
    return 0;
}
```

### 场景3：策略模式

```cpp
#include <functional>
#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>

// 排序策略
class SortStrategy {
public:
    virtual ~SortStrategy() = default;
    virtual void sort(std::vector<int>& data) = 0;
};

// 使用std::function的策略实现
class FunctionSortStrategy : public SortStrategy {
    std::function<void(std::vector<int>&)> sort_func;
    
public:
    FunctionSortStrategy(std::function<void(std::vector<int>&)> func)
        : sort_func(std::move(func)) {}
    
    void sort(std::vector<int>& data) override {
        if (sort_func) {
            sort_func(data);
        }
    }
};

// 上下文类
class DataProcessor {
    std::unique_ptr<SortStrategy> strategy;
    std::vector<int> data;
    
public:
    DataProcessor(std::vector<int> initial_data) 
        : data(std::move(initial_data)) {}
    
    void set_strategy(std::unique_ptr<SortStrategy> new_strategy) {
        strategy = std::move(new_strategy);
    }
    
    void process() {
        if (strategy) {
            strategy->sort(data);
            print_data();
        }
    }
    
    void print_data() const {
        std::cout << "Data: ";
        for (int val : data) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
};

// 各种排序算法作为函数
void bubble_sort(std::vector<int>& arr) {
    int n = arr.size();
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (arr[j] > arr[j+1]) {
                std::swap(arr[j], arr[j+1]);
            }
        }
    }
    std::cout << "Used bubble sort\n";
}

void quick_sort_impl(std::vector<int>& arr, int low, int high) {
    if (low < high) {
        int pivot = arr[high];
        int i = low - 1;
        
        for (int j = low; j <= high - 1; j++) {
            if (arr[j] < pivot) {
                i++;
                std::swap(arr[i], arr[j]);
            }
        }
        std::swap(arr[i + 1], arr[high]);
        int pi = i + 1;
        
        quick_sort_impl(arr, low, pi - 1);
        quick_sort_impl(arr, pi + 1, high);
    }
}

void quick_sort(std::vector<int>& arr) {
    quick_sort_impl(arr, 0, arr.size() - 1);
    std::cout << "Used quick sort\n";
}

// lambda表达式作为策略
auto lambda_sort = [](std::vector<int>& arr) {
    std::sort(arr.begin(), arr.end(), std::greater<int>());
    std::cout << "Used lambda (descending) sort\n";
};

int main() {
    std::vector<int> data = {64, 34, 25, 12, 22, 11, 90};
    DataProcessor processor(data);
    
    // 使用不同的排序策略
    processor.set_strategy(
        std::make_unique<FunctionSortStrategy>(bubble_sort));
    processor.process();
    
    processor.set_strategy(
        std::make_unique<FunctionSortStrategy>(quick_sort));
    processor.process();
    
    processor.set_strategy(
        std::make_unique<FunctionSortStrategy>(lambda_sort));
    processor.process();
    
    // 直接在lambda中定义策略
    processor.set_strategy(
        std::make_unique<FunctionSortStrategy>([](std::vector<int>& arr) {
            std::sort(arr.begin(), arr.end());
            std::cout << "Used inline lambda sort\n";
        }));
    processor.process();
    
    return 0;
}
```

### 场景4：异步任务和线程池

```cpp
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <iostream>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
    
public:
    ThreadPool(size_t threads) {
        for (size_t i = 0; i < threads; ++i) {
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
                    
                    task();
                }
            });
        }
    }
    
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type> {
        
        using return_type = typename std::result_of<F(Args...)>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> res = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            
            tasks.emplace([task]() { (*task)(); });
        }
        
        condition.notify_one();
        return res;
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        
        condition.notify_all();
        for (std::thread& worker : workers) {
            worker.join();
        }
    }
};

// 使用示例
int main() {
    ThreadPool pool(4);
    std::vector<std::future<int>> results;
    
    // 提交多个任务
    for (int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue([i] {
                std::cout << "Task " << i << " started\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "Task " << i << " finished\n";
                return i * i;
            })
        );
    }
    
    // 获取结果
    for (auto&& result : results) {
        std::cout << "Result: " << result.get() << std::endl;
    }
    
    return 0;
}
```

## 高级用法和技巧

### 1. **递归std::function**

```cpp
#include <functional>
#include <iostream>

// 递归lambda需要std::function
std::function<int(int)> factorial = [&factorial](int n) -> int {
    return n <= 1 ? 1 : n * factorial(n - 1);
};

// 或者使用Y组合子
template<typename Func>
struct YCombinator {
    Func func;
    
    template<typename... Args>
    decltype(auto) operator()(Args&&... args) {
        return func(*this, std::forward<Args>(args)...);
    }
};

template<typename Func>
YCombinator<std::decay_t<Func>> make_y_combinator(Func&& func) {
    return {std::forward<Func>(func)};
}

int main() {
    // 方法1：使用std::function的递归
    std::cout << "Factorial of 5: " << factorial(5) << std::endl;
    
    // 方法2：使用Y组合子（不需要std::function）
    auto fib = make_y_combinator(
        [](auto&& self, int n) -> int {
            if (n <= 1) return n;
            return self(n - 1) + self(n - 2);
        }
    );
    
    std::cout << "Fibonacci of 10: " << fib(10) << std::endl;
    
    return 0;
}
```

### 2. **性能优化：使用模板参数避免类型擦除**

```cpp
#include <functional>
#include <iostream>
#include <chrono>

// 通用但较慢（使用类型擦除）
void process_slow(const std::function<void(int)>& func, int n) {
    for (int i = 0; i < n; ++i) {
        func(i);
    }
}

// 更快（使用模板，无类型擦除）
template<typename Func>
void process_fast(Func&& func, int n) {
    for (int i = 0; i < n; ++i) {
        func(i);
    }
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // 使用std::function
    std::function<void(int)> func = [](int x) { volatile int y = x * x; (void)y; };
    process_slow(func, 10000000);
    
    auto mid = std::chrono::high_resolution_clock::now();
    
    // 使用模板
    auto lambda = [](int x) { volatile int y = x * x; (void)y; };
    process_fast(lambda, 10000000);
    
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid);
    
    std::cout << "std::function time: " << duration1.count() << "ms\n";
    std::cout << "Template time: " << duration2.count() << "ms\n";
    
    return 0;
}
```

### 3. **函数组合和管道**

```cpp
#include <functional>
#include <iostream>
#include <vector>
#include <algorithm>

// 函数组合：f(g(x))
template<typename F, typename G>
auto compose(F&& f, G&& g) {
    return [f = std::forward<F>(f), g = std::forward<G>(g)](auto&&... args) {
        return f(g(std::forward<decltype(args)>(args)...));
    };
}

// 管道操作：x | f | g
template<typename T, typename F>
auto operator|(T&& value, F&& func) {
    return func(std::forward<T>(value));
}

int main() {
    // 函数组合示例
    auto add_one = [](int x) { return x + 1; };
    auto square = [](int x) { return x * x; };
    auto double_it = [](int x) { return x * 2; };
    
    // 组合：square(add_one(x))
    auto add_then_square = compose(square, add_one);
    std::cout << "add_then_square(3) = " << add_then_square(3) << std::endl;  // 16
    
    // 组合多个函数
    auto complex_func = compose(double_it, compose(square, add_one));
    std::cout << "complex_func(3) = " << complex_func(3) << std::endl;  // 32
    
    // 管道操作
    auto result = 3 | add_one | square | double_it;
    std::cout << "3 | add_one | square | double_it = " << result << std::endl;  // 32
    
    // 存储组合函数
    std::function<int(int)> stored_func = add_then_square;
    std::cout << "stored_func(4) = " << stored_func(4) << std::endl;  // 25
    
    // 在算法中使用
    std::vector<int> nums = {1, 2, 3, 4, 5};
    std::vector<int> transformed;
    
    std::transform(nums.begin(), nums.end(), 
                   std::back_inserter(transformed),
                   add_then_square);
    
    std::cout << "Transformed: ";
    for (int n : transformed) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

## 注意事项和限制

### 1. **性能开销**

```cpp
// std::function有性能开销：
// 1. 类型擦除（虚函数调用）
// 2. 堆内存分配（可能）
// 3. 额外的间接调用

// 如果性能关键，考虑替代方案：
template<typename Func>
void fast_callback(Func&& func) {  // 模板参数，无开销
    func();
}

void slow_callback(const std::function<void()>& func) {  // 有开销
    func();
}
```

### 2. **不支持默认参数**

```cpp
void func_with_default(int x, int y = 10) {
    std::cout << x + y << std::endl;
}

int main() {
    std::function<void(int, int)> f1 = func_with_default;
    f1(5, 5);  // OK: 10
    
    // 不能直接使用默认参数
    // std::function<void(int)> f2 = func_with_default;  // 错误！
    
    // 需要使用bind或lambda
    using namespace std::placeholders;
    std::function<void(int)> f3 = std::bind(func_with_default, _1, 10);
    f3(5);  // OK: 15
    
    std::function<void(int)> f4 = [](int x) { func_with_default(x, 10); };
    f4(5);  // OK: 15
    
    return 0;
}
```

### 3. **存储引用和成员函数**

```cpp
class MyClass {
public:
    int value = 0;
    
    void increment() { ++value; }
    int get_value() const { return value; }
};

int main() {
    MyClass obj;
    
    // 存储成员函数需要绑定对象
    std::function<void()> increment_func = std::bind(&MyClass::increment, &obj);
    std::function<int()> get_value_func = std::bind(&MyClass::get_value, &obj);
    
    increment_func();
    std::cout << "Value: " << get_value_func() << std::endl;  // 1
    
    // 存储引用需要ref包装
    int x = 10;
    std::function<void(int)> modify_ref = [&x](int val) { x = val; };
    modify_ref(20);
    std::cout << "x = " << x << std::endl;  // 20
    
    // 或者使用std::ref
    std::function<void(int)> modify_ref2 = std::bind([](int& ref, int val) { 
        ref = val; 
    }, std::ref(x), std::placeholders::_1);
    modify_ref2(30);
    std::cout << "x = " << x << std::endl;  // 30
    
    return 0;
}
```

## 最佳实践总结

1. **使用场景**：
   - 回调系统
   - 策略模式
   - 命令模式
   - 事件处理
   - 需要存储不同类型可调用对象时

2. **性能考虑**：
   - 在性能关键路径避免使用
   - 考虑使用模板参数替代
   - 小函数可考虑内联

3. **安全性**：
   - 总是检查是否为空再调用
   - 使用 `std::invoke`（C++17）进行更安全的调用

4. **替代方案**：
   - C++17：`std::invoke`，`std::apply`
   - 模板编程：类型安全的回调
   - 函数指针：简单场景
   - lambda表达式：就地使用

5. **现代C++扩展**：
   - C++20：`std::function_ref`（轻量级，不可复制）
   - 概念（Concepts）：更好的类型约束

`std::function` 是C++中极其有用的工具，它提供了极大的灵活性，但也需要注意其性能开销和正确使用方式。