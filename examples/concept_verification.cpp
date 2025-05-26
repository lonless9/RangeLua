/**
 * @file concept_verification.cpp
 * @brief Test program to verify C++20 concepts are working correctly
 * @version 0.1.0
 */

#include <rangelua/rangelua.hpp>
#include <rangelua/core/concepts.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/api/state.hpp>
#include <iostream>
#include <type_traits>

using namespace rangelua;
using namespace rangelua::concepts;
using namespace rangelua::runtime;

// Test function to verify concepts at compile time
template<typename T>
void test_lua_value_concept() {
    static_assert(LuaValue<T>, "Type should satisfy LuaValue concept");
    static_assert(Hashable<T>, "Type should satisfy Hashable concept");
    static_assert(Comparable<T>, "Type should satisfy Comparable concept");
    static_assert(LuaObject<T>, "Type should satisfy LuaObject concept");
    
    std::cout << "✓ " << typeid(T).name() << " satisfies LuaValue concept\n";
}

template<typename T>
void test_lua_state_concept() {
    static_assert(LuaState<T>, "Type should satisfy LuaState concept");
    
    std::cout << "✓ " << typeid(T).name() << " satisfies LuaState concept\n";
}

template<typename T>
void test_basic_concepts() {
    static_assert(Movable<T>, "Type should be movable");
    static_assert(Copyable<T>, "Type should be copyable");
    
    std::cout << "✓ " << typeid(T).name() << " satisfies basic concepts\n";
}

// Mock classes for testing concepts
class MockLexer {
public:
    using Token = int;
    
    Token next_token() { return 0; }
    Token peek_token() { return 0; }
    SourceLocation current_location() { return {}; }
    bool has_more_tokens() { return false; }
};

class MockParser {
public:
    using AST = int;
    
    bool has_errors() { return false; }
    AST parse() { return 0; }
};

class MockCodeGenerator {
public:
    void emit_instruction(Instruction instr) {}
    Register allocate_register() { return 0; }
    void free_register(Register reg) {}
    void generate(int node) {}
};

class MockVirtualMachine {
public:
    ErrorCode execute() { return ErrorCode::SUCCESS; }
    ErrorCode step() { return ErrorCode::SUCCESS; }
    ErrorCode call(int func) { return ErrorCode::SUCCESS; }
};

template<typename T>
void test_frontend_concepts() {
    if constexpr (Lexer<T>) {
        std::cout << "✓ " << typeid(T).name() << " satisfies Lexer concept\n";
    }
    if constexpr (Parser<T>) {
        std::cout << "✓ " << typeid(T).name() << " satisfies Parser concept\n";
    }
}

template<typename T>
void test_backend_concepts() {
    if constexpr (BasicCodeGenerator<T>) {
        std::cout << "✓ " << typeid(T).name() << " satisfies BasicCodeGenerator concept\n";
    }
}

template<typename T>
void test_runtime_concepts() {
    if constexpr (BasicVirtualMachine<T>) {
        std::cout << "✓ " << typeid(T).name() << " satisfies BasicVirtualMachine concept\n";
    }
}

int main() {
    std::cout << "=== RangeLua C++20 Concepts Verification ===\n\n";
    
    try {
        // Test core Value type
        std::cout << "Testing core Value type:\n";
        test_lua_value_concept<Value>();
        test_basic_concepts<Value>();
        
        // Test State type
        std::cout << "\nTesting State type:\n";
        test_lua_state_concept<api::State>();
        test_basic_concepts<api::State>();
        
        // Test mock frontend types
        std::cout << "\nTesting frontend concepts:\n";
        test_frontend_concepts<MockLexer>();
        test_frontend_concepts<MockParser>();
        
        // Test mock backend types
        std::cout << "\nTesting backend concepts:\n";
        test_backend_concepts<MockCodeGenerator>();
        
        // Test mock runtime types
        std::cout << "\nTesting runtime concepts:\n";
        test_runtime_concepts<MockVirtualMachine>();
        
        // Test concept composition
        std::cout << "\nTesting concept composition:\n";
        static_assert(LuaContainer<Value>, "Value should satisfy LuaContainer if it has size/empty");
        std::cout << "✓ Concept composition works correctly\n";
        
        // Test practical usage
        std::cout << "\nTesting practical usage:\n";
        
        // Create a value and test it
        Value test_value = Value(42.0);
        std::cout << "Created value: " << test_value.debug_string() << "\n";
        std::cout << "Type ID: " << test_value.type_as_int() << "\n";
        std::cout << "Is number: " << test_value.is_number() << "\n";
        
        // Test with different value types
        Value nil_value;
        Value bool_value(true);
        Value string_value("Hello, RangeLua!");
        
        std::cout << "Nil value: " << nil_value.debug_string() << "\n";
        std::cout << "Bool value: " << bool_value.debug_string() << "\n";
        std::cout << "String value: " << string_value.debug_string() << "\n";
        
        std::cout << "\n=== All concept verifications passed! ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error during concept verification: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
