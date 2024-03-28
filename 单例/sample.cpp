#include <mutex>

//采用线程安全的双检查锁定（Double-Checked Locking）优化方法

// 假设这是我们的单例类
class Singleton {
public:
    // 禁止用户通过默认构造函数创建对象
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    // 提供一个静态访问点来获取单例实例
    static Singleton& getInstance() {
        // 第一次检查实例是否已创建
        if (nullptr == instance) {
            // 由于创建实例可能是一个耗时的操作，并且我们只希望在必要时执行，
            // 所以在这里引入互斥锁保证线程安全
            std::lock_guard<std::mutex> lock(mtx);

            // 在加锁后再次检查实例是否已创建，因为有可能在第一个if判断和这里之间，
            // 另一个线程已经创建了实例
            if (nullptr == instance) {
                // 创建并初始化单例实例
                instance = new Singleton();
            }
        }

        return *instance;
    }

    // 示例方法
    void someMethod() {
        // 单例类的具体业务逻辑...
    }

private:
    // 私有化构造函数，确保只能通过getInstance()获得单例实例
    Singleton() {}

    // 静态成员变量用于存储单例实例
    static Singleton* instance;

    // 互斥锁用于保护对单例实例的并发访问
    static std::mutex mtx;
};

// 初始化静态成员变量
Singleton* Singleton::instance = nullptr;
std::mutex Singleton::mtx;

// 使用示例：
int main() {
    Singleton& s1 = Singleton::getInstance();
    s1.someMethod();

    Singleton& s2 = Singleton::getInstance();
    // 这里s1和s2实际上引用的是同一个单例实例

    return 0;
}