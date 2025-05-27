/**
 * @file test_vm_comprehensive.cpp
 * @brief Comprehensive tests for the Virtual Machine implementation
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/vm.hpp>
#include <rangelua/runtime/value.hpp>

using namespace rangelua::backend;
using rangelua::runtime::Value;
using rangelua::runtime::VirtualMachine;
using rangelua::runtime::VMConfig;
using rangelua::runtime::VMState;
using rangelua::runtime::ExecutionContext;
using rangelua::runtime::VMDebugger;
using rangelua::Size;
using rangelua::String;
using rangelua::Number;
using rangelua::ErrorCode;
using rangelua::is_success;
using rangelua::get_value;
using rangelua::OpCode;

TEST_CASE("VM Basic Operations", "[vm][basic]") {
    SECTION("VM Creation and Configuration") {
        VMConfig config;
        config.stack_size = 512;
        config.call_stack_size = 128;

        VirtualMachine vm(config);

        REQUIRE(vm.state() == VMState::Ready);
        REQUIRE(vm.stack_size() == 0);
        REQUIRE(vm.call_depth() == 0);
        REQUIRE_FALSE(vm.is_running());
        REQUIRE_FALSE(vm.has_error());
    }

    SECTION("Stack Operations") {
        VirtualMachine vm;

        // Test push/pop operations
        vm.push(Value(42.0));
        vm.push(Value(true));
        vm.push(Value("hello"));

        REQUIRE(vm.stack_size() == 3);

        Value str_val = vm.pop();
        REQUIRE(str_val.is_string());

        Value bool_val = vm.pop();
        REQUIRE(bool_val.is_boolean());

        Value num_val = vm.pop();
        REQUIRE(num_val.is_number());

        REQUIRE(vm.stack_size() == 0);
    }

    SECTION("Global Variables") {
        VirtualMachine vm;

        vm.set_global("test_var", Value(123.0));
        Value retrieved = vm.get_global("test_var");

        REQUIRE(retrieved.is_number());
        auto num_result = retrieved.to_number();
        REQUIRE(is_success(num_result));
        REQUIRE(get_value(num_result) == 123.0);
    }
}

TEST_CASE("VM Instruction Execution", "[vm][instructions]") {
    SECTION("Load Instructions") {
        VirtualMachine vm;
        BytecodeEmitter emitter("test_load");

        // Create a simple function with load instructions
        emitter.emit_abc(OpCode::OP_LOADI, 0, 0, 42);      // R0 = 42
        emitter.emit_abc(OpCode::OP_LOADTRUE, 1, 0, 0);    // R1 = true
        emitter.emit_abc(OpCode::OP_LOADFALSE, 2, 0, 0);   // R2 = false
        emitter.emit_abc(OpCode::OP_LOADNIL, 3, 5, 0);     // R3-R5 = nil
        emitter.emit_abc(OpCode::OP_RETURN, 0, 6, 0);      // return R0-R5

        emitter.set_stack_size(10);
        emitter.set_parameter_count(0);

        auto function = emitter.get_function();
        auto result = vm.execute(function);

        REQUIRE(is_success(result));
        REQUIRE(vm.state() == VMState::Finished);
    }

    SECTION("Arithmetic Instructions") {
        VirtualMachine vm;
        BytecodeEmitter emitter("test_arithmetic");

        // Create function: return 10 + 5
        emitter.emit_abc(OpCode::OP_LOADI, 0, 0, 10);      // R0 = 10
        emitter.emit_abc(OpCode::OP_LOADI, 1, 0, 5);       // R1 = 5
        emitter.emit_abc(OpCode::OP_ADD, 2, 0, 1);         // R2 = R0 + R1
        emitter.emit_abc(OpCode::OP_RETURN, 2, 2, 0);      // return R2

        emitter.set_stack_size(10);
        emitter.set_parameter_count(0);

        auto function = emitter.get_function();
        auto result = vm.execute(function);

        REQUIRE(is_success(result));
        REQUIRE(vm.state() == VMState::Finished);
    }

    SECTION("Constant Loading") {
        VirtualMachine vm;
        BytecodeEmitter emitter("test_constants");

        // Add constants
        Size str_const = emitter.add_constant(ConstantValue{String("hello world")});
        Size num_const = emitter.add_constant(ConstantValue{Number(3.14159)});

        // Load constants
        emitter.emit_abx(OpCode::OP_LOADK, 0, str_const);  // R0 = "hello world"
        emitter.emit_abx(OpCode::OP_LOADK, 1, num_const);  // R1 = 3.14159
        emitter.emit_abc(OpCode::OP_RETURN, 0, 3, 0);      // return R0, R1

        emitter.set_stack_size(10);
        emitter.set_parameter_count(0);

        auto function = emitter.get_function();
        auto result = vm.execute(function);

        REQUIRE(is_success(result));
        REQUIRE(vm.state() == VMState::Finished);
    }
}

TEST_CASE("VM Table Operations", "[vm][tables]") {
    SECTION("Table Creation and Access") {
        VirtualMachine vm;
        BytecodeEmitter emitter("test_table");

        // Create table and set/get values
        emitter.emit_abc(OpCode::OP_NEWTABLE, 0, 0, 0);    // R0 = {}
        emitter.emit_abc(OpCode::OP_LOADI, 1, 0, 1);       // R1 = 1 (key)
        emitter.emit_abc(OpCode::OP_LOADI, 2, 0, 42);      // R2 = 42 (value)
        emitter.emit_abc(OpCode::OP_SETTABLE, 0, 1, 2);    // R0[R1] = R2
        emitter.emit_abc(OpCode::OP_GETTABLE, 3, 0, 1);    // R3 = R0[R1]
        emitter.emit_abc(OpCode::OP_RETURN, 3, 2, 0);      // return R3

        emitter.set_stack_size(10);
        emitter.set_parameter_count(0);

        auto function = emitter.get_function();
        auto result = vm.execute(function);

        REQUIRE(is_success(result));
        REQUIRE(vm.state() == VMState::Finished);
    }
}

TEST_CASE("VM Control Flow", "[vm][control]") {
    SECTION("Jump Instructions") {
        VirtualMachine vm;
        BytecodeEmitter emitter("test_jump");

        // Simple jump test
        emitter.emit_abc(OpCode::OP_LOADI, 0, 0, 1);       // R0 = 1
        emitter.emit_asbx(OpCode::OP_JMP, 0, 2);           // jump +2
        emitter.emit_abc(OpCode::OP_LOADI, 0, 0, 2);       // R0 = 2 (skipped)
        emitter.emit_abc(OpCode::OP_LOADI, 0, 0, 3);       // R0 = 3 (skipped)
        emitter.emit_abc(OpCode::OP_LOADI, 1, 0, 42);      // R1 = 42 (executed)
        emitter.emit_abc(OpCode::OP_RETURN, 0, 3, 0);      // return R0, R1

        emitter.set_stack_size(10);
        emitter.set_parameter_count(0);

        auto function = emitter.get_function();
        auto result = vm.execute(function);

        REQUIRE(is_success(result));
        REQUIRE(vm.state() == VMState::Finished);
    }
}

TEST_CASE("VM Error Handling", "[vm][errors]") {
    SECTION("Invalid Instruction Handling") {
        VirtualMachine vm;

        // Test runtime error triggering
        vm.trigger_runtime_error("Test error message");
        REQUIRE(vm.has_error());
        REQUIRE(vm.last_error() == ErrorCode::RUNTIME_ERROR);
    }

    SECTION("Stack Overflow Protection") {
        VMConfig config;
        config.stack_size = 5;  // Very small stack
        VirtualMachine vm(config);

        // Try to push more than stack size
        for (int i = 0; i < 10; ++i) {
            vm.push(Value(static_cast<Number>(i)));
        }

        // VM should handle this gracefully
        REQUIRE(vm.stack_size() <= config.stack_size);
    }
}

TEST_CASE("VM Debugging Support", "[vm][debug]") {
    SECTION("Execution Context") {
        VirtualMachine vm;
        ExecutionContext context(vm);

        // Save initial state
        context.save_state();
        REQUIRE(context.is_valid());

        // Modify VM state
        vm.push(Value(42.0));
        vm.set_global("test", Value(true));

        // Restore state
        context.restore_state();
        REQUIRE_FALSE(context.is_valid());
    }

    SECTION("VM Debugger") {
        VirtualMachine vm;
        VMDebugger debugger(vm);

        // Set breakpoints
        debugger.set_breakpoint(0);
        debugger.set_breakpoint(5);

        // Remove breakpoint
        debugger.remove_breakpoint(5);

        // Test step operations
        auto step_result = debugger.step_instruction();
        REQUIRE(is_success(step_result));
    }
}
