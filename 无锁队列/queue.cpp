#include <iostream>
#include <thread>
#include <vector>
#include <boost/lockfree/spsc_queue.hpp>

// 生产者函数
void producer(boost::lockfree::spsc_queue<int>& queue, int numItems) {
    for (int i = 0; i < numItems; ++i) {
        bool success = queue.push(i);
        if (!success) {
            std::cerr << "Producer failed to push item " << i << std::endl;
        }
    }
}

// 消费者函数
void consumer(boost::lockfree::spsc_queue<int>& queue) {
    while (true) {
        int item;
        if (queue.pop(item)) {
            std::cout << "Consumer got item: " << item << std::endl;
        } else {
            // 如果pop返回false，说明队列为空且被设置为非阻塞模式
            break;
        }
    }
}

int main() {
    const int NUM_PRODUCERS = 2;
    const int NUM_CONSUMERS = 3;
    const int NUM_ITEMS_PER_PRODUCER = 10;

    boost::lockfree::spsc_queue<int> queue(1024); // 创建一个大小为1024的单生产者单消费者无锁队列

    // 创建并启动生产者线程
    std::vector<std::thread> producers;
    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(producer, std::ref(queue), NUM_ITEMS_PER_PRODUCER);
    }

    // 创建并启动消费者线程
    std::vector<std::thread> consumers;
    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer, std::ref(queue));
    }

    // 等待所有生产者完成
    for (auto& t : producers) {
        t.join();
    }

    // 告诉消费者队列不再有新数据，通过push失败（假设queue容量已满）
    while (queue.push(-1)) {}

    // 等待所有消费者完成
    for (auto& t : consumers) {
        t.join();
    }

    return 0;
}