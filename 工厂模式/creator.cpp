// 抽象产品接口
class Product {
public:
    // 纯虚函数声明，由具体产品类实现
    virtual ~Product() {}
    virtual void use() const = 0;
};

// 具体产品类1：ConcreteProductA
class ConcreteProductA : public Product {
public:
    void use() const override {
        std::cout << "Using product A." << std::endl;
    }
};

// 具体产品类2：ConcreteProductB
class ConcreteProductB : public Product {
public:
    void use() const override {
        std::cout << "Using product B." << std::endl;
    }
};

// 工厂类：Creator
class Creator {
public:
    // 创建产品的静态方法，根据参数决定创建哪种具体产品
    static std::unique_ptr<Product> createProduct(const std::string& type) {
        if (type == "A") {
            return std::make_unique<ConcreteProductA>();
        } else if (type == "B") {
            return std::make_unique<ConcreteProductB>();
        } else {
            throw std::invalid_argument("Invalid product type.");
        }
    }
};

// 使用示例
int main() {
    try {
        auto productA = Creator::createProduct("A");
        productA->use();

        auto productB = Creator::createProduct("B");
        productB->use();

        // 错误的产品类型尝试
        auto invalidProduct = Creator::createProduct("C"); // 这将抛出异常
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}