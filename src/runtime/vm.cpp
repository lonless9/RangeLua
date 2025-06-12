/**
* @file vm.cpp
* @brief Virtual machine implementation stub
* @version 0.1.0
*/


#include <rangelua/core/error.hpp>
#include <rangelua/runtime/metamethod.hpp>
#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/vm.hpp>
#include <rangelua/runtime/vm/all_strategies.hpp>
#include <rangelua/utils/debug.hpp>
#include <rangelua/utils/logger.hpp>

#include <cstdint>
#include <algorithm>

namespace rangelua::runtime {

// VirtualMachine implementation
VirtualMachine::VirtualMachine(VMConfig config) : config_(config) {
    // Create default memory manager if none provided
    auto memory_result = getMemoryManager();
    if (is_success(memory_result)) {
        memory_manager_ = get_value(memory_result);
    } else {
        // Use factory to create concrete implementation
        owned_memory_manager_ = MemoryManagerFactory::create_runtime_manager();
        memory_manager_ = owned_memory_manager_.get();
    }

    // Initialize strategy registry
    strategy_registry_ = InstructionStrategyFactory::create_registry();

    // Initialize environment system
    registry_ = std::make_unique<Registry>();
    auto global_table = registry_->getGlobalTable();
    environment_ = std::make_unique<Environment>(global_table);

    stack_.reserve(config_.stack_size);
    call_stack_.reserve(config_.call_stack_size);
}

VirtualMachine::VirtualMachine(RuntimeMemoryManager& memory_manager, VMConfig config)
    : config_(config), memory_manager_(&memory_manager) {
    // Initialize strategy registry
    strategy_registry_ = InstructionStrategyFactory::create_registry();

    // Initialize environment system
    registry_ = std::make_unique<Registry>();
    auto global_table = registry_->getGlobalTable();
    environment_ = std::make_unique<Environment>(global_table);

    stack_.reserve(config_.stack_size);
    call_stack_.reserve(config_.call_stack_size);
}

Result<std::vector<Value>> VirtualMachine::pcall(const Value& function,
                                                 const std::vector<Value>& args) {
    // pcall is just xpcall with a nil message handler.
    return xpcall(function, Value{}, args);
}

Result<std::vector<Value>> VirtualMachine::xpcall(const Value& function,
                                                  const Value& msgh,
                                                  const std::vector<Value>& args) {
    if (!function.is_function()) {
        return make_success(std::vector<Value>{
            Value(false), Value("bad argument #1 to pcall/xpcall (function expected)")});
    }

    size_t original_call_stack_size = call_stack_.size();
    size_t original_stack_top = stack_top_;
    VMState original_state = state_;

    try {
        auto call_result = call(function, args);
        // If `call` returns an ErrorCode, it means setup failed before execution.
        if (is_error(call_result)) {
            // This will now be caught as an exception, but handle for safety
            error_obj_ = Value(error_code_to_string(get_error(call_result)));
            throw RuntimeError("Call setup failed");
        }

        auto values = get_value(std::move(call_result));
        std::vector<Value> success_results;
        success_results.emplace_back(true);
        success_results.insert(success_results.end(),
                               std::make_move_iterator(values.begin()),
                               std::make_move_iterator(values.end()));
        return make_success(std::move(success_results));

    } catch (const RuntimeError&) {
        Value original_error = error_obj_;

        // Restore state to before the failed call
        if (call_stack_.size() > original_call_stack_size) {
            call_stack_.resize(original_call_stack_size);
        }
        stack_top_ = original_stack_top;
        state_ = original_state;

        if (!msgh.is_function()) {
            return make_success(std::vector<Value>{Value(false), original_error});
        }

        // Call the message handler in its own protected context.
        try {
            auto msgh_results_res = call(msgh, {original_error});
            if (is_error(msgh_results_res)) {
                // error in message handler during setup
                return make_success(
                    std::vector<Value>{Value(false), Value("error in error handling")});
            }
            auto results = get_value(std::move(msgh_results_res));
            Value final_error = results.empty() ? Value{} : results[0];
            return make_success(std::vector<Value>{Value(false), final_error});
        } catch (const RuntimeError&) {
            // The message handler itself raised an error.
            // State is already restored from the first catch.
            // Restore stack state again in case message handler messed with it.
            if (call_stack_.size() > original_call_stack_size) {
                call_stack_.resize(original_call_stack_size);
            }
            stack_top_ = original_stack_top;
            state_ = original_state;
            return make_success(
                std::vector<Value>{Value(false), Value("error in error handling")});
        }
    }
}

Result<std::vector<Value>> VirtualMachine::execute(const backend::BytecodeFunction& function,
                                                     const std::vector<Value>& args) {
    VM_LOG_INFO("Starting execution of function: {}", function.name);
    VM_LOG_DEBUG("Function has {} instructions, {} constants",
                 function.instructions.size(),
                 function.constants.size());

    // The main execution loop is now wrapped in a try-catch block
    // to handle uncaught runtime errors.
    try {
        // Create a heap-allocated copy of the function to ensure its lifetime
        auto func_owner = std::make_unique<backend::BytecodeFunction>(function);

        // Setup initial call frame, transferring ownership of the function object
        auto setup_result =
            setup_call_frame(std::move(func_owner), {}, args.size(), stack_top_);
        if (std::holds_alternative<ErrorCode>(setup_result)) {
            VM_LOG_ERROR("Failed to setup call frame");
            return std::get<ErrorCode>(setup_result);
        }

        // Copy arguments to stack
        VM_LOG_DEBUG("Copying {} arguments to stack", args.size());
        for (const auto& arg : args) {
            push(arg);
        }

        state_ = VMState::Running;
        VM_LOG_DEBUG("VM state set to Running, starting execution loop");

        // Main execution loop
        size_t instruction_count = 0;
        while (state_ == VMState::Running && !call_stack_.empty()) {
            auto step_result = step();
            instruction_count++;

            // step() no longer returns an error code for runtime errors, it throws.
            // We only need to check for setup/internal errors here.
            if (std::holds_alternative<ErrorCode>(step_result)) {
                VM_LOG_ERROR("VM execution failed at instruction {}", instruction_count);
                state_ = VMState::Error;
                return std::get<ErrorCode>(step_result);
            }
        }

        VM_LOG_DEBUG("VM execution completed after {} instructions", instruction_count);

        // Collect results
        std::vector<Value> results;
        // TODO: Collect return values from stack

        state_ = VMState::Finished;
        VM_LOG_INFO("VM execution finished successfully");
        return results;

    } catch (const RuntimeError& e) {
        // This is the top-level handler for uncaught runtime errors.
        state_ = VMState::Error;
        last_error_ = ErrorCode::RUNTIME_ERROR;

        // error_obj_ should have been set by trigger_runtime_error
        String error_message = error_obj_.is_string()
                                   ? error_obj_.as_string()
                                   : "An unknown runtime error occurred.";

        // Generate stack trace and print the error to stderr, similar to the default Lua
        // interpreter.
        String stack_trace = generate_stack_trace_string();
        fprintf(stderr, "rangelua: %s\n%s\n", error_message.c_str(), stack_trace.c_str());

        return ErrorCode::RUNTIME_ERROR;
    } catch (const Exception& e) {
        state_ = VMState::Error;
        return e.code();
    } catch (const std::exception& e) {
        state_ = VMState::Error;
        return ErrorCode::RUNTIME_ERROR;
    } catch (...) {
        state_ = VMState::Error;
        return ErrorCode::UNKNOWN_ERROR;
    }
}

Status VirtualMachine::step() {
    if (call_stack_.empty()) {
        VM_LOG_DEBUG("VM finished - no more call frames");
        state_ = VMState::Finished;
        return std::monostate{};
    }

    auto& frame = call_stack_.back();
    if (!frame.function || frame.instruction_pointer >= frame.function->instructions.size()) {
        // Function finished
        VM_LOG_DEBUG("Function finished, popping call frame");
        call_stack_.pop_back();
        return std::monostate{};
    }

    // Fetch instruction
    Instruction instr = frame.function->instructions[frame.instruction_pointer++];
    OpCode opcode = backend::InstructionEncoder::decode_opcode(instr);

    VM_LOG_DEBUG("Executing instruction: {} (PC: {})",
                 backend::Disassembler::opcode_name(opcode),
                 frame.instruction_pointer - 1);

    // Execute instruction
    auto result = execute_instruction(opcode, instr);

    if (std::holds_alternative<ErrorCode>(result)) {
        VM_LOG_ERROR("Instruction execution failed: {}",
                     error_code_to_string(std::get<ErrorCode>(result)));
    }

    return result;
}

Result<std::vector<Value>> VirtualMachine::call(const Value& function,
                                                const std::vector<Value>& args) {
    if (!function.is_function()) {
        VM_LOG_ERROR("Attempt to call a {} value", function.type_name());
        trigger_runtime_error("attempt to call a non-function value");
    }

    auto function_result = function.to_function();
    if (is_error(function_result)) {
        VM_LOG_ERROR("Failed to extract function from value");
        return get_error(function_result);
    }
    auto function_ptr = get_value(function_result);

    if (function_ptr->isCFunction()) {
        VM_LOG_DEBUG("Calling C function with {} arguments", args.size());
        // No try-catch here. Let exceptions propagate to pcall/xpcall.
        auto result = function_ptr->call(this, args);
        if (state_ == VMState::Error) {
            // Handle non-throwing errors that just set state
            return last_error_;
        }
        return result;
    }

    if (function_ptr->isLuaFunction() || function_ptr->isClosure()) {
        VM_LOG_DEBUG("Calling Lua function/closure with {} arguments", args.size());
        return call_lua_function(function_ptr, args);
    }

    VM_LOG_ERROR("Unknown function type");
    return ErrorCode::TYPE_ERROR;
}

Result<std::vector<Value>> VirtualMachine::resume() {
    if (state_ != VMState::Suspended) {
        return ErrorCode::RUNTIME_ERROR;
    }

    state_ = VMState::Running;

    // Continue execution
    std::vector<Value> results;
    while (state_ == VMState::Running && !call_stack_.empty()) {
        auto step_result = step();
        if (std::holds_alternative<ErrorCode>(step_result)) {
            state_ = VMState::Error;
            return std::get<ErrorCode>(step_result);
        }
    }

    return results;
}

void VirtualMachine::suspend() {
    if (state_ == VMState::Running) {
        state_ = VMState::Suspended;
    }
}

void VirtualMachine::reset() {
    state_ = VMState::Ready;
    stack_.clear();
    call_stack_.clear();

    // Reset environment system
    registry_ = std::make_unique<Registry>();
    auto global_table = registry_->getGlobalTable();
    environment_ = std::make_unique<Environment>(global_table);

    stack_top_ = 0;
    last_error_ = ErrorCode::SUCCESS;
}

Size VirtualMachine::instruction_pointer() const noexcept {
    return call_stack_.empty() ? 0 : call_stack_.back().instruction_pointer;
}

const backend::BytecodeFunction* VirtualMachine::current_function() const noexcept {
    return call_stack_.empty() ? nullptr : call_stack_.back().function;
}

void VirtualMachine::push(Value value) {
    // Check for stack overflow before pushing
    if (stack_top_ >= config_.stack_size) {
        set_error(ErrorCode::STACK_OVERFLOW);
        return;
    }

    ensure_stack_size(stack_top_ + 1);
    if (stack_top_ < stack_.size()) {
        stack_[stack_top_] = std::move(value);
    } else {
        stack_.push_back(std::move(value));
    }
    ++stack_top_;
}

Value VirtualMachine::pop() {
    if (stack_top_ == 0) {
        return Value{};  // Return nil for empty stack
    }

    --stack_top_;
    return std::move(stack_[stack_top_]);
}

const Value& VirtualMachine::top() const {
    static Value nil_value;
    if (stack_top_ == 0) {
        return nil_value;
    }
    return stack_[stack_top_ - 1];
}

const Value& VirtualMachine::get_stack(Size index) const {
    static Value nil_value;
    if (index >= stack_top_) {
        return nil_value;
    }
    return stack_[index];
}

void VirtualMachine::set_stack(Size index, const Value& value) {
    ensure_stack_size(index + 1);
    if (index < stack_.size()) {
        stack_[index] = value;
        stack_top_ = std::max(stack_top_, index + 1);
    }
}

Value VirtualMachine::get_global(const String& name) const {
    if (!environment_) {
        return Value{};
    }
    return environment_->getGlobal(name);
}

void VirtualMachine::set_global(const String& name, Value value) {
    if (!environment_) {
        return;
    }
    environment_->setGlobal(name, std::move(value));
}

// IVMContext interface implementation
void VirtualMachine::set_instruction_pointer(Size ip) noexcept {
    if (!call_stack_.empty()) {
        call_stack_.back().instruction_pointer = ip;
    }
}

void VirtualMachine::adjust_instruction_pointer(std::int32_t offset) noexcept {
    if (!call_stack_.empty()) {
        call_stack_.back().instruction_pointer += offset;
    }
}

Value VirtualMachine::get_constant(std::uint16_t index) const {
    if (!call_stack_.empty()) {
        const auto& frame = call_stack_.back();

        // First try to get constants from closure (for user-defined functions)
        if (frame.closure) {
            const auto& constants = frame.closure->constants();
            if (index < constants.size()) {
                VM_LOG_DEBUG("Getting constant[{}] from closure", index);
                return constants[index];
            }
        }

        // Fallback to BytecodeFunction constants (for main chunk)
        if (frame.function) {
            const auto& constants = frame.function->constants;
            if (index < constants.size()) {
                const auto& constant = constants[index];

                // Convert ConstantValue to Value
                VM_LOG_DEBUG("Getting constant[{}] from BytecodeFunction", index);
                return std::visit(
                    [](const auto& val) -> Value {
                        using T = std::decay_t<decltype(val)>;
                        if constexpr (std::is_same_v<T, std::monostate>) {
                            return Value{};
                        } else if constexpr (std::is_same_v<T, bool>) {
                            return Value(val);
                        } else if constexpr (std::is_same_v<T, Number>) {
                            return Value(val);
                        } else if constexpr (std::is_same_v<T, Int>) {
                            return Value(val);
                        } else if constexpr (std::is_same_v<T, String>) {
                            return Value(val);
                        } else {
                            return Value{};
                        }
                    },
                    constant);
            }
        }
    }

    VM_LOG_ERROR("Invalid constant index: {}", index);
    return Value{};
}

Status VirtualMachine::call_function(const Value& function,
                                     const std::vector<Value>& args,
                                     std::vector<Value>& results) {
    try {
        if (!function.is_function()) {
            VM_LOG_ERROR("Attempt to call a {} value", function.type_name());
            return ErrorCode::TYPE_ERROR;
        }

        // Get the function pointer from the Value
        auto function_result = function.to_function();
        if (is_error(function_result)) {
            VM_LOG_ERROR("Failed to extract function from value");
            return get_error(function_result);
        }

        auto function_ptr = get_value(function_result);

        // Handle C functions directly
        if (function_ptr->isCFunction()) {
            VM_LOG_DEBUG("Calling C function with {} arguments", args.size());
            // Let exceptions from C functions (like error()) propagate up.
            // Special handling for tostring function to support metamethods
            if (is_tostring_function(function_ptr)) {
                results = call_tostring_with_metamethod(args);
            } else {
                results = function_ptr->call(this, args);
            }
            return std::monostate{};
        }

        // Handle Lua functions through VM execution
        if (function_ptr->isLuaFunction() || function_ptr->isClosure()) {
            VM_LOG_DEBUG("Calling Lua function/closure with {} arguments", args.size());
            auto call_result = call_lua_function(function_ptr, args);
            if (is_error(call_result)) {
                VM_LOG_ERROR("Lua function call failed");
                return get_error(call_result);
            }
            results = get_value(call_result);
            return std::monostate{};
        }

        VM_LOG_ERROR("Unknown function type");
        return ErrorCode::TYPE_ERROR;

    } catch (const RuntimeError& e) {
        // Re-throw RuntimeError so pcall/xpcall can handle it
        throw;
    } catch (const Exception& e) {
        VM_LOG_ERROR("Exception in function call: {}", e.what());
        return e.code();
    } catch (...) {
        VM_LOG_ERROR("Unknown error in function call");
        return ErrorCode::RUNTIME_ERROR;
    }
}

Value VirtualMachine::get_upvalue(UpvalueIndex index) const {
    if (!call_stack_.empty() && call_stack_.back().function) {
        const auto& frame = call_stack_.back();

        VM_LOG_DEBUG("GETUPVAL: frame.closure = {}, upvalue count = {}",
                     frame.closure ? "valid" : "null",
                     frame.closure ? frame.closure->upvalueCount() : 0);

        // Check if we have a closure with upvalues
        if (frame.closure && index < frame.closure->upvalueCount()) {
            auto upvalue = frame.closure->getUpvalue(index);
            if (upvalue) {
                VM_LOG_DEBUG(
                    "GETUPVAL: upvalue[{}] = {}", index, upvalue->getValue().debug_string());
                return upvalue->getValue();
            }
        }

        VM_LOG_DEBUG("GETUPVAL: upvalue[{}] not found, returning nil", index);
    }
    return Value{};
}

void VirtualMachine::set_upvalue(UpvalueIndex index, const Value& value) {
    if (!call_stack_.empty() && call_stack_.back().function) {
        const auto& frame = call_stack_.back();

        // Check if we have a closure with upvalues
        if (frame.closure && index < frame.closure->upvalueCount()) {
            auto upvalue = frame.closure->getUpvalue(index);
            if (upvalue) {
                upvalue->setValue(value);
                VM_LOG_DEBUG("SETUPVAL: upvalue[{}] = {}", index, value.debug_string());
                return;
            }
        }

        VM_LOG_DEBUG("SETUPVAL: upvalue[{}] not found, ignoring", index);
    } else {
        VM_LOG_DEBUG("SETUPVAL: No current function, ignoring");
    }
}

Status VirtualMachine::return_from_function(Size result_count) {
    if (call_stack_.empty()) {
        VM_LOG_ERROR("Cannot return from function: call stack is empty");
        return ErrorCode::RUNTIME_ERROR;
    }

    CallFrame& frame = call_stack_.back();
    Size current_base = frame.stack_base;

    VM_LOG_DEBUG("return_from_function: current_base={}, result_count={}, stack_top_={}",
                 current_base,
                 result_count,
                 stack_top_);

    // The return values are currently at the top of the stack
    // For RETURN instruction, the values start at the specified register
    // We need to save them in a temporary vector
    std::vector<Value> return_values;
    return_values.reserve(result_count);

    // The return values start at stack_top_ - result_count
    Size return_start = stack_top_ - result_count;
    for (Size i = 0; i < result_count; ++i) {
        if (return_start + i < stack_.size()) {
            return_values.push_back(std::move(stack_[return_start + i]));
            VM_LOG_DEBUG("Saved return value[{}]: {}", i, return_values[i].debug_string());
        }
    }

    // Pop the call frame
    call_stack_.pop_back();

    // Now place the return values at the current base
    for (Size i = 0; i < result_count && i < return_values.size(); ++i) {
        if (current_base + i < stack_.size()) {
            stack_[current_base + i] = std::move(return_values[i]);
            VM_LOG_DEBUG("Placed return value[{}] at stack[{}]: {}",
                         i,
                         current_base + i,
                         stack_[current_base + i].debug_string());
        }
    }

    // Adjust stack top to just after the return values
    stack_top_ = current_base + result_count;

    VM_LOG_DEBUG(
        "Returned from function with {} results, new stack_top_={}", result_count, stack_top_);
    return std::monostate{};
}

Status VirtualMachine::return_from_function(Register return_start, Size result_count) {
    if (call_stack_.empty()) {
        VM_LOG_ERROR("Cannot return from function: call stack is empty");
        return ErrorCode::RUNTIME_ERROR;
    }

    CallFrame& frame = call_stack_.back();
    Size current_base = frame.stack_base;

    VM_LOG_DEBUG("return_from_function: return_start={}, current_base={}, result_count={}, "
                 "stack_top_={}",
                 return_start,
                 current_base,
                 result_count,
                 stack_top_);

    // The return values start at the specified register (relative to current frame)
    std::vector<Value> return_values;
    return_values.reserve(result_count);

    // Convert relative register to absolute stack position
    Size absolute_start = current_base + return_start;
    for (Size i = 0; i < result_count; ++i) {
        if (absolute_start + i < stack_.size()) {
            return_values.push_back(std::move(stack_[absolute_start + i]));
            VM_LOG_DEBUG("Saved return value[{}]: {}", i, return_values[i].debug_string());
        }
    }

    // Pop the call frame
    call_stack_.pop_back();

    // Now place the return values at the current base
    for (Size i = 0; i < result_count && i < return_values.size(); ++i) {
        if (current_base + i < stack_.size()) {
            stack_[current_base + i] = std::move(return_values[i]);
            VM_LOG_DEBUG("Placed return value[{}] at stack[{}]: {}",
                         i,
                         current_base + i,
                         stack_[current_base + i].debug_string());
        }
    }

    // Adjust stack top to just after the return values
    stack_top_ = current_base + result_count;

    VM_LOG_DEBUG(
        "Returned from function with {} results, new stack_top_={}", result_count, stack_top_);
    return std::monostate{};
}

Result<std::vector<Value>> VirtualMachine::call_lua_function(GCPtr<Function> function,
                                                             const std::vector<Value>& args) {
    if (!function) {
        VM_LOG_ERROR("Null function pointer");
        return ErrorCode::RUNTIME_ERROR;
    }

    if (!function->isLuaFunction() && !function->isClosure()) {
        VM_LOG_ERROR("Function is not a Lua function or closure");
        return ErrorCode::TYPE_ERROR;
    }

    try {
        // Save the original stack top before setting up the function call
        // This will be where the return values should be placed
        Size function_call_base = stack_top_;

        // Create a heap-allocated bytecode function to ensure its lifetime.
        auto bytecode_func = std::make_unique<backend::BytecodeFunction>();
        bytecode_func->name = "lua_function";
        bytecode_func->parameter_count = function->parameterCount();
        bytecode_func->stack_size = 32;  // Increased stack size for safety
        bytecode_func->instructions = function->bytecode();
        bytecode_func->is_vararg = function->isVararg();
        bytecode_func->line_info = function->lineInfo();

        // Copy constants from the function to the bytecode function
        const auto& constants = function->constants();
        bytecode_func->constants.reserve(constants.size());
        for (const auto& constant : constants) {
            // Convert runtime Value to backend ConstantValue
            backend::ConstantValue backend_constant;
            if (constant.is_nil()) {
                backend_constant = std::monostate{};
            } else if (constant.is_boolean()) {
                auto bool_result = constant.to_boolean();
                if (!is_error(bool_result)) {
                    backend_constant = get_value(bool_result);
                }
            } else if (constant.is_number()) {
                auto num_result = constant.to_number();
                if (!is_error(num_result)) {
                    backend_constant = get_value(num_result);
                }
            } else if (constant.is_string()) {
                auto str_result = constant.to_string();
                if (!is_error(str_result)) {
                    backend_constant = get_value(str_result);
                }
            } else {
                // Fallback to nil for unsupported types
                backend_constant = std::monostate{};
            }
            bytecode_func->constants.push_back(backend_constant);
        }

        VM_LOG_DEBUG("Created bytecode function with {} constants and {} instructions",
                     bytecode_func->constants.size(),
                     bytecode_func->instructions.size());

        // Push arguments onto stack first, before setting up call frame
        function_call_base = stack_top_;
        VM_LOG_DEBUG("Before pushing args: function_call_base={}, stack_top_={}",
                     function_call_base,
                     stack_top_);

        for (Size i = 0; i < args.size(); ++i) {
            push(args[i]);
            VM_LOG_DEBUG(
                "Pushed arg[{}] = {} to stack[{}]", i, args[i].debug_string(), stack_top_ - 1);
        }

        VM_LOG_DEBUG(
            "Function call: function_call_base={}, stack_top_after_push={}, args.size()={}",
            function_call_base,
            stack_top_,
            args.size());

        // Setup call frame for the function, using the base where arguments start
        // The unique_ptr is moved, transferring ownership to the CallFrame
        auto setup_result =
            setup_call_frame(std::move(bytecode_func), function, args.size(), function_call_base);
        if (std::holds_alternative<ErrorCode>(setup_result)) {
            VM_LOG_ERROR("Failed to setup call frame for Lua function");
            return std::get<ErrorCode>(setup_result);
        }

        // Execute the function directly without calling execute() to avoid double setup
        state_ = VMState::Running;
        VM_LOG_DEBUG("VM state set to Running, starting execution loop");

        // Save the initial call stack size to know when our function returns
        Size initial_call_stack_size = call_stack_.size();

        // Main execution loop for this function
        [[maybe_unused]] size_t instruction_count = 0;
        while (state_ == VMState::Running && call_stack_.size() >= initial_call_stack_size) {
            auto step_result = step();
            instruction_count++;

            if (std::holds_alternative<ErrorCode>(step_result)) {
                VM_LOG_ERROR("Failed to execute Lua function");
                return std::get<ErrorCode>(step_result);
            }
        }

        // After loop, check if it terminated due to an error state
        if (state_ == VMState::Error) {
            return last_error_;
        }

        VM_LOG_DEBUG("Lua function execution completed after {} instructions",
                     instruction_count);

        // Collect results from stack
        std::vector<Value> results;

        // After function execution, the return values should be placed starting at the
        // call frame's stack_base (which is where return_from_function places them)
        Size result_count = 0;
        if (stack_top_ > function_call_base) {
            result_count = stack_top_ - function_call_base;
        }

        for (Size i = 0; i < result_count; ++i) {
            results.push_back(stack_[function_call_base + i]);
        }

        // If no results were collected, return a single nil value (Lua convention)
        if (results.empty()) {
            results.emplace_back();
        }

        return results;

    } catch (const RuntimeError& e) {
        // Re-throw RuntimeError so pcall/xpcall can handle it
        throw;
    } catch (const std::exception& e) {
        VM_LOG_ERROR("Exception in Lua function call: {}", e.what());
        return ErrorCode::RUNTIME_ERROR;
    }
}

// Private helper methods
void VirtualMachine::unwind_stack_to_protected_call() {
    while (!call_stack_.empty()) {
        if (call_stack_.back().is_protected_call) {
            // Found the protected call boundary
            return;
        }
        call_stack_.pop_back();
    }
}

String VirtualMachine::generate_stack_trace_string() const {
    std::stringstream ss;
    ss << "stack traceback:";
    for (auto it = call_stack_.rbegin(); it != call_stack_.rend(); ++it) {
        const auto& frame = *it;
        ss << "\n\t";
        if (frame.closure && frame.closure->isCFunction()) {
            ss << "[C-function]";
        } else if (frame.function) {
            if (!frame.function->source_name.empty()) {
                ss << frame.function->source_name;
                // TODO: Line info is not yet populated correctly by codegen
                if (frame.instruction_pointer > 0 &&
                    frame.instruction_pointer <= frame.function->line_info.size()) {
                    ss << ":" << frame.function->line_info[frame.instruction_pointer - 1];
                } else {
                    ss << ":?";  // Placeholder for line number
                }
            } else {
                ss << "[string \"...\"]";
            }
        } else {
            ss << "[C-function]";
        }

        if (frame.function && !frame.function->name.empty()) {
            ss << ": in function '" << frame.function->name << "'";
        } else if (frame.closure && frame.closure->isCFunction()) {
            ss << ": in a C function";
        } else {
            ss << ": in main chunk";
        }
    }
    return ss.str();
}
Status VirtualMachine::execute_instruction(OpCode opcode, Instruction instruction) {
    VM_LOG_DEBUG("Executing instruction {} using strategy pattern", static_cast<int>(opcode));

    if (!strategy_registry_) {
        VM_LOG_ERROR("Strategy registry not initialized");
        return ErrorCode::RUNTIME_ERROR;
    }

    // Use strategy pattern for instruction execution
    return strategy_registry_->execute_instruction(*this, opcode, instruction);
}

Value& VirtualMachine::stack_at(Register reg) {
    Size index = call_stack_.empty() ? reg : call_stack_.back().stack_base + reg;
    ensure_stack_size(index + 1);
    if (index >= stack_.size()) {
        stack_.resize(index + 1);
    }
    stack_top_ = std::max(stack_top_, index + 1);
    return stack_[index];
}

const Value& VirtualMachine::stack_at(Register reg) const {
    Size index = call_stack_.empty() ? reg : call_stack_.back().stack_base + reg;
    static Value nil_value;
    if (index >= stack_.size()) {
        return nil_value;
    }
    return stack_[index];
}

void VirtualMachine::ensure_stack_size(Size size) {
    if (size > config_.stack_size) {
        set_error(ErrorCode::STACK_OVERFLOW);
        return;
    }

    if (size > stack_.size()) {
        stack_.resize(size);
    }
}

Status VirtualMachine::setup_call_frame(
    std::unique_ptr<const backend::BytecodeFunction> function,
    GCPtr<Function> closure,
    Size arg_count,
    Size stack_base) {
    if (call_stack_.size() >= config_.call_stack_size) {
        return ErrorCode::STACK_OVERFLOW;
    }

    // Ensure we have enough stack space for the function
    Size required_stack = stack_top_ + function->stack_size;
    ensure_stack_size(required_stack);

    CallFrame frame;
    frame.function = function.get();      // Raw pointer for quick access
    frame.owned_function = std::move(function);  // Take ownership
    frame.closure = closure;
    frame.stack_base = stack_base;
    frame.local_count = frame.function->parameter_count;
    frame.parameter_count = frame.function->parameter_count;
    frame.argument_count = arg_count;
    frame.has_varargs = frame.function->is_vararg;
    frame.vararg_base = stack_base + frame.parameter_count;

    VM_LOG_DEBUG("CallFrame setup: function_stack_base={}, parameter_count={}, arg_count={}, "
                 "vararg_base={}, is_vararg={}",
                 frame.stack_base,
                 frame.parameter_count,
                 frame.argument_count,
                 frame.vararg_base,
                 frame.has_varargs);

    // For main chunks (top-level functions), create a closure with _ENV as upvalue[0]
    if (call_stack_.empty() && environment_) {
        if (!frame.closure) {
            // This is the main chunk - create a closure with _ENV upvalue
            // Create a minimal function with empty bytecode for the closure
            std::vector<Instruction> empty_bytecode;
            auto main_closure = makeGCObject<Function>(empty_bytecode, std::vector<Size>{}, 0);
            main_closure->setSource(frame.function->source_name);  // ADD THIS LINE
            main_closure->makeClosure();  // Convert to closure

            // Create _ENV upvalue pointing to the global table
            auto global_table = environment_->getGlobalTable();
            if (global_table) {
                Value env_value(global_table);
                auto env_upvalue = makeGCObject<Upvalue>(env_value);
                main_closure->addUpvalue(env_upvalue);

                VM_LOG_DEBUG(
                    "Created main chunk closure with _ENV upvalue pointing to global table");
            }
            frame.closure = main_closure;
        }
    }

    call_stack_.push_back(std::move(frame));

    // For non-varargs functions, initialize local variables beyond parameters to nil
    if (!frame.function->is_vararg) {
        for (Size i = arg_count; i < frame.function->parameter_count; ++i) {
            stack_at(i) = Value{};
        }
    }

    VM_LOG_DEBUG("Setup call frame: function={}, args={}, params={}, stack_base={}, varargs={}",
                 frame.function->name,
                 arg_count,
                 frame.function->parameter_count,
                 stack_base,
                 frame.function->is_vararg);

    return std::monostate{};
}

Status VirtualMachine::setup_call_frame(const backend::BytecodeFunction& function,
                                        Size arg_count) {
    // Use the old behavior: calculate stack_base from current stack_top_
    Size stack_base = stack_top_ - arg_count;
    auto func_owner = std::make_unique<backend::BytecodeFunction>(function);
    return setup_call_frame(std::move(func_owner), GCPtr<Function>{}, arg_count, stack_base);
}

void VirtualMachine::set_error(ErrorCode code) {
    last_error_ = code;
    state_ = VMState::Error;

    // Log error with enhanced debugging information
    log_error(code, "VM execution error");

    // Dump stack trace in debug mode
    if constexpr (config::DEBUG_ENABLED) {
        utils::Debug::dump_stack_trace();
    }
}

void VirtualMachine::set_runtime_error(const String& message) {
    trigger_runtime_error(message);
}

void VirtualMachine::trigger_runtime_error(const String& message) {
    VM_LOG_ERROR("Runtime error triggered: {}", message);

    String location_info;
    if (!call_stack_.empty()) {
        const auto& frame = call_stack_.back();
        std::string source_name;

        // Correctly get source name from closure if available
        if (frame.closure) {
            source_name = frame.closure->getSource();
        }
        if (source_name.empty() && frame.function) {
            source_name = frame.function->source_name;
        }

        if (!source_name.empty()) {
            location_info += source_name;
        } else {
            location_info += "[string \"...\"]";
        }
        if (frame.function && frame.instruction_pointer > 0 &&
            frame.instruction_pointer <= frame.function->line_info.size()) {
            location_info +=
                ":" + std::to_string(frame.function->line_info[frame.instruction_pointer - 1]);
        } else {
            location_info += ":?";
        }
        location_info += ": ";
    }

    String full_message = location_info + message;
    error_obj_ = Value(full_message);

    String stack_trace = generate_stack_trace_string();
    VM_LOG_ERROR("Full error: {}\n{}", full_message, stack_trace);

    state_ = VMState::Error;
    last_error_ = ErrorCode::RUNTIME_ERROR;

    // This will be caught by pcall/xpcall
    throw RuntimeError(full_message);
}

GCPtr<Table> VirtualMachine::get_global_table() const {
    if (!environment_) {
        return GCPtr<Table>{};
    }
    return environment_->getGlobalTable();
}

Registry* VirtualMachine::get_registry() const noexcept {
    return registry_.get();
}

const CallFrame* VirtualMachine::current_call_frame() const noexcept {
    return call_stack_.empty() ? nullptr : &call_stack_.back();
}

void VirtualMachine::set_stack_top(Size new_top) noexcept {
    if (new_top <= config_.stack_size) {
        stack_top_ = new_top;
        ensure_stack_size(new_top);
    }
}

// Legacy instruction implementations (deprecated - moved to strategy pattern)

// Upvalue management
Upvalue* VirtualMachine::find_upvalue(Value* stack_location) {
    // Search for existing upvalue pointing to this stack location
    Upvalue* current = open_upvalues_;
    Upvalue** prev = &open_upvalues_;

    while (current && current->getStackLocation() > stack_location) {
        prev = &current->next;
        current = current->next;
    }

    // If found, return it
    if (current && current->getStackLocation() == stack_location) {
        return current;
    }

    // Create new upvalue
    auto new_upvalue = new Upvalue(stack_location);
    new_upvalue->next = current;
    new_upvalue->previous = prev;
    *prev = new_upvalue;

    if (current) {
        current->previous = &new_upvalue->next;
    }

    return new_upvalue;
}

void VirtualMachine::close_upvalues(Value* level) {
    while (open_upvalues_ && open_upvalues_->getStackLocation() >= level) {
        Upvalue* upvalue = open_upvalues_;
        open_upvalues_ = upvalue->next;

        if (open_upvalues_) {
            open_upvalues_->previous = &open_upvalues_;
        }

        // Close the upvalue
        upvalue->close();
        upvalue->next = nullptr;
        upvalue->previous = nullptr;
    }
}

bool VirtualMachine::is_tostring_function(const GCPtr<Function>& function) const {
    // Check if this is the tostring function by comparing with the global tostring
    Value global_tostring = get_global("tostring");
    if (global_tostring.is_function()) {
        auto global_tostring_result = global_tostring.to_function();
        if (!is_error(global_tostring_result)) {
            auto global_function_ptr = get_value(global_tostring_result);
            return function.get() == global_function_ptr.get();
        }
    }
    return false;
}

std::vector<Value>
VirtualMachine::call_tostring_with_metamethod(const std::vector<Value>& args) {
    if (args.empty()) {
        return {Value("nil")};
    }

    const auto& value = args[0];

    // Handle basic types that don't need metamethods
    if (value.is_nil()) {
        return {Value("nil")};
    }
    if (value.is_boolean()) {
        return {Value(value.as_boolean() ? "true" : "false")};
    }
    if (value.is_string()) {
        return {value};
    }
    if (value.is_number()) {
        Number num = value.as_number();
        // Format number similar to Lua's default formatting
        if (num == static_cast<double>(static_cast<Int>(num))) {
            return {Value(std::to_string(static_cast<Int>(num)))};
        } else {
            std::string result = std::to_string(num);
            // Remove trailing zeros
            result.erase(result.find_last_not_of('0') + 1, std::string::npos);
            result.erase(result.find_last_not_of('.') + 1, std::string::npos);
            return {Value(result)};
        }
    }

    // For other types (table, function, userdata, thread), try __tostring metamethod
    // First check if there's a metamethod available
    Value metamethod = MetamethodSystem::get_metamethod(value, Metamethod::TOSTRING);
    if (!metamethod.is_nil()) {
        // Try to call the metamethod (this will only work for C functions without VM context)
        auto metamethod_result =
            MetamethodSystem::try_unary_metamethod(*this, value, Metamethod::TOSTRING);
        if (!is_error(metamethod_result)) {
            Value result = get_value(metamethod_result);
            if (result.is_string()) {
                return {result};
            }
        }
        // If metamethod call failed (likely because it's a Lua function),
        // we can't call it from here without VM context.
        // This is a limitation that should be addressed by calling tostring from VM level.
    }

    // Fallback to default representation
    if (value.is_table()) {
        return {Value("table")};
    }
    return {Value(value.debug_string())};
}

// ExecutionContext implementation
ExecutionContext::ExecutionContext(VirtualMachine& vm)
    : vm_(vm), saved_state_(VMState::Ready), saved_stack_top_(0), is_saved_(false) {}

void ExecutionContext::save_state() {
    if (!is_saved_) {
        saved_state_ = vm_.state_;
        saved_stack_ = vm_.stack_;
        // TODO: Fix this. CallFrame is not copyable anymore.
        // saved_call_stack_ = vm_.call_stack_;
        saved_stack_top_ = vm_.stack_top_;
        is_saved_ = true;

        VM_LOG_DEBUG("Execution context saved");
    }
}

void ExecutionContext::restore_state() {
    if (is_saved_) {
        vm_.state_ = saved_state_;
        vm_.stack_ = std::move(saved_stack_);
        // TODO: Fix this. CallFrame is not copyable anymore.
        // vm_.call_stack_ = std::move(saved_call_stack_);
        vm_.stack_top_ = saved_stack_top_;
        is_saved_ = false;

        VM_LOG_DEBUG("Execution context restored");
    }
}

bool ExecutionContext::is_valid() const noexcept {
    return is_saved_;
}

// VMDebugger implementation
VMDebugger::VMDebugger(VirtualMachine& vm) : vm_(vm) {}

void VMDebugger::set_breakpoint(Size instruction) {
    breakpoints_.insert(instruction);
    VM_LOG_DEBUG("Breakpoint set at instruction {}", instruction);
}

void VMDebugger::remove_breakpoint(Size instruction) {
    breakpoints_.erase(instruction);
    VM_LOG_DEBUG("Breakpoint removed from instruction {}", instruction);
}

Status VMDebugger::step_instruction() {
    is_debugging_ = true;
    auto result = vm_.step();
    is_debugging_ = false;
    return result;
}

Status VMDebugger::step_over() {
    // TODO: Implement step over (step without entering function calls)
    return step_instruction();
}

Status VMDebugger::step_into() {
    // TODO: Implement step into (step into function calls)
    return step_instruction();
}

Status VMDebugger::continue_execution() {
    is_debugging_ = true;

    while (vm_.is_running()) {
        Size current_ip = vm_.instruction_pointer();

        // Check for breakpoints
        if (breakpoints_.contains(current_ip)) {
            VM_LOG_DEBUG("Breakpoint hit at instruction {}", current_ip);
            break;
        }

        auto result = vm_.step();
        if (is_error(result)) {
            is_debugging_ = false;
            return result;
        }
    }

    is_debugging_ = false;
    return std::monostate{};
}

std::vector<String> VMDebugger::get_stack_trace() const {
    std::vector<String> trace;

    for (const auto& frame : vm_.call_stack_) {
        if (frame.function) {
            String frame_info = frame.function->name + " at instruction " +
                                std::to_string(frame.instruction_pointer);
            trace.push_back(std::move(frame_info));
        }
    }

    return trace;
}

std::unordered_map<String, Value> VMDebugger::get_locals() const {
    std::unordered_map<String, Value> locals;

    if (!vm_.call_stack_.empty()) {
        const auto& frame = vm_.call_stack_.back();
        if (frame.function) {
            // Get local variable names and values
            for (Size i = 0; i < frame.function->locals.size() && i < frame.local_count; ++i) {
                const auto& name = frame.function->locals[i];
                const auto& value = vm_.stack_at(i);
                locals[name] = value;
            }
        }
    }

    return locals;
}

} // namespace rangelua::runtime
