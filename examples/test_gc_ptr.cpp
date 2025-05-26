/**
 * @file test_gc_ptr.cpp
 * @brief Comprehensive test suite for GCPtr and WeakGCPtr implementation
 * @version 0.1.0
 * 
 * This file contains unit tests that demonstrate:
 * - Basic GCPtr functionality
 * - Weak reference behavior
 * - Cycle detection and breaking
 * - Thread safety
 * - Memory leak prevention
 */

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>

// We need to include the core types first
#include <rangelua/core/types.hpp>

// Mock GCObject implementation for testing
namespace rangelua::runtime {

// Forward declaration from gc_ptr.hpp
struct GCControlBlock;

class GCObject {
public:
    explicit GCObject(LuaType type) noexcept : header_(type), control_(new GCControlBlock()) {}
    virtual ~GCObject() { delete control_; }

    // Non-copyable, non-movable
    GCObject(const GCObject&) = delete;
    GCObject& operator=(const GCObject&) = delete;
    GCObject(GCObject&&) = delete;
    GCObject& operator=(GCObject&&) = delete;

    // GC interface
    GCHeader& gcHeader() noexcept { return header_; }
    const GCHeader& gcHeader() const noexcept { return header_; }
    
    void addRef() noexcept { control_->addStrongRef(); }
    void removeRef() noexcept { 
        if (control_->removeStrongRef()) {
            delete this;
        }
    }
    std::uint32_t refCount() const noexcept { return control_->getStrongCount(); }
    
    GCControlBlock& gcControlBlock() noexcept { return *control_; }
    const GCControlBlock& gcControlBlock() const noexcept { return *control_; }

    // Traversal for cycle detection
    virtual void traverse(std::function<void(GCObject*)> visitor) = 0;

private:
    GCHeader header_;
    GCControlBlock* control_;
};

} // namespace rangelua::runtime

// Now include our GCPtr implementation
#include <rangelua/runtime/gc_ptr.hpp>

using namespace rangelua::runtime;
using namespace rangelua;

// Test objects for cycle detection
class TestNode : public GCObject {
public:
    explicit TestNode(int value) : GCObject(LuaType::TABLE), value_(value) {}
    
    void setNext(GCPtr<TestNode> next) { next_ = std::move(next); }
    GCPtr<TestNode> getNext() const { return next_; }
    
    void setWeakNext(WeakGCPtr<TestNode> next) { weakNext_ = std::move(next); }
    WeakGCPtr<TestNode> getWeakNext() const { return weakNext_; }
    
    int getValue() const { return value_; }
    
    void traverse(std::function<void(GCObject*)> visitor) override {
        if (next_) {
            visitor(next_.get());
        }
    }

private:
    int value_;
    GCPtr<TestNode> next_;
    WeakGCPtr<TestNode> weakNext_;
};

// Test basic GCPtr functionality
void testBasicGCPtr() {
    std::cout << "Testing basic GCPtr functionality...\n";
    
    // Test construction and destruction
    {
        auto node = makeGCPtr<TestNode>(42);
        assert(node);
        assert(node->getValue() == 42);
        assert(node.use_count() == 1);
        assert(node.unique());
    }
    
    // Test copy semantics
    {
        auto node1 = makeGCPtr<TestNode>(1);
        auto node2 = node1;  // Copy
        assert(node1.use_count() == 2);
        assert(node2.use_count() == 2);
        assert(node1 == node2);
        assert(!node1.unique());
        assert(!node2.unique());
    }
    
    // Test move semantics
    {
        auto node1 = makeGCPtr<TestNode>(1);
        auto node2 = std::move(node1);
        assert(!node1);  // Moved from
        assert(node2);
        assert(node2.use_count() == 1);
        assert(node2.unique());
    }
    
    std::cout << "Basic GCPtr tests passed!\n";
}

// Test weak reference functionality
void testWeakGCPtr() {
    std::cout << "Testing WeakGCPtr functionality...\n";
    
    WeakGCPtr<TestNode> weak;
    
    // Test weak reference lifecycle
    {
        auto strong = makeGCPtr<TestNode>(100);
        weak = strong.weak();
        
        assert(!weak.expired());
        assert(weak.use_count() == 1);
        
        auto locked = weak.lock();
        assert(locked);
        assert(locked->getValue() == 100);
        assert(weak.use_count() == 2);  // strong + locked
    }
    
    // After strong reference goes out of scope
    assert(weak.expired());
    auto locked = weak.lock();
    assert(!locked);  // Should be null
    
    std::cout << "WeakGCPtr tests passed!\n";
}

// Test cycle creation and detection
void testCycleDetection() {
    std::cout << "Testing cycle detection...\n";
    
    // Create a simple cycle: A -> B -> A
    auto nodeA = makeGCPtr<TestNode>(1);
    auto nodeB = makeGCPtr<TestNode>(2);
    
    nodeA->setNext(nodeB);
    nodeB->setNext(nodeA);  // Creates cycle
    
    // Test cycle detection
    bool hasCycleA = hasCycle(nodeA);
    bool hasCycleB = hasCycle(nodeB);
    
    std::cout << "Cycle detected from A: " << hasCycleA << "\n";
    std::cout << "Cycle detected from B: " << hasCycleB << "\n";
    
    // Break the cycle using weak reference
    nodeB->setNext(nullptr);  // Break strong cycle
    nodeB->setWeakNext(nodeA.weak());  // Use weak reference instead
    
    std::cout << "Cycle detection tests completed!\n";
}

// Test thread safety
void testThreadSafety() {
    std::cout << "Testing thread safety...\n";
    
    auto sharedNode = makeGCPtr<TestNode>(999);
    const int numThreads = 4;
    const int operationsPerThread = 1000;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&sharedNode, operationsPerThread]() {
            for (int j = 0; j < operationsPerThread; ++j) {
                // Create and destroy copies
                auto copy = sharedNode;
                auto weak = copy.weak();
                auto locked = weak.lock();
                
                // Simulate some work
                if (locked) {
                    volatile int value = locked->getValue();
                    (void)value;  // Suppress unused variable warning
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    assert(sharedNode.use_count() == 1);  // Should be back to 1
    std::cout << "Thread safety tests passed!\n";
}

// Performance comparison test
void testPerformance() {
    std::cout << "Testing performance...\n";
    
    const int iterations = 100000;
    
    // Test GCPtr performance
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto node = makeGCPtr<TestNode>(i);
        auto copy = node;
        auto weak = node.weak();
        auto locked = weak.lock();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "GCPtr operations took: " << duration.count() << " microseconds\n";
    std::cout << "Average per operation: " << (duration.count() / iterations) << " microseconds\n";
}

int main() {
    std::cout << "=== GCPtr and WeakGCPtr Test Suite ===\n\n";
    
    try {
        testBasicGCPtr();
        std::cout << "\n";
        
        testWeakGCPtr();
        std::cout << "\n";
        
        testCycleDetection();
        std::cout << "\n";
        
        testThreadSafety();
        std::cout << "\n";
        
        testPerformance();
        std::cout << "\n";
        
        std::cout << "=== All tests completed successfully! ===\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception\n";
        return 1;
    }
}
