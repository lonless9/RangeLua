#pragma once

/**
 * @file concepts.hpp
 * @brief C++20 concepts for type safety and template constraints
 * @version 0.1.0
 */

#include <concepts>
#include <functional>
#include <type_traits>

#include "instruction.hpp"
#include "types.hpp"

namespace rangelua::concepts {

    // Forward declarations to avoid circular dependencies
    namespace detail {
        template <typename T>
        struct is_lua_value : std::false_type {};

        template <typename T>
        struct is_lua_table : std::false_type {};

        template <typename T>
        struct is_lua_function : std::false_type {};
    }  // namespace detail

    // Core type concepts with improved flexibility
    template <typename T>
    concept LuaValue = requires(T t) {
        typename T::value_type;
        // Support both direct int return and convertible types
        requires std::convertible_to<decltype(t.type_as_int()), int> ||
                     std::convertible_to<decltype(t.type_id()), int> || requires {
                         { static_cast<int>(t.type()) } -> std::convertible_to<int>;
                     };
        { t.is_nil() } -> std::convertible_to<bool>;
        { t.is_boolean() } -> std::convertible_to<bool>;
        { t.is_number() } -> std::convertible_to<bool>;
        { t.is_string() } -> std::convertible_to<bool>;
        { t.is_table() } -> std::convertible_to<bool>;
        { t.is_function() } -> std::convertible_to<bool>;
    };

    // Relaxed table concept that doesn't require specific Value type
    template <typename T, typename ValueType = typename T::value_type>
    concept LuaTable = requires(T t, ValueType key, ValueType value) {
        { t.size() } -> std::convertible_to<Size>;
        { t.empty() } -> std::convertible_to<bool>;
        { t.get(key) } -> std::convertible_to<ValueType>;
        { t.set(key, value) } -> std::same_as<void>;
    };

    // Alternative table concept for when Value type is not available
    template <typename T>
    concept BasicTable = requires(T t) {
        { t.size() } -> std::convertible_to<Size>;
        { t.empty() } -> std::convertible_to<bool>;
    };

    template <typename T>
    concept LuaFunction = requires(T t) {
        { t.arity() } -> std::convertible_to<Size>;
        { t.is_native() } -> std::convertible_to<bool>;
        { t.is_lua() } -> std::convertible_to<bool>;
    };

    // Flexible state concept that works with different value types
    template <typename T, typename ValueType = typename T::value_type>
    concept LuaState = requires(T t, ValueType value) {
        { t.stack_size() } -> std::convertible_to<Size>;
        { t.push(value) } -> std::same_as<void>;
        { t.pop() } -> std::convertible_to<ValueType>;
        { t.top() } -> std::convertible_to<ValueType>;
    };

    // Memory management concepts with improved flexibility
    template <typename T>
    concept GCObject = requires(T t) {
        { t.mark() } -> std::same_as<void>;
        { t.is_marked() } -> std::convertible_to<bool>;
        { t.size() } -> std::convertible_to<Size>;
    };

    // More flexible GC object concept for objects that may not have all methods
    template <typename T>
    concept BasicGCObject = requires(T t) {
        { t.is_marked() } -> std::convertible_to<bool>;
    } && (requires(T t) { { t.mark() } -> std::same_as<void>; } ||
          requires(T t) { { t.set_marked(std::declval<bool>()) } -> std::same_as<void>; });

    template <typename T>
    concept MemoryAllocator = requires(T t, Size size, void* ptr) {
        { t.allocate(size) } -> std::convertible_to<void*>;
        { t.deallocate(ptr, size) } -> std::same_as<void>;
        { t.reallocate(ptr, size, size) } -> std::convertible_to<void*>;
    };

    // Alternative allocator concept for simpler allocators
    template <typename T>
    concept BasicAllocator = requires(T t, Size size, void* ptr) {
        { t.allocate(size) } -> std::convertible_to<void*>;
        { t.deallocate(ptr) } -> std::same_as<void>;
    };

    // Frontend concepts with better type safety
    template <typename T>
    concept Lexer = requires(T t) {
        typename T::Token;
        { t.next_token() } -> std::convertible_to<typename T::Token>;
        { t.peek_token() } -> std::convertible_to<typename T::Token>;
        { t.current_location() } -> std::convertible_to<SourceLocation>;
        { t.has_more_tokens() } -> std::convertible_to<bool>;
    };

    // Flexible parser concept that supports different return types
    template <typename T>
    concept Parser = requires(T t) {
        { t.has_errors() } -> std::convertible_to<bool>;
    } && (requires(T t) {
        typename T::AST;
        { t.parse() } -> std::convertible_to<typename T::AST>;
    } || requires(T t) {
        { t.parse() } -> std::same_as<void>;
    });

    // AST node concept with visitor pattern support
    template <typename T, typename VisitorType = void>
    concept ASTNode = requires(T node) {
        typename T::NodeType;
        { node.type() } -> std::convertible_to<typename T::NodeType>;
        { node.location() } -> std::convertible_to<SourceLocation>;
    } && (std::same_as<VisitorType, void> || requires(T node, VisitorType visitor) {
                          node.accept(visitor);
                      });

