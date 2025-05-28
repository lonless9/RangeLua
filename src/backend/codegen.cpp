/**
 * @file codegen.cpp
 * @brief Code generator implementation stub
 * @version 0.1.0
 */

#include <rangelua/backend/codegen.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::backend {

    // RegisterAllocator implementation (Lua 5.5 style)
    RegisterAllocator::RegisterAllocator(Size max_registers)
        : max_registers_(max_registers), free_reg_(0), max_stack_size_(0), local_count_(0) {}

    Register RegisterAllocator::reserve_registers(Size n) {
        check_stack(n);
        Register start = free_reg_;
        free_reg_ += static_cast<Register>(n);
        return start;
    }

    void RegisterAllocator::free_register(Register reg, Size local_count) {
        // Only free registers that are not local variables
        if (reg >= local_count && reg == free_reg_ - 1) {
            free_reg_--;
        }
    }

    void RegisterAllocator::free_registers(Register r1, Register r2, Size local_count) {
        // Free registers in reverse order (higher register first)
        if (r1 > r2) {
            free_register(r1, local_count);
            free_register(r2, local_count);
        } else {
            free_register(r2, local_count);
            free_register(r1, local_count);
        }
    }

    void RegisterAllocator::check_stack(Size needed) {
        Register new_stack = free_reg_ + static_cast<Register>(needed);
        if (new_stack > max_registers_) {
            // This should be an error in a real implementation
            CODEGEN_LOG_ERROR("Register stack overflow: need {}, max {}", new_stack, max_registers_);
            return;
        }
        if (new_stack > max_stack_size_) {
            max_stack_size_ = new_stack;
        }
    }

    void RegisterAllocator::reset() {
        free_reg_ = 0;
        max_stack_size_ = 0;
        local_count_ = 0;
    }

    // Temporary compatibility methods for migration
    Result<Register> RegisterAllocator::allocate() {
        Register reg = reserve_registers(1);
        return reg;
    }

    void RegisterAllocator::free(Register reg) {
        free_register(reg, local_count_);
    }

    Register RegisterAllocator::high_water_mark() const noexcept {
        return max_stack_size_;
    }

    // Expression management methods (Lua 5.5 style)
    void CodeGenerator::discharge_vars(ExpressionDesc& expr) {
        switch (expr.kind) {
            case ExpressionKind::LOCAL: {
                // Local variable - already in register
                expr.kind = ExpressionKind::NONRELOC;
                // expr.u.info already contains the register
                break;
            }
            case ExpressionKind::UPVAL: {
                // Upvalue - emit GETUPVAL instruction
                Register reg = register_allocator_.reserve_registers(1);
                emitter_.emit_abc(OpCode::OP_GETUPVAL, reg, static_cast<Register>(expr.u.info), 0);
                expr.kind = ExpressionKind::RELOC;
                expr.u.info = emitter_.instruction_count() - 1;
                break;
            }
            case ExpressionKind::GLOBAL: {
                // Global variable - emit GETTABUP instruction
                Register reg = register_allocator_.reserve_registers(1);
                CODEGEN_LOG_DEBUG(
                    "Emitting GETTABUP for global: result=R[{}], upvalue=0, key=K[{}]",
                    reg,
                    expr.u.info);
                emitter_.emit_abc(OpCode::OP_GETTABUP, reg, 0, static_cast<Register>(expr.u.info));
                expr.kind = ExpressionKind::RELOC;
                expr.u.info = emitter_.instruction_count() - 1;
                break;
            }
            case ExpressionKind::INDEXED: {
                // Table access - emit appropriate instruction based on key type
                Register reg = register_allocator_.reserve_registers(1);

                if (expr.u.indexed.is_const_key && expr.u.indexed.is_int_key) {
                    // Use GETI for integer constants
                    emitter_.emit_abc(
                        OpCode::OP_GETI, reg, expr.u.indexed.table, expr.u.indexed.key);
                } else if (expr.u.indexed.is_const_key &&
                           (expr.u.indexed.is_string_key || !expr.u.indexed.is_int_key)) {
                    // Use GETFIELD for string constants and floating-point constants
                    emitter_.emit_abc(
                        OpCode::OP_GETFIELD, reg, expr.u.indexed.table, expr.u.indexed.key);
                } else {
                    // Use GETTABLE for register keys
                    emitter_.emit_abc(
                        OpCode::OP_GETTABLE, reg, expr.u.indexed.table, expr.u.indexed.key);
                }

                // Free the table and key registers
                register_allocator_.free_register(expr.u.indexed.table, register_allocator_.local_count());
                if (!expr.u.indexed.is_const_key) {
                    register_allocator_.free_register(expr.u.indexed.key, register_allocator_.local_count());
                }

                expr.kind = ExpressionKind::RELOC;
                expr.u.info = emitter_.instruction_count() - 1;
                break;
            }
            default:
                // Other expression types don't need discharging
                break;
        }
    }

    void CodeGenerator::discharge_to_register(ExpressionDesc& expr, Register reg) {
        discharge_vars(expr);

        switch (expr.kind) {
            case ExpressionKind::NIL:
                emitter_.emit_abc(OpCode::OP_LOADNIL, reg, 0, 0);
                break;
            case ExpressionKind::FALSE:
                emitter_.emit_abc(OpCode::OP_LOADFALSE, reg, 0, 0);
                break;
            case ExpressionKind::TRUE:
                emitter_.emit_abc(OpCode::OP_LOADTRUE, reg, 0, 0);
                break;
            case ExpressionKind::KINT:
                // Try to fit in sBx field first
                if (expr.u.ival >= -32768 && expr.u.ival <= 32767) {
                    emitter_.emit_asbx(OpCode::OP_LOADI, reg, static_cast<std::int32_t>(expr.u.ival));
                } else {
                    // Use constant pool
                    Size const_index = emitter_.add_constant(expr.u.ival);
                    emitter_.emit_abx(OpCode::OP_LOADK, reg, static_cast<std::uint32_t>(const_index));
                }
                break;
            case ExpressionKind::KFLT:
                // Use constant pool for floating point numbers
                {
                    Size const_index = emitter_.add_constant(expr.u.nval);
                    emitter_.emit_abx(OpCode::OP_LOADK, reg, static_cast<std::uint32_t>(const_index));
                }
                break;
            case ExpressionKind::K:
                emitter_.emit_abx(OpCode::OP_LOADK, reg, static_cast<std::uint32_t>(expr.u.info));
                break;
            case ExpressionKind::RELOC: {
                // Patch the instruction to use the specified register
                emitter_.patch_instruction(expr.u.info,
                    InstructionEncoder::encode_abc(
                        InstructionEncoder::decode_opcode(emitter_.instructions()[expr.u.info]),
                        reg,
                        InstructionEncoder::decode_b(emitter_.instructions()[expr.u.info]),
                        InstructionEncoder::decode_c(emitter_.instructions()[expr.u.info])
                    ));
                break;
            }
            case ExpressionKind::NONRELOC:
                if (reg != expr.u.info) {
                    emitter_.emit_abc(OpCode::OP_MOVE, reg, static_cast<Register>(expr.u.info), 0);
                }
                break;
            default:
                // Should not happen for properly discharged expressions
                CODEGEN_LOG_ERROR("Cannot discharge expression kind to register");
                break;
        }

        expr.u.info = reg;
        expr.kind = ExpressionKind::NONRELOC;
    }

    void CodeGenerator::discharge_to_any_register(ExpressionDesc& expr) {
        if (expr.kind != ExpressionKind::NONRELOC) {
            Register reg = register_allocator_.reserve_registers(1);
            discharge_to_register(expr, reg);
        }
    }

    Register CodeGenerator::expression_to_any_register(ExpressionDesc& expr) {
        discharge_vars(expr);
        if (expr.kind == ExpressionKind::NONRELOC) {
            // Expression already has a register
            return static_cast<Register>(expr.u.info);
        }
        // Use next available register
        Register reg = register_allocator_.reserve_registers(1);
        discharge_to_register(expr, reg);
        return reg;
    }

    Register CodeGenerator::expression_to_next_register(ExpressionDesc& expr) {
        discharge_vars(expr);
        free_expression(expr);
        Register reg = register_allocator_.reserve_registers(1);
        discharge_to_register(expr, reg);
        return reg;
    }

    void CodeGenerator::expression_to_next_register_inplace(ExpressionDesc& expr) {
        // Similar to luaK_exp2nextreg - ensures expression is in next available register
        discharge_vars(expr);
        free_expression(expr);
        Register reg = register_allocator_.reserve_registers(1);
        discharge_to_register(expr, reg);
        expr.kind = ExpressionKind::NONRELOC;
        expr.u.info = reg;
    }

    void CodeGenerator::expression_to_register(ExpressionDesc& expr, Register reg) {
        discharge_to_register(expr, reg);
    }

    void CodeGenerator::free_expression(ExpressionDesc& expr) {
        if (expr.kind == ExpressionKind::NONRELOC) {
            register_allocator_.free_register(static_cast<Register>(expr.u.info), register_allocator_.local_count());
        }
    }

    void CodeGenerator::free_expressions(ExpressionDesc& e1, ExpressionDesc& e2) {
        Register r1 = (e1.kind == ExpressionKind::NONRELOC) ? static_cast<Register>(e1.u.info) : 255;
        Register r2 = (e2.kind == ExpressionKind::NONRELOC) ? static_cast<Register>(e2.u.info) : 255;
        if (r1 != 255 || r2 != 255) {
            register_allocator_.free_registers(r1, r2, register_allocator_.local_count());
        }
    }

    // JumpManager implementation
    Size JumpManager::emit_jump(Size target) {
        if (!emitter_) {
            return 0;
        }

        Size jump_index;
        if (target == 0) {
            // Forward jump - emit with placeholder target
            jump_index = emitter_->emit_asbx(OpCode::OP_JMP, 0, 0);
        } else {
            // Backward jump - calculate relative offset
            Size current_pc = emitter_->instruction_count();
            std::int32_t offset =
                static_cast<std::int32_t>(target) - static_cast<std::int32_t>(current_pc) - 1;
            jump_index = emitter_->emit_asbx(OpCode::OP_JMP, 0, offset);
        }

        return jump_index;
    }

    Size JumpManager::emit_conditional_jump(Register condition_reg, Size target) {
        if (!emitter_) {
            return 0;
        }

        Size jump_index;
        if (target == 0) {
            // Forward conditional jump - emit TEST instruction followed by JMP
            emitter_->emit_abc(OpCode::OP_TEST, condition_reg, 0, 0);
            jump_index = emitter_->emit_asbx(OpCode::OP_JMP, 0, 0);
        } else {
            // Backward conditional jump
            Size current_pc = emitter_->instruction_count();
            std::int32_t offset =
                static_cast<std::int32_t>(target) - static_cast<std::int32_t>(current_pc) - 1;
            emitter_->emit_abc(OpCode::OP_TEST, condition_reg, 0, 0);
            jump_index = emitter_->emit_asbx(OpCode::OP_JMP, 0, offset);
        }

        return jump_index;
    }

    void JumpManager::patch_jump(Size jump_index, Size target) {
        if (!emitter_) {
            return;
        }

        // Calculate relative offset from jump instruction to target
        std::int32_t offset =
            static_cast<std::int32_t>(target) - static_cast<std::int32_t>(jump_index) - 1;

        // Create new jump instruction with correct offset
        Instruction patched_instr = InstructionEncoder::encode_asbx(OpCode::OP_JMP, 0, offset);

        // Patch the instruction
        emitter_->patch_instruction(jump_index, patched_instr);

        // Record the patch for debugging
        pending_patches_.emplace_back(jump_index, target);
    }

    void JumpManager::patch_jump_list(const JumpList& jumps, Size target) {
        for (Size jump : jumps) {
            patch_jump(jump, target);
        }
    }

    JumpManager::JumpList JumpManager::create_jump_list() {
        return JumpList{};
    }

    JumpManager::JumpList JumpManager::merge_jump_lists(JumpList list1, const JumpList& list2) {
        list1.insert(list1.end(), list2.begin(), list2.end());
        return list1;
    }

    Size JumpManager::current_instruction() const noexcept {
        return emitter_ ? emitter_->instruction_count() : 0;
    }

    void JumpManager::set_emitter(BytecodeEmitter* emitter) noexcept {
        emitter_ = emitter;
    }

    // ScopeManager implementation
    void ScopeManager::enter_scope() {
        scopes_.push_back({locals_.size(), scopes_.size()});
    }

    void ScopeManager::exit_scope() {
        if (!scopes_.empty()) {
            Size start_local = scopes_.back().start_local;

            // Remove local variables from current scope
            if (start_local < locals_.size()) {
                locals_.erase(locals_.begin() + static_cast<std::ptrdiff_t>(start_local),
                              locals_.end());
            }

            // Update local name map
            local_names_.clear();
            for (Size i = 0; i < locals_.size(); ++i) {
                local_names_[locals_[i].name] = i;
            }

            scopes_.pop_back();
        }
    }

    Size ScopeManager::declare_local(String name, Register reg) {
        Size index = locals_.size();

        // Add to locals vector
        locals_.push_back({name, reg, 0, 0, false});

        // Update name mapping
        local_names_[name] = index;

        return index;
    }

    ScopeManager::VariableResolution ScopeManager::resolve_variable(const String& name) {
        // First check local variables (from innermost to outermost scope)
        auto local_it = local_names_.find(name);
        if (local_it != local_names_.end()) {
            Size local_index = local_it->second;
            if (local_index < locals_.size()) {
                return {VariableResolution::Type::Local, locals_[local_index].reg};
            }
        }

        // Check upvalues
        for (Size i = 0; i < upvalues_.size(); ++i) {
            if (upvalues_[i].name == name) {
                return {VariableResolution::Type::Upvalue, static_cast<std::uint8_t>(i)};
            }
        }

        // Not found - treat as global
        return {VariableResolution::Type::Global, 0};
    }

    Size ScopeManager::scope_depth() const noexcept {
        return scopes_.size();
    }

    const std::vector<ScopeManager::LocalVariable>& ScopeManager::current_locals() const noexcept {
        return locals_;
    }

    // CodeGenerator implementation
    CodeGenerator::CodeGenerator(BytecodeEmitter& emitter)
        : emitter_(emitter), register_allocator_(256), jump_manager_(), scope_manager_() {
        // Initialize jump manager with emitter pointer
        jump_manager_.set_emitter(&emitter_);
    }

    Status CodeGenerator::generate(const frontend::Program& ast) {
        CODEGEN_LOG_INFO("Starting code generation for program");

        // Reset state for new compilation
        register_allocator_.reset();
        current_expression_.reset();

        // Generate code for the program
        ast.accept(*this);

        // Emit final return instruction if not already present
        if (emitter_.instruction_count() == 0 ||
            InstructionEncoder::decode_opcode(emitter_.instructions().back()) !=
                OpCode::OP_RETURN) {
            emitter_.emit_abc(OpCode::OP_RETURN, 0, 1, 0);  // Return with no values
        }

        // Update stack size based on register usage
        emitter_.set_stack_size(register_allocator_.stack_size());

        CODEGEN_LOG_INFO("Code generation completed. Instructions: {}, Stack size: {}",
                         emitter_.instruction_count(),
                         register_allocator_.stack_size());

        // Debug: Print disassembled bytecode
        BytecodeFunction function = emitter_.get_function();
        CODEGEN_LOG_DEBUG("Generated bytecode:\n{}",
                          backend::Disassembler::disassemble_function(function));

        return make_success();
    }

    Result<Register> CodeGenerator::generate_expression(const frontend::Expression& expr) {
        CODEGEN_LOG_DEBUG("Generating expression code");

        // Save current expression state
        Optional<ExpressionDesc> saved_expr = current_expression_;
        current_expression_.reset();

        // Generate code for the expression
        expr.accept(*this);

        // Check if expression generated a result
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("Expression did not produce a result");
            current_expression_ = saved_expr;
            return ErrorCode::RUNTIME_ERROR;
        }

        // Convert expression to register
        ExpressionDesc result_expr = current_expression_.value();
        Register result = expression_to_any_register(result_expr);

        current_expression_ = saved_expr;
        return result;
    }

    Status CodeGenerator::generate_statement(const frontend::Statement& stmt) {
        CODEGEN_LOG_DEBUG("Generating statement code");

        // Save current expression state
        Optional<ExpressionDesc> saved_expr = current_expression_;

        // Generate code for the statement
        stmt.accept(*this);

        // Restore expression state
        current_expression_ = saved_expr;

        return make_success();
    }

    // AST Visitor implementations (using new expression system)
    void CodeGenerator::visit(const frontend::LiteralExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for literal expression");

        ExpressionDesc expr;

        // Set expression type based on literal value
        std::visit(
            [&expr, this](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    expr.kind = ExpressionKind::NIL;
                } else if constexpr (std::is_same_v<T, bool>) {
                    expr.kind = value ? ExpressionKind::TRUE : ExpressionKind::FALSE;
                } else if constexpr (std::is_same_v<T, Int>) {
                    expr.kind = ExpressionKind::KINT;
                    expr.u.ival = value;
                } else if constexpr (std::is_same_v<T, Number>) {
                    expr.kind = ExpressionKind::KFLT;
                    expr.u.nval = value;
                    // Add floating-point constant to constant table
                    expr.kind = ExpressionKind::K;
                    expr.u.info = emitter_.add_constant(value);
                } else if constexpr (std::is_same_v<T, String>) {
                    expr.kind = ExpressionKind::KSTR;
                    // For now, put string constants in constant table
                    // In a full implementation, we'd check if it's a short string first
                    expr.kind = ExpressionKind::K;
                    expr.u.info = emitter_.add_constant(value);
                }
            },
            node.value());

        current_expression_ = expr;
    }

    void CodeGenerator::visit(const frontend::IdentifierExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for identifier: {}", node.name());

        // Resolve the variable
        auto resolution = scope_manager_.resolve_variable(node.name());

        CODEGEN_LOG_DEBUG("Variable '{}' resolved as: type={}, index={}",
                          node.name(),
                          static_cast<int>(resolution.type),
                          resolution.index);

        ExpressionDesc expr;

        switch (resolution.type) {
            case ScopeManager::VariableResolution::Type::Local:
                // Local variable - already in register
                expr.kind = ExpressionKind::LOCAL;
                expr.u.info = resolution.index;
                CODEGEN_LOG_DEBUG(
                    "Local variable '{}' in register R[{}]", node.name(), resolution.index);
                break;
            case ScopeManager::VariableResolution::Type::Upvalue:
                // Upvalue - will be loaded when discharged
                expr.kind = ExpressionKind::UPVAL;
                expr.u.info = resolution.index;
                CODEGEN_LOG_DEBUG("Upvalue '{}' at index {}", node.name(), resolution.index);
                break;
            case ScopeManager::VariableResolution::Type::Global:
                // Global variable - will be loaded when discharged
                expr.kind = ExpressionKind::GLOBAL;
                expr.u.info = emitter_.add_constant(node.name());
                CODEGEN_LOG_DEBUG(
                    "Global variable '{}' with constant index {}", node.name(), expr.u.info);
                break;
        }

        current_expression_ = expr;
    }

    void CodeGenerator::visit(const frontend::BinaryOpExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for binary operation");

        // Generate code for left operand
        node.left().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("Left operand did not produce expression");
            return;
        }
        ExpressionDesc left_expr = current_expression_.value();

        // Generate code for right operand
        node.right().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("Right operand did not produce expression");
            return;
        }
        ExpressionDesc right_expr = current_expression_.value();

        // Try constant folding first
        if (try_constant_folding(node.operator_type(), left_expr, right_expr)) {
            current_expression_ = left_expr;  // Result is in left_expr after folding
            return;
        }

        // Convert operands to registers
        Register left_reg = expression_to_any_register(left_expr);
        Register right_reg = expression_to_any_register(right_expr);

        // Allocate register for result
        Register result_reg = register_allocator_.reserve_registers(1);

        // Generate appropriate instruction based on operator
        switch (node.operator_type()) {
            case frontend::BinaryOpExpression::Operator::Add:
                emitter_.emit_abc(OpCode::OP_ADD, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::Subtract:
                emitter_.emit_abc(OpCode::OP_SUB, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::Multiply:
                emitter_.emit_abc(OpCode::OP_MUL, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::Divide:
                emitter_.emit_abc(OpCode::OP_DIV, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::IntegerDivide:
                emitter_.emit_abc(OpCode::OP_IDIV, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::Modulo:
                emitter_.emit_abc(OpCode::OP_MOD, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::Power:
                emitter_.emit_abc(OpCode::OP_POW, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::BitwiseAnd:
                emitter_.emit_abc(OpCode::OP_BAND, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::BitwiseOr:
                emitter_.emit_abc(OpCode::OP_BOR, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::BitwiseXor:
                emitter_.emit_abc(OpCode::OP_BXOR, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::ShiftLeft:
                emitter_.emit_abc(OpCode::OP_SHL, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::ShiftRight:
                emitter_.emit_abc(OpCode::OP_SHR, result_reg, left_reg, right_reg);
                break;
            case frontend::BinaryOpExpression::Operator::Equal:
                generate_comparison_with_result(
                    left_reg, right_reg, result_reg, OpCode::OP_EQ, false);
                break;
            case frontend::BinaryOpExpression::Operator::NotEqual:
                generate_comparison_with_result(
                    left_reg, right_reg, result_reg, OpCode::OP_EQ, true);
                break;
            case frontend::BinaryOpExpression::Operator::Less:
                generate_comparison_with_result(
                    left_reg, right_reg, result_reg, OpCode::OP_LT, false);
                break;
            case frontend::BinaryOpExpression::Operator::LessEqual:
                generate_comparison_with_result(
                    left_reg, right_reg, result_reg, OpCode::OP_LE, false);
                break;
            case frontend::BinaryOpExpression::Operator::Greater:
                generate_comparison_with_result(
                    right_reg, left_reg, result_reg, OpCode::OP_LT, false);
                break;
            case frontend::BinaryOpExpression::Operator::GreaterEqual:
                generate_comparison_with_result(
                    right_reg, left_reg, result_reg, OpCode::OP_LE, false);
                break;
            case frontend::BinaryOpExpression::Operator::And:
                generate_logical_and(left_expr, right_expr, result_reg);
                return;  // Early return since we handle register management internally
            case frontend::BinaryOpExpression::Operator::Or:
                generate_logical_or(left_expr, right_expr, result_reg);
                return;  // Early return since we handle register management internally
            case frontend::BinaryOpExpression::Operator::Concat:
                // Handle concatenation following Lua 5.5 pattern
                generate_concat_operation(left_expr, right_expr);
                return;  // Early return since we handle register management internally
            default:
                CODEGEN_LOG_ERROR("Unsupported binary operator");
                break;
        }

        // Free the operand registers
        free_expressions(left_expr, right_expr);

        // Set result expression
        ExpressionDesc result_expr;
        result_expr.kind = ExpressionKind::NONRELOC;
        result_expr.u.info = result_reg;
        current_expression_ = result_expr;
    }

    void CodeGenerator::generate_comparison_with_result(Register left_reg,
                                                        Register right_reg,
                                                        Register result_reg,
                                                        OpCode comparison_op,
                                                        bool negate) {
        // Generate comparison instruction that produces a boolean result
        // This follows Lua 5.5 pattern for comparison operations used as expressions

        // Load false into result register first (default case)
        emitter_.emit_abc(OpCode::OP_LOADFALSE, result_reg, 0, 0);

        // Emit the comparison instruction - if true, it will skip the next instruction
        emitter_.emit_abc(comparison_op, left_reg, right_reg, negate ? 1 : 0);

        // Load true into result register (executed if comparison succeeded)
        emitter_.emit_abc(OpCode::OP_LOADTRUE, result_reg, 0, 0);

        CODEGEN_LOG_DEBUG("Generated comparison with result: {} R[{}] R[{}] -> R[{}], negate: {}",
                          static_cast<int>(comparison_op),
                          left_reg,
                          right_reg,
                          result_reg,
                          negate);
    }

    void CodeGenerator::generate_logical_and(ExpressionDesc& left_expr,
                                             ExpressionDesc& right_expr,
                                             Register result_reg) {
        CODEGEN_LOG_DEBUG("Generating logical AND operation");

        // Lua AND semantics: if left is falsy, return left; otherwise return right
        // Implementation:
        // 1. Evaluate left operand
        // 2. Move left to result register
        // 3. Test left operand - if falsy, skip right evaluation
        // 4. Evaluate right operand and move to result register

        // Convert left operand to register
        Register left_reg = expression_to_any_register(left_expr);

        // Move left value to result register initially
        emitter_.emit_abc(OpCode::OP_MOVE, result_reg, left_reg, 0);

        // Test left operand - if falsy (nil or false), skip right evaluation
        emitter_.emit_abc(OpCode::OP_TEST, left_reg, 0, 0);  // Jump if left is falsy
        Size skip_right_jump = jump_manager_.emit_jump();

        // Left is truthy, evaluate right operand
        Register right_reg = expression_to_any_register(right_expr);

        // Move right value to result register (overwriting left)
        emitter_.emit_abc(OpCode::OP_MOVE, result_reg, right_reg, 0);

        // Patch the skip jump to here
        Size end_pos = jump_manager_.current_instruction();
        jump_manager_.patch_jump(skip_right_jump, end_pos);

        // Free operand registers
        free_expressions(left_expr, right_expr);

        // Set result expression
        ExpressionDesc result_expr;
        result_expr.kind = ExpressionKind::NONRELOC;
        result_expr.u.info = result_reg;
        current_expression_ = result_expr;

        CODEGEN_LOG_DEBUG(
            "Logical AND generated: R[{}] := R[{}] and R[{}]", result_reg, left_reg, right_reg);
    }

    void CodeGenerator::generate_logical_or(ExpressionDesc& left_expr,
                                            ExpressionDesc& right_expr,
                                            Register result_reg) {
        CODEGEN_LOG_DEBUG("Generating logical OR operation");

        // Lua OR semantics: if left is truthy, return left; otherwise return right
        // Implementation:
        // 1. Evaluate left operand
        // 2. Move left to result register
        // 3. Test left operand - if truthy, skip right evaluation
        // 4. Evaluate right operand and move to result register

        // Convert left operand to register
        Register left_reg = expression_to_any_register(left_expr);

        // Move left value to result register initially
        emitter_.emit_abc(OpCode::OP_MOVE, result_reg, left_reg, 0);

        // Test left operand - if truthy, skip right evaluation
        emitter_.emit_abc(OpCode::OP_TEST, left_reg, 0, 1);  // Jump if left is truthy (k=1)
        Size skip_right_jump = jump_manager_.emit_jump();

        // Left is falsy, evaluate right operand
        Register right_reg = expression_to_any_register(right_expr);

        // Move right value to result register (overwriting left)
        emitter_.emit_abc(OpCode::OP_MOVE, result_reg, right_reg, 0);

        // Patch the skip jump to here
        Size end_pos = jump_manager_.current_instruction();
        jump_manager_.patch_jump(skip_right_jump, end_pos);

        // Free operand registers
        free_expressions(left_expr, right_expr);

        // Set result expression
        ExpressionDesc result_expr;
        result_expr.kind = ExpressionKind::NONRELOC;
        result_expr.u.info = result_reg;
        current_expression_ = result_expr;

        CODEGEN_LOG_DEBUG(
            "Logical OR generated: R[{}] := R[{}] or R[{}]", result_reg, left_reg, right_reg);
    }

    void CodeGenerator::generate_concat_operation(ExpressionDesc& left_expr,
                                                  ExpressionDesc& right_expr) {
        CODEGEN_LOG_DEBUG("Generating concatenation operation");

        // For now, disable CONCAT merging optimization to fix the register allocation issue
        // TODO: Implement proper consecutive register allocation for CONCAT merging

        // Following Lua 5.5 pattern: ensure operands are in consecutive registers
        expression_to_next_register_inplace(left_expr);
        expression_to_next_register_inplace(right_expr);

        // Emit CONCAT instruction for 2 operands
        emitter_.emit_abc(OpCode::OP_CONCAT,
                          static_cast<Register>(left_expr.u.info),  // Start register
                          2,                                        // Number of elements (2)
                          0                                         // Unused C field
        );

        // Free the right operand register
        free_expression(right_expr);

        // Set result expression - the result is in the left operand's register
        ExpressionDesc result_expr;
        result_expr.kind = ExpressionKind::NONRELOC;
        result_expr.u.info = left_expr.u.info;
        current_expression_ = result_expr;

        CODEGEN_LOG_DEBUG("Concatenation generated: R[{}] := R[{}]..R[{}] (count=2)",
                          static_cast<Register>(left_expr.u.info),
                          static_cast<Register>(left_expr.u.info),
                          static_cast<Register>(left_expr.u.info) + 1);
    }

    void CodeGenerator::visit(const frontend::UnaryOpExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for unary operation");

        // Generate code for operand
        node.operand().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("Operand did not produce expression");
            return;
        }
        ExpressionDesc operand_expr = current_expression_.value();

        // Convert operand to register
        Register operand_reg = expression_to_any_register(operand_expr);

        // Allocate register for result
        Register result_reg = register_allocator_.reserve_registers(1);

        // Generate appropriate instruction based on operator
        switch (node.operator_type()) {
            case frontend::UnaryOpExpression::Operator::Minus:
                emitter_.emit_abc(OpCode::OP_UNM, result_reg, operand_reg, 0);
                break;
            case frontend::UnaryOpExpression::Operator::Not:
                emitter_.emit_abc(OpCode::OP_NOT, result_reg, operand_reg, 0);
                break;
            case frontend::UnaryOpExpression::Operator::Length:
                emitter_.emit_abc(OpCode::OP_LEN, result_reg, operand_reg, 0);
                break;
            case frontend::UnaryOpExpression::Operator::BitwiseNot:
                emitter_.emit_abc(OpCode::OP_BNOT, result_reg, operand_reg, 0);
                break;
            default:
                CODEGEN_LOG_ERROR("Unsupported unary operator");
                break;
        }

        // Free the operand register
        free_expression(operand_expr);

        // Set result expression
        ExpressionDesc result_expr;
        result_expr.kind = ExpressionKind::NONRELOC;
        result_expr.u.info = result_reg;
        current_expression_ = result_expr;
    }

    void CodeGenerator::visit(const frontend::FunctionCallExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for function call");

        // Generate code for the function expression
        node.function().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("Function expression did not produce result");
            return;
        }
        ExpressionDesc func_expr = current_expression_.value();

        // Generate code for arguments
        std::vector<ExpressionDesc> arg_expressions;
        for (const auto& arg : node.arguments()) {
            arg->accept(*this);
            if (current_expression_.has_value()) {
                arg_expressions.push_back(current_expression_.value());
            }
        }

        // In Lua, function calls need consecutive registers: func, arg1, arg2, ...
        // Reserve consecutive registers for the call
        Size total_needed = 1 + arg_expressions.size();
        Register call_base = register_allocator_.reserve_registers(total_needed);

        // Move function to call position
        expression_to_register(func_expr, call_base);

        // Move arguments to call positions
        for (Size i = 0; i < arg_expressions.size(); ++i) {
            expression_to_register(arg_expressions[i], call_base + 1 + static_cast<Register>(i));
        }

        // Emit CALL instruction
        Size arg_count = arg_expressions.size();

        // Determine the number of return values based on context
        Register return_count = 2;  // Default: 1 return value (2 = 1 + 1)
        if (multi_return_context_) {
            return_count = 4;  // 3 return values (4 = 3 + 1) for generic for loops
        }

        emitter_.emit_abc(
            OpCode::OP_CALL, call_base, static_cast<Register>(arg_count + 1), return_count);

        // Free the argument registers (but keep the function register for the result)
        for (Size i = 1; i < total_needed; ++i) {
            register_allocator_.free_register(call_base + static_cast<Register>(i), register_allocator_.local_count());
        }

        // Set result expression - the result is in call_base
        ExpressionDesc result_expr;
        result_expr.kind = ExpressionKind::NONRELOC;
        result_expr.u.info = call_base;
        current_expression_ = result_expr;
    }

    void CodeGenerator::visit(const frontend::BlockStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for block statement");

        // Enter new scope for the block
        scope_manager_.enter_scope();

        // Generate code for all statements in the block
        for (const auto& stmt : node.statements()) {
            stmt->accept(*this);
        }

        // Exit scope, cleaning up local variables
        scope_manager_.exit_scope();
    }

    void CodeGenerator::visit(const frontend::AssignmentStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for assignment statement");

        // Generate code for all values first
        std::vector<ExpressionDesc> value_expressions;
        for (const auto& value : node.values()) {
            value->accept(*this);
            if (current_expression_.has_value()) {
                value_expressions.push_back(current_expression_.value());
            }
        }

        // Assign to targets
        Size value_index = 0;
        for (const auto& target : node.targets()) {
            if (value_index < value_expressions.size()) {
                ExpressionDesc value_expr = value_expressions[value_index];

                // Handle different target types
                if (auto identifier =
                        dynamic_cast<const frontend::IdentifierExpression*>(target.get())) {
                    // Simple variable assignment
                    auto resolution = scope_manager_.resolve_variable(identifier->name());

                    switch (resolution.type) {
                        case ScopeManager::VariableResolution::Type::Local: {
                            // Local variable assignment
                            Register value_reg = expression_to_any_register(value_expr);
                            emitter_.emit_abc(OpCode::OP_MOVE, resolution.index, value_reg, 0);
                            free_expression(value_expr);
                            break;
                        }
                        case ScopeManager::VariableResolution::Type::Upvalue: {
                            // Upvalue assignment
                            Register value_reg = expression_to_any_register(value_expr);
                            emitter_.emit_abc(OpCode::OP_SETUPVAL, value_reg, resolution.index, 0);
                            free_expression(value_expr);
                            break;
                        }
                        case ScopeManager::VariableResolution::Type::Global: {
                            // Global variable assignment using _ENV upvalue
                            // SETTABUP: UpValue[A][K[B]] := R[C]
                            Register value_reg = expression_to_any_register(value_expr);
                            Size const_index = emitter_.add_constant(identifier->name());
                            CODEGEN_LOG_DEBUG("Emitting SETTABUP for global '{}': upvalue=0, "
                                              "key=K[{}], value=R[{}]",
                                              identifier->name(),
                                              const_index,
                                              value_reg);
                            emitter_.emit_abc(
                                OpCode::OP_SETTABUP,
                                0,  // A: upvalue index (0 for _ENV)
                                static_cast<Register>(const_index),  // B: constant index for key
                                value_reg);                          // C: value register
                            free_expression(value_expr);
                            break;
                        }
                    }
                } else if (auto table_access =
                               dynamic_cast<const frontend::TableAccessExpression*>(target.get())) {
                    // Table assignment - generate table and key
                    table_access->table().accept(*this);
                    if (!current_expression_.has_value()) {
                        CODEGEN_LOG_ERROR("Table expression did not produce result");
                        continue;
                    }
                    ExpressionDesc table_expr = current_expression_.value();

                    table_access->key().accept(*this);
                    if (!current_expression_.has_value()) {
                        CODEGEN_LOG_ERROR("Key expression did not produce result");
                        continue;
                    }
                    ExpressionDesc key_expr = current_expression_.value();

                    // Convert table to register
                    Register table_reg = expression_to_any_register(table_expr);
                    Register value_reg = expression_to_any_register(value_expr);

                    // Emit appropriate SET instruction based on key type
                    if (key_expr.kind == ExpressionKind::KINT) {
                        // Use SETI for integer constants
                        emitter_.emit_abc(OpCode::OP_SETI,
                                          table_reg,
                                          static_cast<Register>(key_expr.u.ival),
                                          value_reg);
                        free_expression(key_expr);
                    } else if (key_expr.kind == ExpressionKind::K ||
                               key_expr.kind == ExpressionKind::KSTR) {
                        // Use SETFIELD for string constants and floating-point constants
                        emitter_.emit_abc(OpCode::OP_SETFIELD,
                                          table_reg,
                                          static_cast<Register>(key_expr.u.info),
                                          value_reg);
                        free_expression(key_expr);
                    } else {
                        // Use SETTABLE for register keys
                        Register key_reg = expression_to_any_register(key_expr);
                        emitter_.emit_abc(OpCode::OP_SETTABLE, table_reg, key_reg, value_reg);
                        free_expression(key_expr);
                    }

                    // Free temporary registers
                    free_expression(table_expr);
                    free_expression(value_expr);
                }

                value_index++;
            }
        }
    }

    void CodeGenerator::visit(const frontend::IfStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for if statement");

        std::vector<Size> end_jumps;  // Collect all jumps to the end

        // Generate main if condition
        node.condition().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("Condition expression did not produce result");
            return;
        }
        ExpressionDesc condition_expr = current_expression_.value();

        // Convert condition to register
        Register condition_reg = expression_to_any_register(condition_expr);

        // Test condition and jump if false
        emitter_.emit_abc(OpCode::OP_TEST, condition_reg, 0, 0);
        Size false_jump = jump_manager_.emit_jump();  // Forward jump to next condition/else/end

        // Generate then block
        node.then_body().accept(*this);

        // Jump to end after then block (unless this is the last block)
        if (!node.elseif_clauses().empty() || node.else_body()) {
            end_jumps.push_back(jump_manager_.emit_jump());
        }

        // Free condition register
        free_expression(condition_expr);

        // Process each elseif clause
        for (const auto& elseif_clause : node.elseif_clauses()) {
            // Patch the previous false jump to here
            Size elseif_start = jump_manager_.current_instruction();
            jump_manager_.patch_jump(false_jump, elseif_start);

            // Generate elseif condition
            elseif_clause.condition->accept(*this);
            if (!current_expression_.has_value()) {
                CODEGEN_LOG_ERROR("ElseIf condition expression did not produce result");
                return;
            }
            ExpressionDesc elseif_condition_expr = current_expression_.value();

            // Convert condition to register
            Register elseif_condition_reg = expression_to_any_register(elseif_condition_expr);

            // Test condition and jump if false
            emitter_.emit_abc(OpCode::OP_TEST, elseif_condition_reg, 0, 0);
            false_jump = jump_manager_.emit_jump();  // Forward jump to next condition/else/end

            // Generate elseif body
            elseif_clause.body->accept(*this);

            // Jump to end after elseif body (unless this is the last block)
            if (node.else_body() || &elseif_clause != &node.elseif_clauses().back()) {
                end_jumps.push_back(jump_manager_.emit_jump());
            }

            // Free condition register
            free_expression(elseif_condition_expr);
        }

        // Generate else block if present
        if (node.else_body()) {
            // Patch the last false jump to here (else block)
            Size else_start = jump_manager_.current_instruction();
            jump_manager_.patch_jump(false_jump, else_start);

            node.else_body()->accept(*this);
        } else {
            // No else block, patch the last false jump to the end
            Size end_pos = jump_manager_.current_instruction();
            jump_manager_.patch_jump(false_jump, end_pos);
        }

        // Patch all end jumps to here
        Size final_end = jump_manager_.current_instruction();
        for (Size end_jump : end_jumps) {
            jump_manager_.patch_jump(end_jump, final_end);
        }
    }

    void CodeGenerator::visit(const frontend::Program& node) {
        CODEGEN_LOG_DEBUG("Generating code for program");

        // Enter global scope
        scope_manager_.enter_scope();

        // Clear any previous state
        loop_stack_.clear();
        labels_.clear();
        pending_gotos_.clear();

        // Generate code for all statements
        for (const auto& stmt : node.statements()) {
            stmt->accept(*this);
        }

        // Resolve any pending goto statements
        resolve_pending_gotos();

        // Check for unclosed loops (should not happen with proper AST)
        if (!loop_stack_.empty()) {
            CODEGEN_LOG_ERROR("Program ended with {} unclosed loop contexts", loop_stack_.size());
            loop_stack_.clear();
        }

        // Exit global scope
        scope_manager_.exit_scope();

        CODEGEN_LOG_DEBUG("Program code generation completed");
    }

    // Helper method implementations
    bool CodeGenerator::try_constant_folding(frontend::BinaryOpExpression::Operator op,
                                             ExpressionDesc& e1,
                                             const ExpressionDesc& e2) {
        // For now, implement basic constant folding for integers
        if (e1.kind == ExpressionKind::KINT && e2.kind == ExpressionKind::KINT) {
            switch (op) {
                case frontend::BinaryOpExpression::Operator::Add:
                    e1.u.ival += e2.u.ival;
                    return true;
                case frontend::BinaryOpExpression::Operator::Subtract:
                    e1.u.ival -= e2.u.ival;
                    return true;
                case frontend::BinaryOpExpression::Operator::Multiply:
                    e1.u.ival *= e2.u.ival;
                    return true;
                case frontend::BinaryOpExpression::Operator::IntegerDivide:
                    if (e2.u.ival != 0) {
                        e1.u.ival /= e2.u.ival;
                        return true;
                    }
                    break;
                default:
                    break;
            }
        }
        return false;
    }

    void CodeGenerator::resolve_pending_gotos() {
        // Implementation for resolving goto statements
        // For now, just clear the list
        pending_gotos_.clear();
    }

    // New AST visitor implementations
    void CodeGenerator::visit(const frontend::MethodCallExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for method call: {}", node.method_name());

        // Generate code for the object
        node.object().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("Object expression did not produce result");
            return;
        }
        ExpressionDesc object_expr = current_expression_.value();

        // Generate code for arguments
        std::vector<ExpressionDesc> arg_expressions;
        for (const auto& arg : node.arguments()) {
            arg->accept(*this);
            if (current_expression_.has_value()) {
                arg_expressions.push_back(current_expression_.value());
            }
        }

        // Method calls in Lua are syntactic sugar for obj:method(args) -> obj.method(obj, args)
        // Reserve consecutive registers for: method, object, arg1, arg2, ...
        Size total_needed = 2 + arg_expressions.size();  // method + object + args
        Register call_base = register_allocator_.reserve_registers(total_needed);

        // Get object register
        Register object_reg = expression_to_any_register(object_expr);

        // Get the method from the object
        Size method_const_index = emitter_.add_constant(node.method_name());
        emitter_.emit_abc(OpCode::OP_GETTABLE, call_base, object_reg, static_cast<Register>(method_const_index));

        // Move object to first argument position
        emitter_.emit_abc(OpCode::OP_MOVE, call_base + 1, object_reg, 0);

        // Move arguments to call positions
        for (Size i = 0; i < arg_expressions.size(); ++i) {
            expression_to_register(arg_expressions[i], call_base + 2 + static_cast<Register>(i));
        }

        // Emit CALL instruction
        Size total_args = arg_expressions.size() + 1;  // +1 for object (self)
        emitter_.emit_abc(OpCode::OP_CALL, call_base, static_cast<Register>(total_args + 1), 2);

        // Free argument registers (but keep the method register for the result)
        for (Size i = 1; i < total_needed; ++i) {
            register_allocator_.free_register(call_base + static_cast<Register>(i), register_allocator_.local_count());
        }

        // Clean up expressions
        free_expression(object_expr);
        for (auto& arg_expr : arg_expressions) {
            free_expression(arg_expr);
        }

        // Set result expression - the result is in call_base
        ExpressionDesc result_expr;
        result_expr.kind = ExpressionKind::NONRELOC;
        result_expr.u.info = call_base;
        current_expression_ = result_expr;
    }

    void CodeGenerator::visit(const frontend::TableAccessExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for table access");

        // Generate code for table expression
        node.table().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("Table expression did not produce result");
            return;
        }
        ExpressionDesc table_expr = current_expression_.value();

        // Generate code for key expression
        node.key().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("Key expression did not produce result");
            return;
        }
        ExpressionDesc key_expr = current_expression_.value();

        // Create indexed expression descriptor
        ExpressionDesc result_expr;
        result_expr.kind = ExpressionKind::INDEXED;
        result_expr.u.indexed.table = expression_to_any_register(table_expr);

        // Check if key is a constant
        if (key_expr.kind == ExpressionKind::KINT) {
            // For integer constants, store the integer value directly for GETI instruction
            result_expr.u.indexed.key = static_cast<Register>(key_expr.u.ival);
            result_expr.u.indexed.is_const_key = true;
            result_expr.u.indexed.is_int_key = true;
            result_expr.u.indexed.is_string_key = false;
            // Free the key expression since we're using the constant
            free_expression(key_expr);
        } else if (key_expr.kind == ExpressionKind::K || key_expr.kind == ExpressionKind::KSTR) {
            // For string constants, use the constant index for GETFIELD instruction
            result_expr.u.indexed.key = static_cast<Register>(key_expr.u.info);
            result_expr.u.indexed.is_const_key = true;
            result_expr.u.indexed.is_int_key = false;
            result_expr.u.indexed.is_string_key = true;
            // Free the key expression since we're using the constant
            free_expression(key_expr);

        } else {
            result_expr.u.indexed.key = expression_to_any_register(key_expr);
            result_expr.u.indexed.is_const_key = false;
            result_expr.u.indexed.is_int_key = false;
            result_expr.u.indexed.is_string_key = false;
            // Don't free key_expr here since the register is still needed
        }

        // Don't free table_expr here since the register is still needed
        // The registers will be freed when the INDEXED expression is discharged

        current_expression_ = result_expr;
    }

    void CodeGenerator::visit(const frontend::TableConstructorExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for table constructor");

        // Allocate register for the new table
        Register table_reg = register_allocator_.reserve_registers(1);

        const auto& fields = node.fields();

        // Calculate array and hash sizes for optimization
        Size array_size = 0;
        Size hash_size = 0;
        for (const auto& field : fields) {
            if (field.type == frontend::TableConstructorExpression::Field::Type::List) {
                array_size++;
            } else {
                hash_size++;
            }
        }

        // Create new table with size hints
        emitter_.emit_abc(OpCode::OP_NEWTABLE,
                          table_reg,
                          static_cast<Register>(std::min(array_size, 255UL)),
                          static_cast<Register>(std::min(hash_size, 255UL)));

        // Initialize fields
        Size list_index = 1;  // Lua arrays start at 1
        for (const auto& field : fields) {
            switch (field.type) {
                case frontend::TableConstructorExpression::Field::Type::List: {
                    // List field: table[list_index] = value
                    field.value->accept(*this);

                    if (!current_expression_.has_value()) {
                        CODEGEN_LOG_ERROR("Table field value did not produce expression");
                        continue;
                    }

                    ExpressionDesc value_expr = current_expression_.value();
                    Register value_reg = expression_to_any_register(value_expr);

                    // Load the index into a register first
                    Register index_reg = register_allocator_.reserve_registers(1);
                    emitter_.emit_asbx(
                        OpCode::OP_LOADI, index_reg, static_cast<std::int32_t>(list_index));
                    emitter_.emit_abc(OpCode::OP_SETTABLE, table_reg, index_reg, value_reg);

                    // Free temporary registers
                    register_allocator_.free_register(index_reg, register_allocator_.local_count());
                    free_expression(value_expr);

                    list_index++;
                    break;
                }
                case frontend::TableConstructorExpression::Field::Type::Array:
                case frontend::TableConstructorExpression::Field::Type::Record: {
                    // Array/Record field: table[key] = value
                    if (field.key) {
                        field.key->accept(*this);
                        if (!current_expression_.has_value()) {
                            CODEGEN_LOG_ERROR("Table field key did not produce expression");
                            continue;
                        }
                        ExpressionDesc key_expr = current_expression_.value();
                        Register key_reg = expression_to_any_register(key_expr);

                        field.value->accept(*this);
                        if (!current_expression_.has_value()) {
                            CODEGEN_LOG_ERROR("Table field value did not produce expression");
                            free_expression(key_expr);
                            continue;
                        }
                        ExpressionDesc value_expr = current_expression_.value();
                        Register value_reg = expression_to_any_register(value_expr);

                        emitter_.emit_abc(OpCode::OP_SETTABLE, table_reg, key_reg, value_reg);

                        // Free temporary registers
                        free_expression(key_expr);
                        free_expression(value_expr);
                    }
                    break;
                }
            }
        }

        // Create result expression
        ExpressionDesc result_expr;
        result_expr.kind = ExpressionKind::NONRELOC;
        result_expr.u.info = table_reg;
        current_expression_ = result_expr;
    }

    void CodeGenerator::visit(const frontend::FunctionExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for function expression");

        // Allocate register for the function closure
        auto reg_result = register_allocator_.allocate();
        if (is_error(reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for function expression");
            return;
        }

        Register result_reg = get_value(reg_result);
        current_result_register_ = result_reg;

        // Create a new bytecode emitter for the nested function
        BytecodeEmitter nested_emitter("anonymous_function");

        // Save current state and switch to nested function context
        BytecodeEmitter* saved_emitter = &emitter_;
        RegisterAllocator saved_allocator = register_allocator_;
        register_allocator_ = RegisterAllocator(256);  // Fresh allocator for nested function
        jump_manager_.set_emitter(&nested_emitter);

        // Enter new scope for function parameters and body
        scope_manager_.enter_scope();

        // Declare parameters as local variables
        bool has_vararg = false;
        Size param_count = 0;
        for (const auto& param : node.parameters()) {
            if (param.is_vararg) {
                has_vararg = true;
                continue;  // Don't allocate register for vararg parameter
            }

            auto param_reg_result = register_allocator_.allocate();
            if (is_success(param_reg_result)) {
                Register param_reg = get_value(param_reg_result);
                scope_manager_.declare_local(param.name, param_reg);
                param_count++;
            }
        }

        // Set up function metadata
        nested_emitter.set_parameter_count(param_count);
        nested_emitter.set_vararg(has_vararg);

        // Create a separate code generator for the nested function
        CodeGenerator nested_generator(nested_emitter);

        // Transfer parameter declarations to the nested generator's scope
        nested_generator.scope_manager().enter_scope();
        for (const auto& param : node.parameters()) {
            if (param.is_vararg) {
                continue;  // Don't allocate register for vararg parameter
            }

            auto param_reg_result = nested_generator.register_allocator().allocate();
            if (is_success(param_reg_result)) {
                Register param_reg = get_value(param_reg_result);
                nested_generator.scope_manager().declare_local(param.name, param_reg);
            }
        }

        // Generate code for function body using the nested generator
        node.body().accept(nested_generator);

        // Exit the parameter scope in nested generator
        nested_generator.scope_manager().exit_scope();

        // Ensure function ends with return instruction
        if (nested_emitter.instruction_count() == 0 ||
            InstructionEncoder::decode_opcode(nested_emitter.instructions().back()) !=
                OpCode::OP_RETURN) {
            nested_emitter.emit_abc(OpCode::OP_RETURN, 0, 1, 0);  // Return with no values
        }

        // Update stack size for nested function
        nested_emitter.set_stack_size(nested_generator.register_allocator().high_water_mark() + 1);

        // Get the generated function
        BytecodeFunction nested_function = nested_emitter.get_function();

        // Restore previous state
        register_allocator_ = saved_allocator;
        jump_manager_.set_emitter(saved_emitter);
        scope_manager_.exit_scope();

        // Convert BytecodeFunction to FunctionPrototype and add it
        FunctionPrototype prototype;
        prototype.name = nested_function.name;
        prototype.instructions = std::move(nested_function.instructions);
        prototype.constants = std::move(nested_function.constants);
        prototype.locals = std::move(nested_function.locals);
        prototype.upvalue_descriptors = std::move(nested_function.upvalue_descriptors);
        prototype.parameter_count = nested_function.parameter_count;
        prototype.stack_size = nested_function.stack_size;
        prototype.is_vararg = nested_function.is_vararg;
        prototype.line_info = std::move(nested_function.line_info);
        prototype.source_name = std::move(nested_function.source_name);

        // Add the function prototype and create closure
        Size prototype_index = emitter_.add_prototype(prototype);
        emitter_.emit_abx(
            OpCode::OP_CLOSURE, result_reg, static_cast<std::uint32_t>(prototype_index));

        // Create expression descriptor for the closure
        ExpressionDesc result_expr;
        result_expr.kind = ExpressionKind::NONRELOC;
        result_expr.u.info = result_reg;
        current_expression_ = result_expr;

        // Handle upvalues for the closure
        // For now, we'll implement a simplified upvalue system
        // In a full implementation, we'd need to track which variables are captured
        // and emit appropriate GETUPVAL or MOVE instructions for each upvalue
        CODEGEN_LOG_DEBUG(
            "Function expression compiled with {} parameters, vararg: {}", param_count, has_vararg);
    }

    void CodeGenerator::visit([[maybe_unused]] const frontend::VarargExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for vararg expression");

        // Create vararg expression descriptor
        ExpressionDesc expr;
        expr.kind = ExpressionKind::VARARG;
        expr.u.info = emitter_.instruction_count();  // Will be patched when discharged

        current_expression_ = expr;
    }

    void CodeGenerator::visit(const frontend::ParenthesizedExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for parenthesized expression");

        // Parenthesized expressions in Lua limit multiple return values to one
        // Generate the inner expression
        node.expression().accept(*this);

        // The result is already in current_expression_
        // Parentheses don't change the code generation, just the semantics
        // (limiting multiple return values to one, which we handle at the call site)
    }

    void CodeGenerator::visit(const frontend::LocalDeclarationStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for local declaration");

        const auto& names = node.names();
        const auto& values = node.values();

        // Special handling for multiple variables with single function call
        if (names.size() > 1 && values.size() == 1) {
            // Check if the single value is a function call that might return multiple values
            if (const auto* func_call =
                    dynamic_cast<const frontend::FunctionCallExpression*>(values[0].get())) {
                CODEGEN_LOG_DEBUG("Multi-variable local declaration with single function call");

                // Allocate consecutive registers for all local variables
                std::vector<Register> local_registers;
                for (Size i = 0; i < names.size(); ++i) {
                    auto reg_result = register_allocator_.allocate();
                    if (is_error(reg_result)) {
                        CODEGEN_LOG_ERROR("Failed to allocate register for local variable: {}",
                                          names[i]);
                        return;
                    }
                    local_registers.push_back(get_value(reg_result));
                }

                // Generate the function call with multi-return context
                multi_return_context_ = true;

                // Generate code for the function expression
                func_call->function().accept(*this);
                if (!current_expression_.has_value()) {
                    CODEGEN_LOG_ERROR("Function expression did not produce result");
                    multi_return_context_ = false;
                    return;
                }
                ExpressionDesc func_expr = current_expression_.value();

                // Generate code for arguments
                std::vector<ExpressionDesc> arg_expressions;
                for (const auto& arg : func_call->arguments()) {
                    arg->accept(*this);
                    if (current_expression_.has_value()) {
                        arg_expressions.push_back(current_expression_.value());
                    }
                }

                // Reserve consecutive registers for the call
                Size total_needed = 1 + arg_expressions.size();
                Register call_base = register_allocator_.reserve_registers(total_needed);

                // Move function to call position
                expression_to_register(func_expr, call_base);

                // Move arguments to call positions
                for (Size i = 0; i < arg_expressions.size(); ++i) {
                    expression_to_register(arg_expressions[i],
                                           call_base + 1 + static_cast<Register>(i));
                }

                // Emit CALL instruction with expected number of return values
                Size arg_count = arg_expressions.size();
                Register return_count =
                    static_cast<Register>(names.size() + 1);  // +1 for Lua encoding

                emitter_.emit_abc(
                    OpCode::OP_CALL, call_base, static_cast<Register>(arg_count + 1), return_count);

                multi_return_context_ = false;

                // Move results to local variable registers and declare them
                for (Size i = 0; i < names.size(); ++i) {
                    Register local_reg = local_registers[i];
                    scope_manager_.declare_local(names[i], local_reg);

                    if (call_base + i != local_reg) {
                        emitter_.emit_abc(
                            OpCode::OP_MOVE, local_reg, call_base + static_cast<Register>(i), 0);
                    }
                }

                // Free the argument registers
                for (Size i = 1; i < total_needed; ++i) {
                    register_allocator_.free_register(call_base + static_cast<Register>(i),
                                                      register_allocator_.local_count());
                }

                // Clean up expressions
                free_expression(func_expr);
                for (auto& arg_expr : arg_expressions) {
                    free_expression(arg_expr);
                }

                return;
            }
        }

        // Standard handling for other cases
        std::vector<ExpressionDesc> value_expressions;
        for (const auto& value : values) {
            // Special handling for function expressions in local declarations
            if (const auto* func_expr =
                    dynamic_cast<const frontend::FunctionExpression*>(value.get())) {
                // Handle function expression specially to avoid generating function body in current
                // context
                CODEGEN_LOG_DEBUG("Generating local function expression");

                // Create a new bytecode emitter for the function
                BytecodeEmitter nested_emitter("local_function");

                // Create a separate code generator for the nested function
                CodeGenerator nested_generator(nested_emitter);

                // Declare parameters as local variables in the nested generator
                bool has_vararg = false;
                Size param_count = 0;
                for (const auto& param : func_expr->parameters()) {
                    if (param.is_vararg) {
                        has_vararg = true;
                        continue;  // Don't allocate register for vararg parameter
                    }

                    auto param_reg_result = nested_generator.register_allocator().allocate();
                    if (is_success(param_reg_result)) {
                        Register param_reg = get_value(param_reg_result);
                        nested_generator.scope_manager().declare_local(param.name, param_reg);
                        param_count++;
                    }
                }

                // Set up function metadata
                nested_emitter.set_parameter_count(param_count);
                nested_emitter.set_vararg(has_vararg);

                // Generate code for function body using the nested generator
                func_expr->body().accept(nested_generator);

                // Ensure function ends with return instruction
                if (nested_emitter.instruction_count() == 0 ||
                    InstructionEncoder::decode_opcode(nested_emitter.instructions().back()) !=
                        OpCode::OP_RETURN) {
                    nested_emitter.emit_abc(OpCode::OP_RETURN, 0, 1, 0);  // Return with no values
                }

                // Update stack size for nested function
                nested_emitter.set_stack_size(
                    nested_generator.register_allocator().high_water_mark() + 1);

                // Get the generated function
                BytecodeFunction nested_function = nested_emitter.get_function();

                // Convert BytecodeFunction to FunctionPrototype and add it
                FunctionPrototype prototype;
                prototype.name = nested_function.name;
                prototype.instructions = std::move(nested_function.instructions);
                prototype.constants = std::move(nested_function.constants);
                prototype.locals = std::move(nested_function.locals);
                prototype.upvalue_descriptors = std::move(nested_function.upvalue_descriptors);
                prototype.parameter_count = nested_function.parameter_count;
                prototype.stack_size = nested_function.stack_size;
                prototype.is_vararg = nested_function.is_vararg;
                prototype.line_info = std::move(nested_function.line_info);
                prototype.source_name = std::move(nested_function.source_name);

                // Allocate register for the closure
                auto closure_reg_result = register_allocator_.allocate();
                if (is_error(closure_reg_result)) {
                    CODEGEN_LOG_ERROR("Failed to allocate register for function closure");
                    continue;
                }
                Register closure_reg = get_value(closure_reg_result);

                // Add function prototype and create closure
                Size prototype_index = emitter_.add_prototype(prototype);
                emitter_.emit_abx(
                    OpCode::OP_CLOSURE, closure_reg, static_cast<std::uint32_t>(prototype_index));

                // Create expression descriptor for the closure
                ExpressionDesc closure_expr;
                closure_expr.kind = ExpressionKind::NONRELOC;
                closure_expr.u.info = closure_reg;
                value_expressions.push_back(closure_expr);
            } else {
                // Normal expression handling
                value->accept(*this);
                if (current_expression_.has_value()) {
                    value_expressions.push_back(current_expression_.value());
                }
            }
        }

        // Declare local variables and assign values
        for (Size i = 0; i < names.size(); ++i) {
            // Allocate register for the local variable
            auto reg_result = register_allocator_.allocate();
            if (is_error(reg_result)) {
                CODEGEN_LOG_ERROR("Failed to allocate register for local variable: {}", names[i]);
                continue;
            }

            Register local_reg = get_value(reg_result);

            // Declare the local variable in scope
            scope_manager_.declare_local(names[i], local_reg);

            // Assign value if available
            if (i < value_expressions.size()) {
                // Move value to local register
                ExpressionDesc value_expr = value_expressions[i];
                Register value_reg = expression_to_any_register(value_expr);
                emitter_.emit_abc(OpCode::OP_MOVE, local_reg, value_reg, 0);
                // Free the temporary value register
                free_expression(value_expr);
            } else {
                // Initialize with nil if no value provided
                emitter_.emit_abc(OpCode::OP_LOADNIL, local_reg, 0, 0);
            }
        }
    }

    void CodeGenerator::visit(const frontend::FunctionDeclarationStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for function declaration");

        // Allocate register for the function closure
        auto func_reg_result = register_allocator_.allocate();
        if (is_error(func_reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for function declaration");
            return;
        }

        Register func_reg = get_value(func_reg_result);

        // Create a new bytecode emitter for the function
        BytecodeEmitter nested_emitter("declared_function");

        // Count parameters for function metadata
        bool has_vararg = false;
        Size param_count = 0;
        for (const auto& param : node.parameters()) {
            if (param.is_vararg) {
                has_vararg = true;
                continue;  // Don't allocate register for vararg parameter
            }
            param_count++;
        }

        // Set up function metadata
        nested_emitter.set_parameter_count(param_count);
        nested_emitter.set_vararg(has_vararg);

        // Create a separate code generator for the nested function
        CodeGenerator nested_generator(nested_emitter);

        // Transfer parameter declarations to the nested generator's scope
        nested_generator.scope_manager().enter_scope();
        for (const auto& param : node.parameters()) {
            if (param.is_vararg) {
                continue;  // Don't allocate register for vararg parameter
            }

            auto param_reg_result = nested_generator.register_allocator().allocate();
            if (is_success(param_reg_result)) {
                Register param_reg = get_value(param_reg_result);
                nested_generator.scope_manager().declare_local(param.name, param_reg);
            }
        }

        // Generate code for function body using the nested generator
        node.body().accept(nested_generator);

        // Exit the parameter scope in nested generator
        nested_generator.scope_manager().exit_scope();

        // Ensure function ends with return instruction
        if (nested_emitter.instruction_count() == 0 ||
            InstructionEncoder::decode_opcode(nested_emitter.instructions().back()) !=
                OpCode::OP_RETURN) {
            nested_emitter.emit_abc(OpCode::OP_RETURN, 0, 1, 0);  // Return with no values
        }

        // Update stack size for nested function
        nested_emitter.set_stack_size(nested_generator.register_allocator().high_water_mark() + 1);

        // Get the generated function
        BytecodeFunction nested_function = nested_emitter.get_function();

        // Convert BytecodeFunction to FunctionPrototype and add it
        FunctionPrototype prototype;
        prototype.name = nested_function.name;
        prototype.instructions = std::move(nested_function.instructions);
        prototype.constants = std::move(nested_function.constants);
        prototype.locals = std::move(nested_function.locals);
        prototype.upvalue_descriptors = std::move(nested_function.upvalue_descriptors);
        prototype.parameter_count = nested_function.parameter_count;
        prototype.stack_size = nested_function.stack_size;
        prototype.is_vararg = nested_function.is_vararg;
        prototype.line_info = std::move(nested_function.line_info);
        prototype.source_name = std::move(nested_function.source_name);

        // Add function prototype and create closure
        Size prototype_index = emitter_.add_prototype(prototype);
        emitter_.emit_abx(
            OpCode::OP_CLOSURE, func_reg, static_cast<std::uint32_t>(prototype_index));

        // Handle assignment based on whether it's local or global
        if (node.is_local()) {
            // Local function declaration: local function name(...) ... end
            if (const auto* identifier = dynamic_cast<const frontend::IdentifierExpression*>(&node.name())) {
                // Allocate register for the local function
                auto local_reg_result = register_allocator_.allocate();
                if (is_success(local_reg_result)) {
                    Register local_reg = get_value(local_reg_result);

                    // Declare the local function
                    scope_manager_.declare_local(identifier->name(), local_reg);

                    // Move function to local register
                    emitter_.emit_abc(OpCode::OP_MOVE, local_reg, func_reg, 0);
                }
            }
        } else {
            // Global function declaration: function name(...) ... end
            if (const auto* identifier = dynamic_cast<const frontend::IdentifierExpression*>(&node.name())) {
                // Global function assignment
                Size const_index = emitter_.add_constant(identifier->name());
                emitter_.emit_abx(OpCode::OP_SETTABUP, func_reg, static_cast<std::uint32_t>(const_index));
            } else {
                // Complex function name (e.g., table.func or obj:method)
                // For now, log that this is not implemented
                CODEGEN_LOG_DEBUG("Complex function name assignment not yet implemented");
            }
        }

        // Free the function register
        register_allocator_.free(func_reg);

        CODEGEN_LOG_DEBUG("Function declaration compiled with {} parameters, vararg: {}",
                          param_count,
                          has_vararg);
    }

    void CodeGenerator::visit(const frontend::WhileStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for while statement");

        // Mark loop start
        Size loop_start = jump_manager_.current_instruction();

        // Enter loop context for break/continue handling
        enter_loop(loop_start);

        // Generate condition
        node.condition().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("While condition did not produce expression");
            exit_loop();
            return;
        }

        ExpressionDesc condition_expr = current_expression_.value();
        Register condition_reg = expression_to_any_register(condition_expr);

        // Test condition and jump if false (exit loop)
        emitter_.emit_abc(OpCode::OP_TEST, condition_reg, 0, 0);
        Size exit_jump = jump_manager_.emit_jump();  // Forward jump to loop end

        // Generate loop body
        node.body().accept(*this);

        // Jump back to loop start (continue point)
        jump_manager_.emit_jump(loop_start);

        // Patch exit jump to here (after loop)
        Size loop_end = jump_manager_.current_instruction();
        jump_manager_.patch_jump(exit_jump, loop_end);

        // Exit loop context and patch break/continue jumps
        exit_loop();

        // Free condition register and expression
        free_expression(condition_expr);
    }

    void CodeGenerator::visit(const frontend::ForNumericStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for numeric for statement");

        // Enter new scope for the loop variable
        scope_manager_.enter_scope();

        // Allocate registers for loop control variables
        auto start_reg_result = register_allocator_.allocate();
        auto stop_reg_result = register_allocator_.allocate();
        auto step_reg_result = register_allocator_.allocate();
        auto var_reg_result = register_allocator_.allocate();

        if (is_error(start_reg_result) || is_error(stop_reg_result) || is_error(step_reg_result) ||
            is_error(var_reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate registers for numeric for loop");
            scope_manager_.exit_scope();
            return;
        }

        Register start_reg = get_value(start_reg_result);
        Register stop_reg = get_value(stop_reg_result);
        Register step_reg = get_value(step_reg_result);
        Register var_reg = get_value(var_reg_result);

        // Generate code for start expression
        node.start().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("For start expression did not produce result");
            scope_manager_.exit_scope();
            return;
        }
        ExpressionDesc start_expr = current_expression_.value();
        Register start_value_reg = expression_to_any_register(start_expr);
        emitter_.emit_abc(OpCode::OP_MOVE, start_reg, start_value_reg, 0);
        free_expression(start_expr);

        // Generate code for stop expression
        node.stop().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("For stop expression did not produce result");
            scope_manager_.exit_scope();
            return;
        }
        ExpressionDesc stop_expr = current_expression_.value();
        Register stop_value_reg = expression_to_any_register(stop_expr);
        emitter_.emit_abc(OpCode::OP_MOVE, stop_reg, stop_value_reg, 0);
        free_expression(stop_expr);

        // Generate code for step expression (default to 1 if not provided)
        if (node.step()) {
            node.step()->accept(*this);
            if (!current_expression_.has_value()) {
                CODEGEN_LOG_ERROR("For step expression did not produce result");
                scope_manager_.exit_scope();
                return;
            }
            ExpressionDesc step_expr = current_expression_.value();
            Register step_value_reg = expression_to_any_register(step_expr);
            emitter_.emit_abc(OpCode::OP_MOVE, step_reg, step_value_reg, 0);
            free_expression(step_expr);
        } else {
            // Default step is 1
            emitter_.emit_asbx(OpCode::OP_LOADI, step_reg, 1);
        }

        // Declare the loop variable
        scope_manager_.declare_local(node.variable(), var_reg);

        // Emit FORPREP instruction (sets up the loop)
        Size loop_start = emitter_.emit_asbx(OpCode::OP_FORPREP, start_reg, 0);

        // Enter loop context for break/continue handling
        enter_loop(loop_start);

        // Generate loop body
        node.body().accept(*this);

        // Emit FORLOOP instruction (increments and tests)
        Size loop_end = jump_manager_.current_instruction();
        // FORLOOP should jump back to the instruction after FORPREP (loop body start)
        Size loop_body_start = loop_start + 1;
        std::int32_t loop_offset =
            static_cast<std::int32_t>(loop_body_start) - static_cast<std::int32_t>(loop_end) - 1;
        emitter_.emit_asbx(OpCode::OP_FORLOOP, start_reg, loop_offset);

        // Patch the FORPREP instruction to jump to after the loop
        Size after_loop = jump_manager_.current_instruction();
        std::int32_t prep_offset =
            static_cast<std::int32_t>(after_loop) - static_cast<std::int32_t>(loop_start) - 1;
        emitter_.patch_instruction(
            loop_start,
            InstructionEncoder::encode_asbx(OpCode::OP_FORPREP, start_reg, prep_offset));

        // Exit loop context and patch break/continue jumps
        exit_loop();

        // Exit scope
        scope_manager_.exit_scope();

        // Free registers
        register_allocator_.free(start_reg);
        register_allocator_.free(stop_reg);
        register_allocator_.free(step_reg);
        register_allocator_.free(var_reg);
    }

    void CodeGenerator::visit(const frontend::ForGenericStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for generic for statement");

        // Enter new scope for the loop variables
        scope_manager_.enter_scope();

        // For generic for loops, we need to handle the iterator expression specially
        // The pattern is: for vars in explist do body end
        // Where explist typically returns 3 values: iterator function, state, control variable

        const auto& expressions = node.expressions();
        if (expressions.empty()) {
            CODEGEN_LOG_ERROR("Generic for loop has no iterator expressions");
            scope_manager_.exit_scope();
            return;
        }

        // Allocate consecutive registers for the Lua 5.5 generic for loop protocol:
        // base + 0: iterator function
        // base + 1: state (invariant state)
        // base + 2: control variable (initial value)
        // base + 3: (reserved for upvalue creation by TFORPREP)
        // base + 4: first loop variable
        // base + 5: second loop variable (if any)
        // etc.
        Size total_registers_needed = 4 + node.variables().size();  // base + state + control + reserved + variables
        Register base_reg = register_allocator_.reserve_registers(total_registers_needed);
        Register state_reg = base_reg + 1;
        Register control_reg = base_reg + 2;

        // Generate the iterator expression (e.g., ipairs(t))
        // This should return 3 values: iterator function, state, control variable
        // Set the multi-return context flag so function calls return 3 values
        multi_return_context_ = true;
        expressions[0]->accept(*this);
        multi_return_context_ = false;

        if (current_expression_.has_value()) {
            ExpressionDesc expr = current_expression_.value();

            // The function call should have returned 3 values to consecutive registers
            // starting from the call base register due to the multi_return_context_ flag
            Register call_base_reg = expression_to_any_register(expr);

            // Move the values to the correct registers if needed
            if (call_base_reg != base_reg) {
                emitter_.emit_abc(OpCode::OP_MOVE, base_reg, call_base_reg, 0);
            }
            if (call_base_reg + 1 != state_reg) {
                emitter_.emit_abc(OpCode::OP_MOVE, state_reg, call_base_reg + 1, 0);
            }
            if (call_base_reg + 2 != control_reg) {
                emitter_.emit_abc(OpCode::OP_MOVE, control_reg, call_base_reg + 2, 0);
            }

            free_expression(expr);
        }

        // Set up loop variables at base + 4, base + 5, etc.
        // These are the registers where TFORCALL will store the iterator results
        std::vector<Register> var_registers;
        for (Size i = 0; i < node.variables().size(); ++i) {
            Register var_reg = base_reg + 4 + static_cast<Register>(i);
            var_registers.push_back(var_reg);
            scope_manager_.declare_local(node.variables()[i], var_reg);
        }

        // Emit TFORPREP instruction to set up the loop
        // TFORPREP creates an upvalue for R[A + 3] and jumps to TFORCALL
        Size tforprep_pc = emitter_.emit_abx(OpCode::OP_TFORPREP, base_reg, 0);

        // TFORCALL: R[A+4], ... ,R[A+3+C] := R[A](R[A+1], R[A+2])
        // This is the loop start - where TFORLOOP will jump back to
        Size loop_start = jump_manager_.current_instruction();
        enter_loop(loop_start);

        // Emit TFORCALL instruction to call the iterator function
        auto num_vars = static_cast<Register>(var_registers.size());
        emitter_.emit_abc(OpCode::OP_TFORCALL, base_reg, 0, num_vars);

        // Generate loop body
        node.body().accept(*this);

        // Emit TFORLOOP instruction
        // TFORLOOP: if R[A+4] ~= nil then { R[A+2] := R[A+4]; pc -= Bx }
        Size tforloop_pc = jump_manager_.current_instruction();
        std::int32_t loop_offset = static_cast<std::int32_t>(loop_start) - static_cast<std::int32_t>(tforloop_pc) - 1;
        emitter_.emit_abx(OpCode::OP_TFORLOOP, base_reg, static_cast<Size>(-loop_offset));

        // Patch the TFORPREP instruction to jump to after the loop
        Size after_loop = jump_manager_.current_instruction();
        std::int32_t prep_offset = static_cast<std::int32_t>(after_loop) - static_cast<std::int32_t>(tforprep_pc) - 1;
        emitter_.patch_instruction(
            tforprep_pc,
            InstructionEncoder::encode_abx(OpCode::OP_TFORPREP, base_reg, static_cast<Size>(prep_offset)));

        // Exit loop context and patch break/continue jumps
        exit_loop();

        // Exit scope
        scope_manager_.exit_scope();

        // Free the reserved registers (in reverse order)
        for (Size i = 0; i < total_registers_needed; ++i) {
            register_allocator_.free_register(base_reg + total_registers_needed - 1 - static_cast<Register>(i),
                                              register_allocator_.local_count());
        }
    }

    void CodeGenerator::visit(const frontend::RepeatStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for repeat statement");

        // Mark loop start
        Size loop_start = jump_manager_.current_instruction();

        // Enter loop context for break/continue handling
        enter_loop(loop_start);

        // Generate loop body first (repeat-until executes body at least once)
        node.body().accept(*this);

        // Generate condition
        node.condition().accept(*this);
        if (!current_expression_.has_value()) {
            CODEGEN_LOG_ERROR("Repeat condition did not produce expression");
            exit_loop();
            return;
        }

        ExpressionDesc condition_expr = current_expression_.value();
        Register condition_reg = expression_to_any_register(condition_expr);

        // Test condition and jump back to start if false (continue loop)
        emitter_.emit_abc(OpCode::OP_TEST, condition_reg, 1, 0);  // Test for false
        jump_manager_.emit_jump(loop_start);                      // Jump back to loop start

        // Exit loop context and patch break/continue jumps
        exit_loop();

        // Free condition register and expression
        free_expression(condition_expr);
    }

    void CodeGenerator::visit(const frontend::DoStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for do statement");

        // Do statements create a new scope
        scope_manager_.enter_scope();

        // Generate the body
        node.body().accept(*this);

        // Exit the scope
        scope_manager_.exit_scope();
    }

    void CodeGenerator::visit(const frontend::ReturnStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for return statement");

        const auto& values = node.values();

        if (values.empty()) {
            // Return with no values
            emitter_.emit_abc(OpCode::OP_RETURN, 0, 1, 0);  // Return 0 values
        } else {
            // Generate code for return values
            std::vector<ExpressionDesc> value_expressions;
            for (const auto& value : values) {
                value->accept(*this);
                if (current_expression_.has_value()) {
                    value_expressions.push_back(current_expression_.value());
                }
            }

            // Convert expressions to registers
            std::vector<Register> value_registers;
            for (auto& expr : value_expressions) {
                Register reg = expression_to_any_register(expr);
                value_registers.push_back(reg);
            }

            // Emit return instruction
            if (value_registers.size() == 1) {
                // Single return value
                emitter_.emit_abc(OpCode::OP_RETURN, value_registers[0], 2, 0);  // Return 1 value
            } else if (value_registers.size() > 1) {
                // Multiple return values - arrange them in consecutive registers
                Register base_reg = value_registers[0];

                // Check if values are already in consecutive registers
                bool consecutive = true;
                for (Size i = 1; i < value_registers.size(); ++i) {
                    if (value_registers[i] != base_reg + i) {
                        consecutive = false;
                        break;
                    }
                }

                // If not consecutive, move them to consecutive registers
                if (!consecutive) {
                    // Allocate consecutive registers starting from base_reg
                    for (Size i = 1; i < value_registers.size(); ++i) {
                        Register target_reg = base_reg + static_cast<Register>(i);
                        if (value_registers[i] != target_reg) {
                            emitter_.emit_abc(OpCode::OP_MOVE, target_reg, value_registers[i], 0);
                        }
                    }
                }

                // Emit return with multiple values
                // B field = number of return values + 1 (0 means return all values from base_reg to
                // top)
                emitter_.emit_abc(OpCode::OP_RETURN,
                                  base_reg,
                                  static_cast<Register>(value_registers.size() + 1),
                                  0);
            } else {
                // No return values - should not happen since we checked !values.empty()
                emitter_.emit_abc(OpCode::OP_RETURN, 0, 1, 0);
            }

            // Free expressions and registers
            for (auto& expr : value_expressions) {
                free_expression(expr);
            }
        }
    }

    void CodeGenerator::visit([[maybe_unused]] const frontend::BreakStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for break statement");

        if (!in_loop()) {
            CODEGEN_LOG_ERROR("Break statement outside of loop");
            return;
        }

        // Emit forward jump that will be patched by the enclosing loop
        Size break_jump = jump_manager_.emit_jump();
        add_break_jump(break_jump);

        CODEGEN_LOG_DEBUG("Break statement emitted jump index: {}", break_jump);
    }

    void CodeGenerator::visit(const frontend::GotoStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for goto statement: {}", node.label());

        // Use the label management system to emit goto
        emit_goto(node.label());
    }

    void CodeGenerator::visit(const frontend::LabelStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for label statement: {}", node.name());

        // Use the label management system to define the label
        define_label(node.name());
    }

    void CodeGenerator::visit(const frontend::ExpressionStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for expression statement");

        // Generate the expression
        node.expression().accept(*this);

        // For expression statements, we don't need to keep the result
        // Free the expression if one was generated
        if (current_expression_.has_value()) {
            ExpressionDesc expr = current_expression_.value();
            free_expression(expr);
            current_expression_.reset();
        }
    }

    RegisterAllocator& CodeGenerator::register_allocator() noexcept {
        return register_allocator_;
    }

    JumpManager& CodeGenerator::jump_manager() noexcept {
        return jump_manager_;
    }

    ScopeManager& CodeGenerator::scope_manager() noexcept {
        return scope_manager_;
    }

    BytecodeEmitter& CodeGenerator::emitter() noexcept {
        return emitter_;
    }

    Size CodeGenerator::add_constant(const frontend::LiteralExpression::Value& value) {
        // Convert frontend literal value to bytecode constant value
        ConstantValue constant_value = to_constant_value(value);

        // Use the emitter's add_constant method
        return emitter_.add_constant(constant_value);
    }



    // Loop context management implementation
    void CodeGenerator::enter_loop(Size loop_start) {
        LoopContext context;
        context.loop_start = loop_start;
        context.scope_depth = scope_manager_.scope_depth();
        loop_stack_.push_back(std::move(context));

        CODEGEN_LOG_DEBUG("Entered loop context at instruction {}, scope depth {}",
                          loop_start,
                          context.scope_depth);
    }

    void CodeGenerator::exit_loop() {
        if (loop_stack_.empty()) {
            CODEGEN_LOG_ERROR("Attempting to exit loop context when not in a loop");
            return;
        }

        LoopContext& context = loop_stack_.back();
        Size loop_end = jump_manager_.current_instruction();

        // Patch all break jumps to point to the end of the loop
        jump_manager_.patch_jump_list(context.break_jumps, loop_end);

        // Patch all continue jumps to point to the start of the loop
        jump_manager_.patch_jump_list(context.continue_jumps, context.loop_start);

        CODEGEN_LOG_DEBUG("Exited loop context. Patched {} break jumps and {} continue jumps",
                          context.break_jumps.size(),
                          context.continue_jumps.size());

        loop_stack_.pop_back();
    }

    bool CodeGenerator::in_loop() const noexcept {
        return !loop_stack_.empty();
    }

    void CodeGenerator::add_break_jump(Size jump_index) {
        if (loop_stack_.empty()) {
            CODEGEN_LOG_ERROR("Break statement outside of loop context");
            return;
        }

        loop_stack_.back().break_jumps.push_back(jump_index);
        CODEGEN_LOG_DEBUG("Added break jump at instruction {}", jump_index);
    }

    void CodeGenerator::add_continue_jump(Size jump_index) {
        if (loop_stack_.empty()) {
            CODEGEN_LOG_ERROR("Continue statement outside of loop context");
            return;
        }

        loop_stack_.back().continue_jumps.push_back(jump_index);
        CODEGEN_LOG_DEBUG("Added continue jump at instruction {}", jump_index);
    }

    // Label management implementation
    void CodeGenerator::define_label(const String& name) {
        Size position = jump_manager_.current_instruction();
        Size scope_depth = scope_manager_.scope_depth();

        // Check for duplicate labels in the same scope
        for (const auto& label : labels_) {
            if (label.name == name && label.scope_depth == scope_depth) {
                CODEGEN_LOG_ERROR("Duplicate label '{}' in the same scope", name);
                return;
            }
        }

        // Add the label
        labels_.push_back({name, position, scope_depth});
        CODEGEN_LOG_DEBUG(
            "Defined label '{}' at instruction {}, scope depth {}", name, position, scope_depth);

        // Resolve any pending gotos to this label
        auto it = pending_gotos_.begin();
        while (it != pending_gotos_.end()) {
            if (it->first == name) {
                jump_manager_.patch_jump(it->second, position);
                CODEGEN_LOG_DEBUG(
                    "Resolved pending goto to label '{}' at jump {}", name, it->second);
                it = pending_gotos_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void CodeGenerator::emit_goto(const String& label) {
        Size current_scope = scope_manager_.scope_depth();

        // Look for the label in accessible scopes (current and outer scopes)
        for (const auto& label_info : labels_) {
            if (label_info.name == label && label_info.scope_depth <= current_scope) {
                // Found the label - emit direct jump
                jump_manager_.emit_jump(label_info.position);
                CODEGEN_LOG_DEBUG("Emitted goto to existing label '{}' at instruction {}",
                                  label,
                                  label_info.position);
                return;
            }
        }

        // Label not found yet - emit forward jump and add to pending list
        Size jump_index = jump_manager_.emit_jump();
        pending_gotos_.emplace_back(label, jump_index);
        CODEGEN_LOG_DEBUG("Emitted forward goto to label '{}' at jump {}", label, jump_index);
    }



    // CodeGenContext implementation
    CodeGenContext::CodeGenContext(CodeGenerator& generator) : generator_(generator) {}

    void CodeGenContext::enter_function(const String& name, const std::vector<String>& parameters) {
        function_stack_.push(name);
        scope_stack_.push(generator_.scope_manager().scope_depth());

        // Enter new scope for function parameters
        generator_.scope_manager().enter_scope();

        // Declare parameters as local variables
        for (const auto& param : parameters) {
            auto reg_result = generator_.register_allocator().allocate();
            if (is_success(reg_result)) {
                Register param_reg = get_value(reg_result);
                generator_.scope_manager().declare_local(param, param_reg);
            }
        }
    }

    BytecodeFunction CodeGenContext::exit_function() {
        if (!function_stack_.empty()) {
            function_stack_.pop();
        }

        if (!scope_stack_.empty()) {
            scope_stack_.pop();
        }

        // Exit function scope
        generator_.scope_manager().exit_scope();

        // Get the generated function from the emitter
        // Note: This is a simplified implementation
        // In a full implementation, we'd need to manage multiple emitters
        return generator_.emitter().get_function();
    }

    const String& CodeGenContext::function_name() const noexcept {
        if (function_stack_.empty()) {
            static const String empty_name = "";
            return empty_name;
        }
        return function_stack_.top();
    }

    bool CodeGenContext::in_function() const noexcept {
        return !function_stack_.empty();
    }

}  // namespace rangelua::backend
