#pragma once

/**
 * @file concepts.hpp
 * @brief C++20 concepts for type safety and template constraints
 * @version 0.1.0
 */

#include "instruction.hpp"
#include "types.hpp"
#include <concepts>
#include <functional>

namespace rangelua::concepts {

// Basic type concepts
template<typename T>
concept LuaValue = requires(T t) {
    typename T::value_type;
    { t.type() } -> std::convertible_to<int>;
    { t.is_nil() } -> std::convertible_to<bool>;
    { t.is_boolean() } -> std::convertible_to<bool>;
    { t.is_number() } -> std::convertible_to<bool>;
    { t.is_string() } -> std::convertible_to<bool>;
    { t.is_table() } -> std::convertible_to<bool>;
    { t.is_function() } -> std::convertible_to<bool>;
};

template<typename T>
concept LuaTable = requires(T t) {
    { t.size() } -> std::convertible_to<Size>;
    { t.empty() } -> std::convertible_to<bool>;
    { t.get(std::declval<Value>()) } -> std::convertible_to<Value>;
    { t.set(std::declval<Value>(), std::declval<Value>()) } -> std::same_as<void>;
};

template<typename T>
concept LuaFunction = requires(T t) {
    { t.arity() } -> std::convertible_to<Size>;
    { t.is_native() } -> std::convertible_to<bool>;
    { t.is_lua() } -> std::convertible_to<bool>;
};

template<typename T>
concept LuaState = requires(T t) {
    { t.stack_size() } -> std::convertible_to<Size>;
    { t.push(std::declval<Value>()) } -> std::same_as<void>;
    { t.pop() } -> std::convertible_to<Value>;
    { t.top() } -> std::convertible_to<Value>;
};

// Memory management concepts
template<typename T>
concept GCObject = requires(T t) {
    { t.mark() } -> std::same_as<void>;
    { t.is_marked() } -> std::convertible_to<bool>;
    { t.size() } -> std::convertible_to<Size>;
};

template<typename T>
concept MemoryAllocator = requires(T t, Size size) {
    { t.allocate(size) } -> std::convertible_to<void*>;
    { t.deallocate(std::declval<void*>(), size) } -> std::same_as<void>;
    { t.reallocate(std::declval<void*>(), size, size) } -> std::convertible_to<void*>;
};

// Frontend concepts
template<typename T>
concept Lexer = requires(T t) {
    typename T::Token;
    { t.next_token() } -> std::convertible_to<typename T::Token>;
    { t.peek_token() } -> std::convertible_to<typename T::Token>;
    { t.current_location() } -> std::convertible_to<SourceLocation>;
};

template<typename T>
concept Parser = requires(T t) {
    typename T::AST;
    { t.has_errors() } -> std::convertible_to<bool>;
} && requires(T t) {
    // Allow parse() to return either AST directly or Result<AST>
    t.parse();
};

template<typename T>
concept ASTNode = requires(T node) {
    typename T::NodeType;
    { node.type() } -> std::convertible_to<typename T::NodeType>;
    { node.location() } -> std::convertible_to<SourceLocation>;
} && requires(T node) {
    // Accept any visitor type
    node.accept(std::declval<int>());  // Placeholder for visitor
};

// Backend concepts
template<typename T>
concept CodeGenerator = requires(T generator) {
    { generator.emit_instruction(std::declval<Instruction>()) } -> std::same_as<void>;
    { generator.allocate_register() } -> std::convertible_to<Register>;
    { generator.free_register(std::declval<Register>()) } -> std::same_as<void>;
} && requires(T generator) {
    // Generate from any AST node type
    generator.generate(std::declval<int>());  // Placeholder for AST node
};

template<typename T>
concept Optimizer = requires(T t) {
    typename T::OptimizationLevel;
    { t.optimization_level() } -> std::convertible_to<typename T::OptimizationLevel>;
    { t.is_pass_enabled(std::declval<StringView>()) } -> std::convertible_to<bool>;
} && requires(T t) {
    // Optimize with any function type
    t.optimize(std::declval<int&>());  // Placeholder for BytecodeFunction
};

template<typename T>
concept BytecodeEmitter = requires(T emitter) {
    { emitter.emit(std::declval<OpCode>()) } -> std::same_as<void>;
    { emitter.get_bytecode() } -> Range;
} && requires(T emitter) {
    // Emit with variable arguments
    emitter.emit_with_args(std::declval<OpCode>(), std::declval<int>());
};

// Runtime concepts
template<typename T>
concept VirtualMachine = requires(T t) {
    { t.execute() } -> std::convertible_to<ErrorCode>;
    { t.step() } -> std::convertible_to<ErrorCode>;
    { t.call(std::declval<Function>()) } -> std::convertible_to<ErrorCode>;
};

template<typename T>
concept GarbageCollector = requires(T collector) {
    { collector.collect() } -> std::same_as<void>;
    { collector.mark_phase() } -> std::same_as<void>;
    { collector.sweep_phase() } -> std::same_as<void>;
} && requires(T collector) {
    // Add root with any GC object pointer
    collector.add_root(std::declval<void*>());
};

// Visitor pattern concepts
template<typename V, typename... Nodes>
concept Visitor = requires(V v) {
    (requires { v.visit(std::declval<Nodes>()); } && ...);
};

template<typename T>
concept Visitable = requires(T visitable) {
    // Accept any visitor type
    visitable.accept(std::declval<int>());
};

// Strategy pattern concepts
template<typename T>
concept Strategy = requires(T strategy) {
    { strategy.execute() } -> std::same_as<void>;
};

// Observer pattern concepts
template<typename T>
concept Observer = requires(T observer) {
    // Notify with any event type
    observer.notify(std::declval<int>());
};

template<typename T>
concept Observable = requires(T observable) {
    { observable.add_observer(std::declval<int>()) } -> std::same_as<void>;
    { observable.remove_observer(std::declval<int>()) } -> std::same_as<void>;
    { observable.notify_observers(std::declval<int>()) } -> std::same_as<void>;
};

// Command pattern concepts
template<typename T>
concept Command = requires(T t) {
    { t.execute() } -> std::same_as<void>;
    { t.undo() } -> std::same_as<void>;
    { t.can_undo() } -> std::convertible_to<bool>;
};

// Factory pattern concepts
template<typename F, typename T>
concept Factory = requires(F factory) {
    { factory.create() } -> std::convertible_to<T>;
} && requires(F factory) {
    // Create with variable arguments
    factory.create(std::declval<int>());
};

// Coroutine concepts
template<typename T>
concept Coroutine = requires(T t) {
    { t.resume() } -> std::convertible_to<ErrorCode>;
    { t.yield() } -> std::same_as<void>;
    { t.is_suspended() } -> std::convertible_to<bool>;
    { t.is_dead() } -> std::convertible_to<bool>;
};

// Utility concepts
template<typename T>
concept Serializable = requires(T t) {
    { t.serialize() } -> StringLike;
    { T::deserialize(std::declval<StringView>()) } -> std::convertible_to<T>;
};

template<typename T>
concept Hashable = requires(T t) {
    { std::hash<T>{}(t) } -> std::convertible_to<Size>;
};

template<typename T>
concept Comparable = requires(T a, T b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
    { a < b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
};

} // namespace rangelua::concepts
