#include <iostream>
#include <cstdlib>
#include <list>
#include <ctime>
#include <chrono>

// 内存块结构体，包含数据和前后指针
struct MemoryBlock {
    bool is_used; // 标记是否已分配
    char data[1]; // 数据区域（实际大小由内存池决定）

    MemoryBlock* next = nullptr;
    MemoryBlock* prev = nullptr;

    size_t block_size;
};

class FixedSizeMemoryPool {
private:
    std::list<MemoryBlock*> free_list; // 空闲块列表

public:
    // 构造函数，初始化内存池并分配指定数量的固定大小内存块
    FixedSizeMemoryPool(size_t block_count, size_t block_data_size) {
        for (size_t i = 0; i < block_count; ++i) {
            auto block = static_cast<MemoryBlock*>(std::malloc(sizeof(MemoryBlock) + block_data_size));
            block->is_used = false;
            block->block_size = sizeof(MemoryBlock) + block_data_size;

            if (i == 0) {
                block->prev = nullptr;
            } else {
                auto previous_block = free_list.back();
                previous_block->next = block;
                block->prev = previous_block;
            }

            free_list.push_back(block);
        }
    }

    ~FixedSizeMemoryPool() {
        while (!free_list.empty()) {
            auto block = free_list.front();
            free_list.pop_front();
            std::free(block);
        }
    }

    // 分配一个内存块，并返回指向数据区域的指针
    void* allocate(size_t block_data_size) {
        if (free_list.empty()) {
            throw std::bad_alloc(); // 如果没有可用内存块，则抛出异常
        }

        auto block = free_list.front();
        free_list.pop_front();

        // 检查内存块大小是否与请求的一致
        if (block->block_size - sizeof(MemoryBlock) != block_data_size) {
            throw std::runtime_error("No suitable block available");
        }

        block->is_used = true;

        return block->data;
    }

    // 释放一个内存块
    void deallocate(void* ptr_to_data, size_t block_data_size) {
        if (!ptr_to_data) {
            return; // 防止非法释放nullptr
        }

        // 从空闲链表中找到对应的数据区域指针
        for (auto it = free_list.begin(); it != free_list.end(); ++it) {
            auto block = *it;
            if (block->data == ptr_to_data && block->block_size - sizeof(MemoryBlock) == block_data_size) {
                block->is_used = false;

                // 将释放的内存块插入到空闲列表尾部
                if (it != free_list.begin()) {
                    auto prev_it = std::prev(it);
                    auto last_block = *prev_it;
                    last_block->next = block;
                    block->prev = last_block;
                }

                free_list.splice(free_list.end(), free_list, it);
                return;
            }
        }

        throw std::invalid_argument("Trying to deallocate an unallocated block or unmatched block size"); // 如果未找到匹配的块，则抛出异常
    }
};

long long int getCurrentMillis() {
    using namespace std::chrono;
    auto now = system_clock::now();
    return duration_cast<milliseconds>(now.time_since_epoch()).count();
}

int main()
{
	std::cout << "Test Memory Pool" << std::endl;
	
	FixedSizeMemoryPool gMemoryPool(1024*1024, 4);
	
	void *chartime = nullptr;

	std::cout << "Current Unix timestamp in milliseconds: " << getCurrentMillis() << std::endl;
	for(int i = 0; i < 10000; i++) {
		chartime = gMemoryPool.allocate(4);
		gMemoryPool.deallocate(chartime, 4);
	}

	std::cout << "Current Unix timestamp in milliseconds: " << getCurrentMillis() << std::endl;
	
	int *chartime1 = nullptr;
	std::cout << "Current Unix timestamp in milliseconds: " << getCurrentMillis() << std::endl;
	for(int i = 0; i < 10000; i++) {
		chartime1 = new int;
		delete chartime1;
	}

	std::cout << "Current Unix timestamp in milliseconds: " << getCurrentMillis() << std::endl;
	
	
	
	return 0;
}
