/**
 * @file gc_demo.cpp
 * @brief Demonstration of RangeLua's garbage collection system
 * @version 0.1.0
 */

#include <iostream>
#include <vector>
#include <chrono>

#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/gc.hpp>
#include <rangelua/runtime/memory.hpp>

using namespace rangelua;
using namespace rangelua::runtime;

/**
 * @brief Simple test object for GC demonstration
 */
class TestObject {
public:
    explicit TestObject(int value) : value_(value) {
        std::cout << "TestObject(" << value_ << ") created\n";
    }
    
    ~TestObject() {
        std::cout << "TestObject(" << value_ << ") destroyed\n";
    }
    
    int getValue() const { return value_; }
    
private:
    int value_;
};

/**
 * @brief Demonstrate basic Value system with GC pointers
 */
void demonstrateValueSystem() {
    std::cout << "\n=== Value System Demonstration ===\n";
    
    // Create basic values
    Value nil_val;
    Value bool_val(true);
    Value num_val(42.0);
    Value str_val("Hello, RangeLua!");
    
    std::cout << "Nil value: " << nil_val.debug_string() << "\n";
    std::cout << "Boolean value: " << bool_val.debug_string() << "\n";
    std::cout << "Number value: " << num_val.debug_string() << "\n";
    std::cout << "String value: " << str_val.debug_string() << "\n";
    
    // Test type queries
    std::cout << "nil_val.is_nil(): " << nil_val.is_nil() << "\n";
    std::cout << "bool_val.is_boolean(): " << bool_val.is_boolean() << "\n";
    std::cout << "num_val.is_number(): " << num_val.is_number() << "\n";
    std::cout << "str_val.is_string(): " << str_val.is_string() << "\n";
    
    // Test truthiness
    std::cout << "nil_val.is_truthy(): " << nil_val.is_truthy() << "\n";
    std::cout << "bool_val.is_truthy(): " << bool_val.is_truthy() << "\n";
    std::cout << "num_val.is_truthy(): " << num_val.is_truthy() << "\n";
    std::cout << "str_val.is_truthy(): " << str_val.is_truthy() << "\n";
}

/**
 * @brief Demonstrate GC pointer functionality
 */
void demonstrateGCPointers() {
    std::cout << "\n=== GC Pointer Demonstration ===\n";
    
    // Create GC pointers (simplified for demo)
    {
        GCPtr<TestObject> ptr1(new TestObject(1));
        std::cout << "Created ptr1 with value: " << ptr1->getValue() << "\n";
        
        {
            GCPtr<TestObject> ptr2 = ptr1; // Copy
            std::cout << "Copied to ptr2, value: " << ptr2->getValue() << "\n";
            
            GCPtr<TestObject> ptr3(new TestObject(3));
            std::cout << "Created ptr3 with value: " << ptr3->getValue() << "\n";
            
            ptr3 = std::move(ptr1); // Move assignment
            std::cout << "Moved ptr1 to ptr3\n";
            
            if (!ptr1) {
                std::cout << "ptr1 is now null after move\n";
            }
            
            std::cout << "ptr2 still valid: " << ptr2->getValue() << "\n";
            std::cout << "ptr3 now has: " << ptr3->getValue() << "\n";
        } // ptr2 and ptr3 go out of scope
        
        std::cout << "Inner scope ended\n";
    } // ptr1 goes out of scope
    
    std::cout << "Outer scope ended\n";
}

/**
 * @brief Demonstrate value factory functions
 */
void demonstrateValueFactory() {
    std::cout << "\n=== Value Factory Demonstration ===\n";
    
    using namespace value_factory;
    
    // Create values using factory functions
    auto nil_val = nil();
    auto bool_val = boolean(false);
    auto num_val = number(3.14159);
    auto int_val = integer(42);
    auto str_val = string("Factory created string");
    auto str_view_val = string(StringView("String view"));
    auto cstr_val = string("C string");
    
    std::cout << "Factory nil: " << nil_val.debug_string() << "\n";
    std::cout << "Factory boolean: " << bool_val.debug_string() << "\n";
    std::cout << "Factory number: " << num_val.debug_string() << "\n";
    std::cout << "Factory integer: " << int_val.debug_string() << "\n";
    std::cout << "Factory string: " << str_val.debug_string() << "\n";
    std::cout << "Factory string view: " << str_view_val.debug_string() << "\n";
    std::cout << "Factory C string: " << cstr_val.debug_string() << "\n";
}

/**
 * @brief Demonstrate value operations
 */
void demonstrateValueOperations() {
    std::cout << "\n=== Value Operations Demonstration ===\n";
    
    Value a(10.0);
    Value b(5.0);
    Value str1("Hello, ");
    Value str2("World!");
    
    // Arithmetic operations
    try {
        auto sum = a + b;
        auto diff = a - b;
        auto prod = a * b;
        auto quot = a / b;
        
        std::cout << "a + b = " << sum.debug_string() << "\n";
        std::cout << "a - b = " << diff.debug_string() << "\n";
        std::cout << "a * b = " << prod.debug_string() << "\n";
        std::cout << "a / b = " << quot.debug_string() << "\n";
    } catch (const std::exception& e) {
        std::cout << "Arithmetic operation failed: " << e.what() << "\n";
    }
    
    // String concatenation
    try {
        auto concat = str1.concat(str2);
        std::cout << "String concatenation: " << concat.debug_string() << "\n";
    } catch (const std::exception& e) {
        std::cout << "String concatenation failed: " << e.what() << "\n";
    }
    
    // Comparison operations
    std::cout << "a == b: " << (a == b) << "\n";
    std::cout << "a != b: " << (a != b) << "\n";
    std::cout << "a > b: " << (a > b) << "\n";
    std::cout << "a < b: " << (a < b) << "\n";
}

/**
 * @brief Performance test for value creation and destruction
 */
void performanceTest() {
    std::cout << "\n=== Performance Test ===\n";
    
    const size_t iterations = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Test value creation and destruction
    for (size_t i = 0; i < iterations; ++i) {
        Value num_val(static_cast<double>(i));
        Value str_val(std::to_string(i));
        Value bool_val(i % 2 == 0);
        
        // Force some operations
        auto sum = num_val + Value(1.0);
        auto concat = str_val.concat(Value("_suffix"));
        bool truthy = bool_val.is_truthy();
        
        // Prevent optimization
        (void)sum;
        (void)concat;
        (void)truthy;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Created and operated on " << iterations << " values in " 
              << duration.count() << " microseconds\n";
    std::cout << "Average time per value: " 
              << static_cast<double>(duration.count()) / iterations << " microseconds\n";
}

/**
 * @brief Main demonstration function
 */
int main() {
    std::cout << "RangeLua Garbage Collection System Demo\n";
    std::cout << "======================================\n";
    
    try {
        demonstrateValueSystem();
        demonstrateGCPointers();
        demonstrateValueFactory();
        demonstrateValueOperations();
        performanceTest();
        
        std::cout << "\n=== Demo completed successfully ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Demo failed with unknown exception\n";
        return 1;
    }
    
    return 0;
}