    // Basic AST node without visitor requirement
    template <typename T>
    concept BasicASTNode = requires(T node) {
        typename T::NodeType;
        { node.type() } -> std::convertible_to<typename T::NodeType>;
        { node.location() } -> std::convertible_to<SourceLocation>;
    };

    // Backend concepts with improved type safety and flexibility
    template <typename T, typename ASTNodeType = void>
    concept CodeGenerator = requires(T generator) {
        { generator.emit_instruction(std::declval<Instruction>()) } -> std::same_as<void>;
        { generator.allocate_register() } -> std::convertible_to<Register>;
        { generator.free_register(std::declval<Register>()) } -> std::same_as<void>;
    } && (std::same_as<ASTNodeType, void> || requires(T generator, ASTNodeType node) {
                                generator.generate(node);
                            });

    // Basic code generator without AST node requirement
    template <typename T>
    concept BasicCodeGenerator = requires(T generator) {
        { generator.emit_instruction(std::declval<Instruction>()) } -> std::same_as<void>;
        { generator.allocate_register() } -> std::convertible_to<Register>;
        { generator.free_register(std::declval<Register>()) } -> std::same_as<void>;
    };

    template <typename T, typename FunctionType = void>
    concept Optimizer = requires(T t) {
        typename T::OptimizationLevel;
        { t.optimization_level() } -> std::convertible_to<typename T::OptimizationLevel>;
        { t.is_pass_enabled(std::declval<StringView>()) } -> std::convertible_to<bool>;
    } && (std::same_as<FunctionType, void> || requires(T t, FunctionType& func) {
                            t.optimize(func);
                        });

    // Basic optimizer without function type requirement
    template <typename T>
    concept BasicOptimizer = requires(T t) {
        typename T::OptimizationLevel;
        { t.optimization_level() } -> std::convertible_to<typename T::OptimizationLevel>;
        { t.is_pass_enabled(std::declval<StringView>()) } -> std::convertible_to<bool>;
    };

    template <typename T>
    concept BytecodeEmitter = requires(T emitter) {
        { emitter.emit(std::declval<OpCode>()) } -> std::same_as<void>;
    } && (requires(T emitter) {
        { emitter.get_bytecode() } -> Range;
    } || requires(T emitter) {
        { emitter.get_function() } -> std::convertible_to<void*>;
    });

    // More flexible emitter that supports different argument patterns
    template <typename T>
    concept FlexibleBytecodeEmitter = BytecodeEmitter<T> &&
        (requires(T emitter, OpCode op, int arg) {
            emitter.emit_with_args(op, arg);
        } || requires(T emitter, OpCode op, Register a, Register b, Register c) {
            emitter.emit_abc(op, a, b, c);
        });

    // Runtime concepts with improved flexibility
    template <typename T, typename FunctionType = void>
    concept VirtualMachine = requires(T t) {
        { t.step() } -> std::convertible_to<ErrorCode>;
    } && (requires(T t) {
        { t.execute() } -> std::convertible_to<ErrorCode>;
    } || requires(T t, FunctionType func) {
        { t.execute(func) } -> std::convertible_to<ErrorCode>;
    }) && (std::same_as<FunctionType, void> || requires(T t, FunctionType func) {
        { t.call(func) } -> std::convertible_to<ErrorCode>;
    });

    // Basic VM concept without function type requirement
    template <typename T>
    concept BasicVirtualMachine = requires(T t) {
        { t.step() } -> std::convertible_to<ErrorCode>;
        { t.execute() } -> std::convertible_to<ErrorCode>;
    };

    template <typename T>
    concept GarbageCollector = requires(T collector) {
        { collector.collect() } -> std::same_as<void>;
    } && (requires(T collector) {
        { collector.mark_phase() } -> std::same_as<void>;
        { collector.sweep_phase() } -> std::same_as<void>;
    } || requires(T collector) {
        { collector.run_collection() } -> std::same_as<void>;
    }) && (requires(T collector, void* ptr) {
        collector.add_root(ptr);
    } || requires(T collector) {
        { collector.get_roots() } -> std::convertible_to<void*>;
    });

    // Design pattern concepts with better type safety
    template <typename V, typename... Nodes>
    concept Visitor = requires(V v) { (requires { v.visit(std::declval<Nodes>()); } && ...); };

    // More flexible visitor concept
    template <typename V, typename NodeType>
    concept SingleNodeVisitor = requires(V v, NodeType node) { v.visit(node); };

    template <typename T, typename VisitorType = void>
    concept Visitable =
        (std::same_as<VisitorType, void> && requires(T visitable) {
            // Accept generic visitor
            visitable.accept(std::declval<void*>());
        }) || (!std::same_as<VisitorType, void> && requires(T visitable, VisitorType visitor) {
            visitable.accept(visitor);
        });

    // Strategy pattern concepts with context support
    template <typename T, typename ContextType = void>
    concept Strategy =
        (std::same_as<ContextType, void> && requires(T strategy) {
            { strategy.execute() } -> std::same_as<void>;
        }) || (!std::same_as<ContextType, void> && requires(T strategy, ContextType context) {
            { strategy.execute(context) } -> std::same_as<void>;
        });

