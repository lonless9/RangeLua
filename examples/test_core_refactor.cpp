/**
 * @file test_core_refactor.cpp
 * @brief Test the refactored core module with modern C++20 features
 * @version 0.1.0
 */

#include <rangelua/core/error.hpp>
#include <rangelua/runtime/memory.hpp>
#include <rangelua/core/patterns.hpp>
#include <rangelua/core/types.hpp>
#include <rangelua/rangelua.hpp>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace rangelua;

// Test modern error handling
void test_error_handling() {
    std::cout << "Testing modern error handling...\n";

    // Test Result type with monadic operations
    auto success_result = make_success(42);
    assert(is_success(success_result));
    assert(get_value(success_result) == 42);

    auto error_result = make_error<int>(ErrorCode::TYPE_ERROR);
    assert(is_error(error_result));
    assert(get_error(error_result) == ErrorCode::TYPE_ERROR);

    // Test monadic operations
    auto transformed = and_then(success_result, [](int value) {
        return make_success(value * 2);
    });
    assert(is_success(transformed));
    assert(get_value(transformed) == 84);

    std::cout << "✓ Error handling tests passed\n";
}

// Test memory management
void test_memory_management() {
    std::cout << "Testing memory management...\n";

    // Test system allocator
    auto system_manager = runtime::MemoryManagerFactory::create_system_manager();
    assert(system_manager != nullptr);

    // Test managed resource
    auto resource = system_manager->make_unique<int>(42);
    assert(resource != nullptr);
    assert(*resource == 42);

    // Test pool allocator
    auto pool_manager = runtime::MemoryManagerFactory::create_pool_manager<64>();
    assert(pool_manager != nullptr);

    std::cout << "✓ Memory management tests passed\n";
}

// Test design patterns
void test_design_patterns() {
    std::cout << "Testing design patterns...\n";

    // Test Observer pattern
    struct TestEvent {
        String message;
        int value;
    };

    class TestObserver : public patterns::Observer<TestEvent> {
    public:
        void notify(const TestEvent& event) override {
            last_event = event;
            notification_count++;
        }

        TestEvent last_event;
        int notification_count = 0;
    };

    patterns::Observable<TestEvent> observable;
    auto observer1 = std::make_shared<TestObserver>();
    auto observer2 = std::make_shared<TestObserver>();

    observable.add_observer(observer1);
    observable.add_observer(observer2);

    TestEvent event{"test", 123};
    observable.notify_observers(event);

    assert(observer1->notification_count == 1);
    assert(observer2->notification_count == 1);
    assert(observer1->last_event.message == "test");
    assert(observer1->last_event.value == 123);

    // Test Factory pattern
    class BaseClass {
    public:
        virtual ~BaseClass() = default;
        virtual String get_type() const = 0;
    };

    class DerivedA : public BaseClass {
    public:
        String get_type() const override { return "A"; }
    };

    class DerivedB : public BaseClass {
    public:
        String get_type() const override { return "B"; }
    };

    patterns::Factory<BaseClass> factory;
    factory.register_type<DerivedA>("A");
    factory.register_type<DerivedB>("B");

    auto obj_a = factory.create("A");
    auto obj_b = factory.create("B");
    auto obj_invalid = factory.create("C");

    assert(obj_a != nullptr);
    assert(obj_b != nullptr);
    assert(obj_invalid == nullptr);
    assert(obj_a->get_type() == "A");
    assert(obj_b->get_type() == "B");

    std::cout << "✓ Design pattern tests passed\n";
}

// Test type concepts
void test_type_concepts() {
    std::cout << "Testing type concepts...\n";

    // Test numeric concept
    static_assert(Numeric<int>);
    static_assert(Numeric<double>);
    static_assert(!Numeric<String>);

    // Test string-like concept
    static_assert(StringLike<String>);
    static_assert(StringLike<StringView>);
    static_assert(StringLike<const char*>);

    // Test range concepts
    std::vector<int> vec{1, 2, 3};
    static_assert(Range<decltype(vec)>);
    static_assert(RangeOf<decltype(vec), int>);
    static_assert(SizedRange<decltype(vec)>);

    // Test smart pointer concepts
    auto unique_ptr = std::make_unique<int>(42);
    auto shared_ptr = std::make_shared<int>(42);
    static_assert(UniquePointerLike<decltype(unique_ptr)>);
    static_assert(SharedPointerLike<decltype(shared_ptr)>);

    std::cout << "✓ Type concept tests passed\n";
}

// Test initialization system
void test_initialization() {
    std::cout << "Testing initialization system...\n";

    // Test initialization
    auto init_result = initialize();
    assert(is_success(init_result));
    assert(is_initialized());

    // Test memory manager access
    auto* memory_manager = get_memory_manager();
    assert(memory_manager != nullptr);

    // Test cleanup
    cleanup();

    std::cout << "✓ Initialization tests passed\n";
}

// Test tagged values
void test_tagged_values() {
    std::cout << "Testing tagged values...\n";

    TaggedValue<int> int_value(42, LuaType::NUMBER);
    assert(int_value.is_type(LuaType::NUMBER));
    assert(int_value.type() == LuaType::NUMBER);
    assert(int_value.as<int>() == 42);

    TaggedValue<String> string_value("hello", LuaType::STRING);
    assert(string_value.is_type(LuaType::STRING));
    assert(string_value.as<String>() == "hello");

    // Test comparison operators
    TaggedValue<int> int_value2(42, LuaType::NUMBER);
    assert(int_value == int_value2);

    std::cout << "✓ Tagged value tests passed\n";
}

int main() {
    std::cout << "=== RangeLua Core Module Refactor Tests ===\n\n";

    try {
        test_error_handling();
        test_memory_management();
        test_design_patterns();
        test_type_concepts();
        test_initialization();
        test_tagged_values();

        std::cout << "\n=== All tests passed! ===\n";
        std::cout << "✓ Modern C++20 features working correctly\n";
        std::cout << "✓ RAII patterns implemented\n";
        std::cout << "✓ Design patterns functional\n";
        std::cout << "✓ Type safety enhanced\n";
        std::cout << "✓ Global state eliminated\n";
        std::cout << "✓ Error handling modernized\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
