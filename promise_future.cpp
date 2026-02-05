#include <iostream>
#include <thread>
#include <future>
#include <chrono>

int main() {
    // 创建承诺
    std::promise<int> result_promise;
    
    // 获取关联的future
    std::future<int> result_future = result_promise.get_future();
    
    // 创建生产者线程
    std::thread producer([&result_promise]() {
        std::cout << "生产者: 开始计算...\n";
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 模拟耗时计算
        
        int result = 42; // 计算结果
        
        std::cout << "生产者: 计算完成，设置结果: " << result << "\n";
        result_promise.set_value(result); // 履行承诺
        std::cout << "生产者: 承诺已履行，退出\n";
    });
    
    // 消费者（主线程）等待结果
    std::cout << "消费者: 等待结果...\n";
    
    // get()会阻塞，直到promise设置值
    int final_result = result_future.get();
    
    std::cout << "消费者: 收到结果 = " << final_result << "\n";
    
    producer.join();
    return 0;
}