    // Observer pattern concepts with event type support
    template <typename T, typename EventType = void>
    concept Observer =
        (std::same_as<EventType, void> && requires(T observer) { observer.notify(); }) ||
        (!std::same_as<EventType, void> &&
         requires(T observer, EventType event) { observer.notify(event); });

    template <typename T, typename ObserverType = void, typename EventType = void>
    concept Observable =
        (std::same_as<ObserverType, void> && requires(T observable) {
            { observable.add_observer(std::declval<void*>()) } -> std::same_as<void>;
            { observable.remove_observer(std::declval<void*>()) } -> std::same_as<void>;
            { observable.notify_observers() } -> std::same_as<void>;
        }) || (!std::same_as<ObserverType, void> && requires(T observable, ObserverType observer) {
            { observable.add_observer(observer) } -> std::same_as<void>;
            { observable.remove_observer(observer) } -> std::same_as<void>;
        } && (std::same_as<EventType, void> || requires(T observable, EventType event) {
            { observable.notify_observers(event) } -> std::same_as<void>;
        }));

    // Command pattern concepts with improved flexibility
    template <typename T>
    concept Command = requires(T t) {
        { t.execute() } -> std::same_as<void>;
    } && (requires(T t) {
        { t.undo() } -> std::same_as<void>;
        { t.can_undo() } -> std::convertible_to<bool>;
    } || requires(T t) {
        { t.is_reversible() } -> std::convertible_to<bool>;
    });

    // Basic command without undo support
    template <typename T>
    concept BasicCommand = requires(T t) {
        { t.execute() } -> std::same_as<void>;
    };

    // Factory pattern concepts with variadic template support
    template <typename F, typename T, typename... Args>
    concept Factory = requires(F factory, Args... args) {
        { factory.create(args...) } -> std::convertible_to<T>;
    } || requires(F factory) {
        { factory.create() } -> std::convertible_to<T>;
    };

    // Basic factory without arguments
    template <typename F, typename T>
    concept BasicFactory = requires(F factory) {
        { factory.create() } -> std::convertible_to<T>;
    };

    // Coroutine concepts with Lua 5.5 compatibility
    template <typename T>
    concept Coroutine = requires(T t) {
        { t.is_suspended() } -> std::convertible_to<bool>;
        { t.is_dead() } -> std::convertible_to<bool>;
    } && (requires(T t) {
        { t.resume() } -> std::convertible_to<ErrorCode>;
        { t.yield() } -> std::same_as<void>;
    } || requires(T t) {
        { t.resume() } -> std::same_as<void>;
        { t.yield() } -> std::convertible_to<ErrorCode>;
    });

    // Utility concepts with modern C++20 features
    template <typename T>
    concept Serializable = requires(T t) {
        { t.serialize() } -> StringLike;
    } && (requires {
        { T::deserialize(std::declval<StringView>()) } -> std::convertible_to<T>;
    } || requires(StringView sv) {
        { T::from_string(sv) } -> std::convertible_to<T>;
    });

    template <typename T>
    concept Hashable = requires(T t) {
        { std::hash<T>{}(t) } -> std::convertible_to<Size>;
    };

    // Enhanced comparable concept with three-way comparison support
    template <typename T>
    concept Comparable = requires(T a, T b) {
        { a == b } -> std::convertible_to<bool>;
        { a != b } -> std::convertible_to<bool>;
    } && (requires(T a, T b) {
        { a <=> b } -> std::convertible_to<std::strong_ordering>;
    } || requires(T a, T b) {
        { a < b } -> std::convertible_to<bool>;
        { a <= b } -> std::convertible_to<bool>;
        { a > b } -> std::convertible_to<bool>;
        { a >= b } -> std::convertible_to<bool>;
    });

    // Additional utility concepts for modern C++20
    template <typename T>
    concept Movable = std::movable<T>;

    template <typename T>
    concept Copyable = std::copyable<T>;

    template <typename T>
    concept DefaultConstructible = std::default_initializable<T>;

    // Range concepts for Lua collections
    template <typename R>
    concept LuaRange = Range<R> && requires(R r) { typename std::ranges::range_value_t<R>; };

    template <typename R, typename T>
    concept LuaRangeOf = LuaRange<R> && std::same_as<std::ranges::range_value_t<R>, T>;

    // Concept composition helpers for common Lua patterns
    template <typename T>
    concept LuaObject = LuaValue<T> && Hashable<T> && Comparable<T>;

    template <typename T>
    concept LuaContainer = LuaObject<T> && requires(T t) {
        { t.size() } -> std::convertible_to<Size>;
        { t.empty() } -> std::convertible_to<bool>;
    };

    // Concept verification helpers
    namespace detail {
        // Helper for conditional concept checking
        template <typename T, typename U>
        inline constexpr bool types_compatible_v =
            std::convertible_to<T, U> || std::convertible_to<U, T>;
    }  // namespace detail

}  // namespace rangelua::concepts
