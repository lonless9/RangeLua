/**
 * @file vm.cpp
 * @brief Virtual machine implementation stub
 * @version 0.1.0
 */

#include <rangelua/core/error.hpp>
#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/vm.hpp>
#include <rangelua/runtime/vm/all_strategies.hpp>
#include <rangelua/utils/debug.hpp>
#include <rangelua/utils/logger.hpp>

#include <cstdint>

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

    Result<std::vector<Value>> VirtualMachine::execute(const backend::BytecodeFunction& function,
                                                       const std::vector<Value>& args) {
        VM_LOG_INFO("Starting execution of function: {}", function.name);
        VM_LOG_DEBUG("Function has {} instructions, {} constants",
                     function.instructions.size(),
                     function.constants.size());

        try {
            // Setup initial call frame
            auto setup_result = setup_call_frame(function, args.size());
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

        } catch (const Exception& e) {
            state_ = VMState::Error;
            return e.code();
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
                try {
                    auto result = function_ptr->call(args);
                    return result;
                } catch (const std::exception& e) {
                    VM_LOG_ERROR("C function call failed: {}", e.what());
                    return ErrorCode::RUNTIME_ERROR;
                }
            }

            // Handle Lua functions through VM execution
            if (function_ptr->isLuaFunction() || function_ptr->isClosure()) {
                VM_LOG_DEBUG("Calling Lua function/closure with {} arguments", args.size());
                return call_lua_function(function_ptr, args);
            }

            VM_LOG_ERROR("Unknown function type");
            return ErrorCode::TYPE_ERROR;

        } catch (const Exception& e) {
            VM_LOG_ERROR("Exception in function call: {}", e.what());
            return e.code();
        } catch (...) {
            VM_LOG_ERROR("Unknown error in function call");
            return ErrorCode::RUNTIME_ERROR;
        }
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

    void VirtualMachine::set_stack(Size index, Value value) {
        ensure_stack_size(index + 1);
        if (index < stack_.size()) {
            stack_[index] = std::move(value);
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
                try {
                    results = function_ptr->call(args);
                    return std::monostate{};
                } catch (const std::exception& e) {
                    VM_LOG_ERROR("C function call failed: {}", e.what());
                    return ErrorCode::RUNTIME_ERROR;
                }
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
            Size original_stack_top = stack_top_;

            // For now, we'll create a simple bytecode function wrapper
            // In a full implementation, this would integrate with the bytecode system
            backend::BytecodeFunction bytecode_func;
            bytecode_func.name = "lua_function";
            bytecode_func.parameter_count = function->parameterCount();
            bytecode_func.stack_size = 16;  // Default stack size
            bytecode_func.instructions = function->bytecode();
            bytecode_func.is_vararg = function->isVararg();  // Copy vararg flag from Function

            // Setup call frame for the function
            auto setup_result = setup_call_frame(bytecode_func, args.size());
            if (std::holds_alternative<ErrorCode>(setup_result)) {
                VM_LOG_ERROR("Failed to setup call frame for Lua function");
                return std::get<ErrorCode>(setup_result);
            }

            // Set the closure in the call frame so constants and upvalues can be accessed
            if (!call_stack_.empty()) {
                call_stack_.back().closure = function;
                VM_LOG_DEBUG("Setting up closure with {} upvalues", function->upvalueCount());
            }

            // Push arguments onto stack
            for (const auto& arg : args) {
                push(arg);
            }

            // If this is a closure, setup upvalues
            if (function->isClosure()) {
                VM_LOG_DEBUG("Setting up closure with {} upvalues", function->upvalueCount());
                // The upvalues would be handled by the closure creation process
                // For now, we'll just log this
            }

            // Execute the function directly without calling execute() to avoid double setup
            state_ = VMState::Running;
            VM_LOG_DEBUG("VM state set to Running, starting execution loop");

            // Save the initial call stack size to know when our function returns
            Size initial_call_stack_size = call_stack_.size();

            // Main execution loop for this function
            size_t instruction_count = 0;
            while (state_ == VMState::Running && call_stack_.size() >= initial_call_stack_size) {
                auto step_result = step();
                instruction_count++;

                if (std::holds_alternative<ErrorCode>(step_result)) {
                    VM_LOG_ERROR("Failed to execute Lua function");
                    return std::get<ErrorCode>(step_result);
                }
            }

            VM_LOG_DEBUG("Lua function execution completed after {} instructions",
                         instruction_count);

            // Collect results from stack
            std::vector<Value> results;

            // After return_from_function, the return values should be at the function's original
            // stack base The function was called with a specific stack base, and
            // return_from_function places the results starting at that base
            Size function_base =
                original_stack_top;  // This was the base where the function started

            // The return_from_function method should have placed the results starting at
            // function_base and adjusted stack_top_ to function_base + result_count
            // Check for underflow to prevent infinite loops
            Size result_count = 0;
            if (stack_top_ >= function_base) {
                result_count = stack_top_ - function_base;
            } else {
                VM_LOG_ERROR("Stack underflow detected: stack_top_={}, function_base={}",
                             stack_top_,
                             function_base);
                // Assume 1 result (the function should return at least one value)
                result_count = 1;
            }

            VM_LOG_DEBUG(
                "Collecting {} return values from function_base {}", result_count, function_base);

            for (Size i = 0; i < result_count; ++i) {
                if (function_base + i < stack_.size()) {
                    results.push_back(stack_[function_base + i]);
                    VM_LOG_DEBUG("Collected return value[{}]: {}", i, results[i].debug_string());
                }
            }

            return results;

        } catch (const std::exception& e) {
            VM_LOG_ERROR("Exception in Lua function call: {}", e.what());
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    // Private helper methods
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

    Status VirtualMachine::setup_call_frame(const backend::BytecodeFunction& function,
                                            Size arg_count) {
        if (call_stack_.size() >= config_.call_stack_size) {
            return ErrorCode::STACK_OVERFLOW;
        }

        // Ensure we have enough stack space for the function
        Size required_stack = stack_top_ + function.stack_size;
        ensure_stack_size(required_stack);

        // Create call frame with enhanced vararg support
        CallFrame frame(&function,
                        stack_top_ - arg_count,
                        function.parameter_count,
                        function.parameter_count,
                        arg_count,
                        function.is_vararg);

        VM_LOG_DEBUG("Function is_vararg flag: {}", function.is_vararg);

        // For main chunks (top-level functions), create a closure with _ENV as upvalue[0]
        if (call_stack_.empty() && environment_) {
            // This is the main chunk - create a closure with _ENV upvalue
            // Create a minimal function with empty bytecode for the closure
            std::vector<Instruction> empty_bytecode;
            auto main_closure = makeGCObject<Function>(empty_bytecode, 0);
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

        call_stack_.push_back(frame);

        // Initialize local variables beyond parameters to nil
        for (Size i = arg_count; i < function.parameter_count; ++i) {
            stack_at(i) = Value{};
        }

        VM_LOG_DEBUG("Setup call frame: function={}, args={}, params={}, stack_base={}, varargs={}",
                     function.name,
                     arg_count,
                     function.parameter_count,
                     frame.stack_base,
                     function.is_vararg);

        // If this is a vararg function, emit VARARGPREP instruction handling
        if (function.is_vararg) {
            VM_LOG_DEBUG("Function has varargs: {} extra args available", frame.vararg_count());
        }

        return std::monostate{};
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
        VM_LOG_ERROR("Runtime error: {}", message);

        // Create detailed runtime error with source location
        RuntimeError error(message);
        log_error(error);

        set_error(ErrorCode::RUNTIME_ERROR);

        // Additional debug information
        // RANGELUA_DEBUG_PRINT("VM State: " + std::to_string(static_cast<int>(state_)));
        // RANGELUA_DEBUG_PRINT("Stack size: " + std::to_string(stack_.size()));
        // RANGELUA_DEBUG_PRINT("Call stack depth: " + std::to_string(call_stack_.size()));
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

    void VirtualMachine::trigger_runtime_error(const String& message) {
        set_runtime_error(message);
    }

    const CallFrame* VirtualMachine::current_call_frame() const noexcept {
        return call_stack_.empty() ? nullptr : &call_stack_.back();
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

    // ExecutionContext implementation
    ExecutionContext::ExecutionContext(VirtualMachine& vm)
        : vm_(vm), saved_state_(VMState::Ready), saved_stack_top_(0), is_saved_(false) {}

    void ExecutionContext::save_state() {
        if (!is_saved_) {
            saved_state_ = vm_.state_;
            saved_stack_ = vm_.stack_;
            saved_call_stack_ = vm_.call_stack_;
            saved_stack_top_ = vm_.stack_top_;
            is_saved_ = true;

            VM_LOG_DEBUG("Execution context saved");
        }
    }

    void ExecutionContext::restore_state() {
        if (is_saved_) {
            vm_.state_ = saved_state_;
            vm_.stack_ = std::move(saved_stack_);
            vm_.call_stack_ = std::move(saved_call_stack_);
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
                    const String& name = frame.function->locals[i];
                    const Value& value = vm_.stack_at(i);
                    locals[name] = value;
                }
            }
        }

        return locals;
    }

}  // namespace rangelua::runtime
