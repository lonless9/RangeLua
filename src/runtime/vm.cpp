/**
 * @file vm.cpp
 * @brief Virtual machine implementation stub
 * @version 0.1.0
 */

#include <rangelua/core/error.hpp>
#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/vm.hpp>
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

        stack_.reserve(config_.stack_size);
        call_stack_.reserve(config_.call_stack_size);
    }

    VirtualMachine::VirtualMachine(RuntimeMemoryManager& memory_manager, VMConfig config)
        : config_(config), memory_manager_(&memory_manager) {
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

            // Use the Value's call method which handles different function types
            auto call_result = function.call(args);
            if (is_error(call_result)) {
                VM_LOG_ERROR("Function call failed");
                return get_error(call_result);
            }

            return get_value(call_result);
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
        globals_.clear();
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
        auto it = globals_.find(name);
        return it != globals_.end() ? it->second : Value{};
    }

    void VirtualMachine::set_global(const String& name, Value value) {
        globals_[name] = std::move(value);
    }

    // Private helper methods
    Status VirtualMachine::execute_instruction(OpCode opcode, Instruction instruction) {
        using namespace backend;

        Register a = InstructionEncoder::decode_a(instruction);

        switch (opcode) {
            case OpCode::OP_MOVE: {
                Register b = InstructionEncoder::decode_b(instruction);
                return op_move(a, b);
            }

            case OpCode::OP_LOADI: {
                std::int32_t sbx = InstructionEncoder::decode_sbx(instruction);
                return op_loadi(a, static_cast<std::int16_t>(sbx));
            }

            case OpCode::OP_LOADF: {
                std::int32_t sbx = InstructionEncoder::decode_sbx(instruction);
                return op_loadf(a, static_cast<std::int16_t>(sbx));
            }

            case OpCode::OP_LOADK: {
                std::uint32_t bx = InstructionEncoder::decode_bx(instruction);
                return op_loadk(a, static_cast<std::uint16_t>(bx));
            }

            case OpCode::OP_LOADNIL: {
                Register b = InstructionEncoder::decode_b(instruction);
                return op_loadnil(a, b);
            }

            case OpCode::OP_LOADTRUE:
                return op_loadtrue(a);

            case OpCode::OP_LOADFALSE:
                return op_loadfalse(a);

            case OpCode::OP_ADD: {
                Register b = InstructionEncoder::decode_b(instruction);
                Register c = InstructionEncoder::decode_c(instruction);
                return op_add(a, b, c);
            }

            case OpCode::OP_JMP: {
                std::int32_t sbx = InstructionEncoder::decode_sbx(instruction);
                return op_jmp(static_cast<std::int16_t>(sbx));
            }

            case OpCode::OP_RETURN: {
                Register b = InstructionEncoder::decode_b(instruction);
                return op_return(a, b);
            }

            case OpCode::OP_NEWTABLE:
                return op_newtable(a);

            case OpCode::OP_GETTABLE: {
                Register b = InstructionEncoder::decode_b(instruction);
                Register c = InstructionEncoder::decode_c(instruction);
                return op_gettable(a, b, c);
            }

            case OpCode::OP_SETTABLE: {
                Register b = InstructionEncoder::decode_b(instruction);
                Register c = InstructionEncoder::decode_c(instruction);
                return op_settable(a, b, c);
            }

            case OpCode::OP_GETTABUP: {
                Register b = InstructionEncoder::decode_b(instruction);
                Register c = InstructionEncoder::decode_c(instruction);
                return op_gettabup(a, b, c);
            }

            case OpCode::OP_CLOSURE: {
                std::uint32_t bx = InstructionEncoder::decode_bx(instruction);
                return op_closure(a, static_cast<std::uint16_t>(bx));
            }

            case OpCode::OP_GETUPVAL: {
                Register b = InstructionEncoder::decode_b(instruction);
                return op_getupval(a, static_cast<UpvalueIndex>(b));
            }

            case OpCode::OP_SETUPVAL: {
                Register b = InstructionEncoder::decode_b(instruction);
                return op_setupval(a, static_cast<UpvalueIndex>(b));
            }

            case OpCode::OP_CALL: {
                Register b = InstructionEncoder::decode_b(instruction);
                Register c = InstructionEncoder::decode_c(instruction);
                return op_call(a, b, c);
            }

            default:
                VM_LOG_ERROR("Unimplemented opcode: {}", static_cast<int>(opcode));
                return ErrorCode::RUNTIME_ERROR;
        }
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

        // Create call frame with proper stack base
        CallFrame frame(&function, stack_top_ - arg_count, function.parameter_count);
        call_stack_.push_back(frame);

        // Initialize local variables beyond parameters to nil
        for (Size i = arg_count; i < function.parameter_count; ++i) {
            stack_at(i) = Value{};
        }

        VM_LOG_DEBUG("Setup call frame: function={}, args={}, params={}, stack_base={}",
                     function.name,
                     arg_count,
                     function.parameter_count,
                     frame.stack_base);

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

    void VirtualMachine::trigger_runtime_error(const String& message) {
        set_runtime_error(message);
    }

    // Instruction implementations (stubs)
    Status VirtualMachine::op_move(Register a, Register b) {
        stack_at(a) = stack_at(b);
        return std::monostate{};
    }

    Status VirtualMachine::op_loadi(Register a, std::int16_t sbx) {
        stack_at(a) = Value(static_cast<Int>(sbx));
        return std::monostate{};
    }

    Status VirtualMachine::op_loadf(Register a, std::int16_t sbx) {
        stack_at(a) = Value(static_cast<Number>(sbx));
        return std::monostate{};
    }

    Status VirtualMachine::op_loadk(Register a, std::uint16_t bx) {
        if (!call_stack_.empty() && call_stack_.back().function) {
            const auto& constants = call_stack_.back().function->constants;
            if (bx < constants.size()) {
                const auto& constant = constants[bx];

                // Convert ConstantValue to Value
                Value value = std::visit(
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

                stack_at(a) = std::move(value);
                return std::monostate{};
            }
        }

        VM_LOG_ERROR("Invalid constant index: {}", bx);
        return ErrorCode::RUNTIME_ERROR;
    }

    Status VirtualMachine::op_loadnil(Register a, Register b) {
        for (Register i = a; i <= b; ++i) {
            stack_at(i) = Value{};
        }
        return std::monostate{};
    }

    Status VirtualMachine::op_loadtrue(Register a) {
        stack_at(a) = Value(true);
        return std::monostate{};
    }

    Status VirtualMachine::op_loadfalse(Register a) {
        stack_at(a) = Value(false);
        return std::monostate{};
    }

    Status VirtualMachine::op_add(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);

            // Use Value's operator+ which handles Lua semantics
            Value result = left + right;

            // Check if the operation resulted in nil (error case)
            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid arithmetic operation: cannot add {} and {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            VM_LOG_ERROR("Arithmetic error in ADD: {}", e.what());
            return e.code();
        } catch (...) {
            VM_LOG_ERROR("Unknown error in ADD operation");
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_jmp(std::int16_t sbx) {
        if (!call_stack_.empty()) {
            call_stack_.back().instruction_pointer += sbx;
        }
        return std::monostate{};
    }

    Status VirtualMachine::op_return(Register a, Register b) {
        if (call_stack_.empty()) {
            VM_LOG_ERROR("RETURN instruction with empty call stack");
            return ErrorCode::RUNTIME_ERROR;
        }

        auto& frame = call_stack_.back();
        Size return_count = (b == 0) ? (stack_top_ - frame.stack_base - a) : (b - 1);

        // Copy return values to the correct position
        if (return_count > 0) {
            // Move return values to start of current frame
            for (Size i = 0; i < return_count; ++i) {
                if (frame.stack_base + i < stack_.size() &&
                    frame.stack_base + a + i < stack_.size()) {
                    stack_[frame.stack_base + i] = std::move(stack_[frame.stack_base + a + i]);
                }
            }
        }

        // Restore stack top to the beginning of this frame plus return values
        stack_top_ = frame.stack_base + return_count;

        // Pop the call frame
        call_stack_.pop_back();

        VM_LOG_DEBUG("RETURN: {} values returned", return_count);
        return std::monostate{};
    }

    // Arithmetic operations implementation
    Status VirtualMachine::op_sub(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);
            Value result = left - right;

            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid arithmetic operation: cannot subtract {} and {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_mul(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);
            Value result = left * right;

            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid arithmetic operation: cannot multiply {} and {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_div(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);
            Value result = left / right;

            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid arithmetic operation: cannot divide {} and {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_mod(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);
            Value result = left % right;

            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid arithmetic operation: cannot modulo {} and {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_pow(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);
            Value result = left ^ right;  // Exponentiation

            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid arithmetic operation: cannot exponentiate {} and {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_unm(Register a, Register b) {
        try {
            const Value& operand = stack_at(b);
            Value result = -operand;  // Unary minus

            if (result.is_nil() && !operand.is_nil()) {
                VM_LOG_ERROR("Invalid arithmetic operation: cannot negate {}", operand.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }
    // Bitwise operations implementation
    Status VirtualMachine::op_band(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);
            Value result = left & right;

            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid bitwise operation: cannot AND {} and {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_bor(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);
            Value result = left | right;

            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid bitwise operation: cannot OR {} and {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_bxor(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);
            // XOR using ^ operator (note: this is different from exponentiation context)
            Value result = left | right;  // TODO: Implement proper XOR in Value class

            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid bitwise operation: cannot XOR {} and {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_shl(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);
            Value result = left << right;

            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid bitwise operation: cannot shift left {} by {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_shr(Register a, Register b, Register c) {
        try {
            const Value& left = stack_at(b);
            const Value& right = stack_at(c);
            Value result = left >> right;

            if (result.is_nil() && (!left.is_nil() || !right.is_nil())) {
                VM_LOG_ERROR("Invalid bitwise operation: cannot shift right {} by {}",
                             left.type_name(),
                             right.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_bnot(Register a, Register b) {
        try {
            const Value& operand = stack_at(b);
            Value result = ~operand;  // Bitwise NOT

            if (result.is_nil() && !operand.is_nil()) {
                VM_LOG_ERROR("Invalid bitwise operation: cannot NOT {}", operand.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }
    // Comparison operations implementation
    Status VirtualMachine::op_eq(Register a, Register b, std::int16_t sbx) {
        try {
            const Value& left = stack_at(a);
            const Value& right = stack_at(b);
            bool result = (left == right);

            // If sbx is non-zero, we want to jump if the comparison is false
            if ((sbx != 0) == result) {
                // Skip next instruction (which is typically a jump)
                if (!call_stack_.empty()) {
                    call_stack_.back().instruction_pointer++;
                }
            }

            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_lt(Register a, Register b, std::int16_t sbx) {
        try {
            const Value& left = stack_at(a);
            const Value& right = stack_at(b);
            bool result = (left < right);

            // If sbx is non-zero, we want to jump if the comparison is false
            if ((sbx != 0) == result) {
                // Skip next instruction (which is typically a jump)
                if (!call_stack_.empty()) {
                    call_stack_.back().instruction_pointer++;
                }
            }

            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_le(Register a, Register b, std::int16_t sbx) {
        try {
            const Value& left = stack_at(a);
            const Value& right = stack_at(b);
            bool result = (left <= right);

            // If sbx is non-zero, we want to jump if the comparison is false
            if ((sbx != 0) == result) {
                // Skip next instruction (which is typically a jump)
                if (!call_stack_.empty()) {
                    call_stack_.back().instruction_pointer++;
                }
            }

            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_not(Register a, Register b) {
        try {
            const Value& operand = stack_at(b);
            // Lua NOT: nil and false are falsy, everything else is truthy
            bool result = operand.is_falsy();
            stack_at(a) = Value(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_len(Register a, Register b) {
        try {
            const Value& operand = stack_at(b);
            Value result = operand.length();

            if (result.is_nil()) {
                VM_LOG_ERROR("Invalid length operation on {}", operand.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }
    // Table operations implementation
    Status VirtualMachine::op_gettable(Register a, Register b, Register c) {
        try {
            const Value& table = stack_at(b);
            const Value& key = stack_at(c);

            if (!table.is_table()) {
                VM_LOG_ERROR("Attempt to index a {} value", table.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            Value result = table.get(key);
            stack_at(a) = std::move(result);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_settable(Register a, Register b, Register c) {
        try {
            Value& table = stack_at(a);
            const Value& key = stack_at(b);
            const Value& value = stack_at(c);

            if (!table.is_table()) {
                VM_LOG_ERROR("Attempt to index a {} value", table.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            table.set(key, value);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_newtable(Register a) {
        try {
            // Create new table using value factory
            Value table = value_factory::table();
            stack_at(a) = std::move(table);
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_gettabup(Register a, Register b, Register c) {
        try {
            VM_LOG_DEBUG("GETTABUP: R[{}] = UpValue[{}][K[{}]]", a, b, c);

            // For now, implement as global variable access
            // In a full implementation, this would access upvalues
            // but for our simple case, we'll treat it as global access

            if (!call_stack_.empty() && call_stack_.back().function) {
                const auto& constants = call_stack_.back().function->constants;
                if (c < constants.size()) {
                    const auto& constant = constants[c];

                    // The constant should be a string (global name)
                    if (std::holds_alternative<String>(constant)) {
                        const String& name = std::get<String>(constant);
                        Value value = get_global(name);
                        stack_at(a) = std::move(value);
                        return std::monostate{};
                    }
                }
            }

            VM_LOG_ERROR("Invalid constant index in GETTABUP: {}", c);
            return ErrorCode::RUNTIME_ERROR;
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }
    // Function call operations implementation
    Status VirtualMachine::op_call(Register a, Register b, Register c) {
        try {
            const Value& function = stack_at(a);

            if (!function.is_function()) {
                VM_LOG_ERROR("Attempt to call a {} value", function.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            // Prepare arguments
            std::vector<Value> args;
            Size arg_count = (b == 0) ? (stack_top_ - a - 1) : (b - 1);

            args.reserve(arg_count);
            for (Size i = 0; i < arg_count; ++i) {
                args.push_back(stack_at(a + 1 + i));
            }

            // Call the function using Value's call method
            auto call_result = function.call(args);
            if (is_error(call_result)) {
                VM_LOG_ERROR("Function call failed");
                return get_error(call_result);
            }

            auto results = get_value(call_result);
            Size result_count = (c == 0) ? results.size() : (c - 1);

            // Store results
            for (Size i = 0; i < result_count && i < results.size(); ++i) {
                stack_at(a + i) = std::move(results[i]);
            }

            // Fill remaining slots with nil if needed
            for (Size i = results.size(); i < result_count; ++i) {
                stack_at(a + i) = Value{};
            }

            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_tailcall(Register a, Register b) {
        try {
            const Value& function = stack_at(a);

            if (!function.is_function()) {
                VM_LOG_ERROR("Attempt to tail call a {} value", function.type_name());
                return ErrorCode::TYPE_ERROR;
            }

            // Prepare arguments
            std::vector<Value> args;
            Size arg_count = (b == 0) ? (stack_top_ - a - 1) : (b - 1);

            args.reserve(arg_count);
            for (Size i = 0; i < arg_count; ++i) {
                args.push_back(stack_at(a + 1 + i));
            }

            // For tail call, we replace the current frame
            if (!call_stack_.empty()) {
                auto& current_frame = call_stack_.back();
                Size base = current_frame.stack_base;

                // Call the function
                auto call_result = function.call(args);
                if (is_error(call_result)) {
                    VM_LOG_ERROR("Tail call failed");
                    return get_error(call_result);
                }

                auto results = get_value(call_result);

                // Move results to the base of current frame
                for (Size i = 0; i < results.size(); ++i) {
                    stack_at(base + i) = std::move(results[i]);
                }

                // Adjust stack top
                stack_top_ = base + results.size();

                // Pop current frame (tail call optimization)
                call_stack_.pop_back();
            }

            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }
    // Control flow and test operations implementation
    Status VirtualMachine::op_test(Register a, Register c) {
        try {
            const Value& value = stack_at(a);
            bool is_truthy = value.is_truthy();

            // If c is non-zero, we want to jump if the value is falsy
            // If c is zero, we want to jump if the value is truthy
            if ((c != 0) != is_truthy) {
                // Skip next instruction (which is typically a jump)
                if (!call_stack_.empty()) {
                    call_stack_.back().instruction_pointer++;
                }
            }

            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_testset(Register a, Register b, Register c) {
        try {
            const Value& value = stack_at(b);
            bool is_truthy = value.is_truthy();

            // If c is non-zero, we want to jump if the value is falsy
            // If c is zero, we want to jump if the value is truthy
            if ((c != 0) != is_truthy) {
                // Skip next instruction and don't set the value
                if (!call_stack_.empty()) {
                    call_stack_.back().instruction_pointer++;
                }
            } else {
                // Set the value
                stack_at(a) = value;
            }

            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    // Upvalue operations
    Status VirtualMachine::op_getupval(Register a, UpvalueIndex b) {
        try {
            if (!call_stack_.empty() && call_stack_.back().function) {
                // Get the current function's upvalues
                // This would need to be implemented when we have proper closure support
                // For now, just set to nil
                stack_at(a) = Value{};
                VM_LOG_DEBUG("GETUPVAL: R{} = upvalue[{}]", a, b);
            } else {
                stack_at(a) = Value{};
                VM_LOG_DEBUG("GETUPVAL: No current function, setting R{} to nil", a);
            }
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_setupval(Register a, UpvalueIndex b) {
        try {
            if (!call_stack_.empty() && call_stack_.back().function) {
                // Set the current function's upvalue
                // This would need to be implemented when we have proper closure support
                [[maybe_unused]] const Value& value = stack_at(a);
                VM_LOG_DEBUG("SETUPVAL: upvalue[{}] = R{}", b, a);
            } else {
                VM_LOG_DEBUG("SETUPVAL: No current function, ignoring");
            }
            return std::monostate{};
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    // Global variable operations
    Status VirtualMachine::op_getglobal(Register a, std::uint16_t bx) {
        try {
            if (!call_stack_.empty() && call_stack_.back().function) {
                const auto& constants = call_stack_.back().function->constants;
                if (bx < constants.size()) {
                    const auto& constant = constants[bx];

                    // The constant should be a string (global name)
                    if (std::holds_alternative<String>(constant)) {
                        const String& name = std::get<String>(constant);
                        Value value = get_global(name);
                        stack_at(a) = std::move(value);
                        return std::monostate{};
                    }
                }
            }

            VM_LOG_ERROR("Invalid global constant index: {}", bx);
            return ErrorCode::RUNTIME_ERROR;
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_setglobal(Register a, std::uint16_t bx) {
        try {
            if (!call_stack_.empty() && call_stack_.back().function) {
                const auto& constants = call_stack_.back().function->constants;
                if (bx < constants.size()) {
                    const auto& constant = constants[bx];

                    // The constant should be a string (global name)
                    if (std::holds_alternative<String>(constant)) {
                        const String& name = std::get<String>(constant);
                        const Value& value = stack_at(a);
                        set_global(name, value);
                        return std::monostate{};
                    }
                }
            }

            VM_LOG_ERROR("Invalid global constant index: {}", bx);
            return ErrorCode::RUNTIME_ERROR;
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

    Status VirtualMachine::op_closure(Register a, std::uint16_t bx) {
        try {
            if (!call_stack_.empty() && call_stack_.back().function) {
                const auto& constants = call_stack_.back().function->constants;
                if (bx < constants.size()) {
                    // For now, just create a simple function value
                    // In a full implementation, this would create a closure with upvalues
                    stack_at(a) = Value{};  // Placeholder
                    VM_LOG_DEBUG("CLOSURE: R{} = closure(K[{}]) (not fully implemented)", a, bx);
                    return std::monostate{};
                }
            }

            VM_LOG_ERROR("Invalid closure constant index: {}", bx);
            return ErrorCode::RUNTIME_ERROR;
        } catch (const Exception& e) {
            return e.code();
        } catch (...) {
            return ErrorCode::RUNTIME_ERROR;
        }
    }

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
