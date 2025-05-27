/**
 * @file codegen.cpp
 * @brief Code generator implementation stub
 * @version 0.1.0
 */

#include <rangelua/backend/codegen.hpp>
#include <rangelua/utils/logger.hpp>

namespace rangelua::backend {

    // RegisterAllocator implementation
    RegisterAllocator::RegisterAllocator(Size max_registers)
        : max_registers_(max_registers),
          allocated_(max_registers, false),
          reserved_(max_registers, false),
          next_register_(0),
          high_water_(0) {}

    Result<Register> RegisterAllocator::allocate() {
        // First try to reuse a freed register
        if (!free_list_.empty()) {
            Register reg = free_list_.top();
            free_list_.pop();

            // Verify the register is not reserved
            if (reserved_[reg]) {
                // Put it back and try next register
                free_list_.push(reg);
            } else {
                allocated_[reg] = true;
                high_water_ = std::max(high_water_, reg);
                return reg;
            }
        }

        // Find next available register
        for (Register reg = next_register_; reg < max_registers_; ++reg) {
            if (!allocated_[reg] && !reserved_[reg]) {
                allocated_[reg] = true;
                high_water_ = std::max(high_water_, reg);
                next_register_ = reg + 1;
                return reg;
            }
        }

        // No registers available
        return ErrorCode::RUNTIME_ERROR;
    }

    Status RegisterAllocator::allocate_specific(Register reg) {
        if (reg >= max_registers_) {
            return make_error<std::monostate>(ErrorCode::RUNTIME_ERROR);
        }

        if (allocated_[reg] || reserved_[reg]) {
            return make_error<std::monostate>(ErrorCode::RUNTIME_ERROR);
        }

        allocated_[reg] = true;
        high_water_ = std::max(high_water_, reg);
        return make_success();
    }

    void RegisterAllocator::free(Register reg) {
        if (reg < max_registers_ && allocated_[reg] && !reserved_[reg]) {
            allocated_[reg] = false;
            free_list_.push(reg);
        }
    }

    void RegisterAllocator::reserve(Register reg) {
        if (reg < max_registers_) {
            reserved_[reg] = true;
            // If it was allocated, mark as not allocated but don't add to free list
            if (allocated_[reg]) {
                allocated_[reg] = false;
            }
        }
    }

    Register RegisterAllocator::high_water_mark() const noexcept {
        return high_water_;
    }

    Size RegisterAllocator::allocated_count() const noexcept {
        Size count = 0;
        for (bool alloc : allocated_) {
            if (alloc)
                count++;
        }
        return count;
    }

    bool RegisterAllocator::is_allocated(Register reg) const noexcept {
        return reg < max_registers_ && allocated_[reg];
    }

    void RegisterAllocator::reset() {
        std::fill(allocated_.begin(), allocated_.end(), false);
        std::fill(reserved_.begin(), reserved_.end(), false);
        while (!free_list_.empty())
            free_list_.pop();
        next_register_ = 0;
        high_water_ = 0;
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
        CODEGEN_LOG_INFO("Starting code generation");

        // TODO: Implement code generation
        ast.accept(*this);

        return make_success();
    }

    Result<Register> CodeGenerator::generate_expression(const frontend::Expression& expr) {
        // TODO: Implement expression code generation
        expr.accept(*this);
        return current_result_register_.value_or(0);
    }

    Status CodeGenerator::generate_statement(const frontend::Statement& stmt) {
        // TODO: Implement statement code generation
        stmt.accept(*this);
        return make_success();
    }

    // AST Visitor implementations
    void CodeGenerator::visit(const frontend::LiteralExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for literal expression");

        // Allocate a register for the result
        auto reg_result = register_allocator_.allocate();
        if (is_error(reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for literal");
            return;
        }

        Register result_reg = get_value(reg_result);
        current_result_register_ = result_reg;

        // Generate appropriate load instruction based on literal type
        std::visit(
            [this, result_reg, &node](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    // nil
                    emitter_.emit_abc(OpCode::OP_LOADNIL, result_reg, 0, 0);
                } else if constexpr (std::is_same_v<T, bool>) {
                    if (value) {
                        emitter_.emit_abc(OpCode::OP_LOADTRUE, result_reg, 0, 0);
                    } else {
                        emitter_.emit_abc(OpCode::OP_LOADFALSE, result_reg, 0, 0);
                    }
                } else if constexpr (std::is_same_v<T, Int>) {
                    // Try to fit in sBx field first
                    if (value >= -32768 && value <= 32767) {
                        emitter_.emit_asbx(
                            OpCode::OP_LOADI, result_reg, static_cast<std::int32_t>(value));
                    } else {
                        // Use constant pool
                        Size const_index = add_constant(node.value());
                        emitter_.emit_abx(
                            OpCode::OP_LOADK, result_reg, static_cast<std::uint32_t>(const_index));
                    }
                } else if constexpr (std::is_same_v<T, Number>) {
                    // Use constant pool for floating point numbers
                    Size const_index = add_constant(node.value());
                    emitter_.emit_abx(
                        OpCode::OP_LOADK, result_reg, static_cast<std::uint32_t>(const_index));
                } else if constexpr (std::is_same_v<T, String>) {
                    // Use constant pool for strings
                    Size const_index = add_constant(node.value());
                    emitter_.emit_abx(
                        OpCode::OP_LOADK, result_reg, static_cast<std::uint32_t>(const_index));
                }
            },
            node.value());
    }

    void CodeGenerator::visit(const frontend::IdentifierExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for identifier: {}", node.name());

        // Resolve the variable
        auto resolution = scope_manager_.resolve_variable(node.name());

        // Allocate a register for the result
        auto reg_result = register_allocator_.allocate();
        if (is_error(reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for identifier");
            return;
        }

        Register result_reg = get_value(reg_result);
        current_result_register_ = result_reg;

        switch (resolution.type) {
            case ScopeManager::VariableResolution::Type::Local:
                // Local variable - move from local register
                emitter_.emit_abc(OpCode::OP_MOVE, result_reg, resolution.index, 0);
                break;
            case ScopeManager::VariableResolution::Type::Upvalue:
                // Upvalue - get from upvalue
                emitter_.emit_abc(OpCode::OP_GETUPVAL, result_reg, resolution.index, 0);
                break;
            case ScopeManager::VariableResolution::Type::Global:
                // Global variable - get from global table
                Size const_index = emitter_.add_constant(node.name());
                emitter_.emit_abx(
                    OpCode::OP_GETTABUP, result_reg, static_cast<std::uint32_t>(const_index));
                break;
        }
    }

    void CodeGenerator::visit(const frontend::BinaryOpExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for binary operation");

        // Generate code for left operand
        node.left().accept(*this);
        Register left_reg = current_result_register_.value_or(0);

        // Generate code for right operand
        node.right().accept(*this);
        Register right_reg = current_result_register_.value_or(0);

        // Allocate register for result
        auto reg_result = register_allocator_.allocate();
        if (is_error(reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for binary operation");
            return;
        }

        Register result_reg = get_value(reg_result);
        current_result_register_ = result_reg;

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
                emitter_.emit_abc(OpCode::OP_EQ, left_reg, right_reg, 0);
                break;
            case frontend::BinaryOpExpression::Operator::NotEqual:
                emitter_.emit_abc(OpCode::OP_EQ, left_reg, right_reg, 1);
                break;
            case frontend::BinaryOpExpression::Operator::Less:
                emitter_.emit_abc(OpCode::OP_LT, left_reg, right_reg, 0);
                break;
            case frontend::BinaryOpExpression::Operator::LessEqual:
                emitter_.emit_abc(OpCode::OP_LE, left_reg, right_reg, 0);
                break;
            case frontend::BinaryOpExpression::Operator::Greater:
                emitter_.emit_abc(OpCode::OP_LT, right_reg, left_reg, 0);
                break;
            case frontend::BinaryOpExpression::Operator::GreaterEqual:
                emitter_.emit_abc(OpCode::OP_LE, right_reg, left_reg, 0);
                break;
            default:
                CODEGEN_LOG_ERROR("Unsupported binary operator");
                break;
        }

        // Free the operand registers
        register_allocator_.free(left_reg);
        register_allocator_.free(right_reg);
    }

    void CodeGenerator::visit(const frontend::UnaryOpExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for unary operation");

        // Generate code for operand
        node.operand().accept(*this);
        Register operand_reg = current_result_register_.value_or(0);

        // Allocate register for result
        auto reg_result = register_allocator_.allocate();
        if (is_error(reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for unary operation");
            return;
        }

        Register result_reg = get_value(reg_result);
        current_result_register_ = result_reg;

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
        register_allocator_.free(operand_reg);
    }

    void CodeGenerator::visit(const frontend::FunctionCallExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for function call");

        // Generate code for the function expression
        node.function().accept(*this);
        Register func_reg = current_result_register_.value_or(0);

        // Generate code for arguments
        std::vector<Register> arg_registers;
        for (const auto& arg : node.arguments()) {
            arg->accept(*this);
            if (current_result_register_.has_value()) {
                arg_registers.push_back(current_result_register_.value());
            }
        }

        // Allocate register for result
        auto result_reg_result = register_allocator_.allocate();
        if (is_error(result_reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for function call result");
            return;
        }

        Register result_reg = get_value(result_reg_result);
        current_result_register_ = result_reg;

        // For now, emit a simple CALL instruction
        // TODO: Implement proper argument passing and multiple return values
        Size arg_count = arg_registers.size();
        emitter_.emit_abc(OpCode::OP_CALL, func_reg, static_cast<Register>(arg_count + 1), 2);

        // Move result to our result register
        emitter_.emit_abc(OpCode::OP_MOVE, result_reg, func_reg, 0);

        // Free argument registers
        for (Register reg : arg_registers) {
            register_allocator_.free(reg);
        }
        register_allocator_.free(func_reg);
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
        std::vector<Register> value_registers;
        for (const auto& value : node.values()) {
            value->accept(*this);
            if (current_result_register_.has_value()) {
                value_registers.push_back(current_result_register_.value());
            }
        }

        // Assign to targets
        Size value_index = 0;
        for (const auto& target : node.targets()) {
            if (value_index < value_registers.size()) {
                Register value_reg = value_registers[value_index];

                // Handle different target types
                if (auto identifier =
                        dynamic_cast<const frontend::IdentifierExpression*>(target.get())) {
                    // Simple variable assignment
                    auto resolution = scope_manager_.resolve_variable(identifier->name());

                    switch (resolution.type) {
                        case ScopeManager::VariableResolution::Type::Local:
                            // Local variable assignment
                            emitter_.emit_abc(OpCode::OP_MOVE, resolution.index, value_reg, 0);
                            break;
                        case ScopeManager::VariableResolution::Type::Upvalue:
                            // Upvalue assignment
                            emitter_.emit_abc(OpCode::OP_SETUPVAL, value_reg, resolution.index, 0);
                            break;
                        case ScopeManager::VariableResolution::Type::Global:
                            // Global variable assignment
                            Size const_index = emitter_.add_constant(identifier->name());
                            emitter_.emit_abx(OpCode::OP_SETTABUP,
                                              value_reg,
                                              static_cast<std::uint32_t>(const_index));
                            break;
                    }
                } else if (auto table_access =
                               dynamic_cast<const frontend::TableAccessExpression*>(target.get())) {
                    // Table assignment - generate table and key
                    table_access->table().accept(*this);
                    Register table_reg = current_result_register_.value_or(0);

                    table_access->key().accept(*this);
                    Register key_reg = current_result_register_.value_or(0);

                    // Emit SETTABLE instruction
                    emitter_.emit_abc(OpCode::OP_SETTABLE, table_reg, key_reg, value_reg);

                    // Free temporary registers
                    register_allocator_.free(table_reg);
                    register_allocator_.free(key_reg);
                }

                // Free the value register
                register_allocator_.free(value_reg);
                value_index++;
            }
        }
    }

    void CodeGenerator::visit(const frontend::IfStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for if statement");

        // Generate condition
        node.condition().accept(*this);
        Register condition_reg = current_result_register_.value_or(0);

        // Test condition and jump if false
        emitter_.emit_abc(OpCode::OP_TEST, condition_reg, 0, 0);
        Size false_jump = jump_manager_.emit_jump();  // Forward jump to else/end

        // Generate then block
        node.then_body().accept(*this);

        Size end_jump = 0;
        if (node.else_body()) {
            // Jump over else block
            end_jump = jump_manager_.emit_jump();
        }

        // Patch false jump to here (else block or end)
        Size else_start = jump_manager_.current_instruction();
        jump_manager_.patch_jump(false_jump, else_start);

        // Generate else block if present
        if (node.else_body()) {
            node.else_body()->accept(*this);

            // Patch end jump
            Size end_pos = jump_manager_.current_instruction();
            jump_manager_.patch_jump(end_jump, end_pos);
        }

        // Free condition register
        register_allocator_.free(condition_reg);
    }

    void CodeGenerator::visit(const frontend::Program& node) {
        // TODO: Implement program code generation
        CODEGEN_LOG_DEBUG("Generating code for program");

        scope_manager_.enter_scope();

        for (const auto& stmt : node.statements()) {
            stmt->accept(*this);
        }

        scope_manager_.exit_scope();
    }

    // New AST visitor implementations
    void CodeGenerator::visit(const frontend::MethodCallExpression& node) {
        // TODO: Implement method call code generation
        CODEGEN_LOG_DEBUG("Generating code for method call: {}", node.method_name());
    }

    void CodeGenerator::visit(const frontend::TableAccessExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for table access");

        // Generate code for table expression
        node.table().accept(*this);
        Register table_reg = current_result_register_.value_or(0);

        // Generate code for key expression
        node.key().accept(*this);
        Register key_reg = current_result_register_.value_or(0);

        // Allocate register for result
        auto result_reg_result = register_allocator_.allocate();
        if (is_error(result_reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for table access result");
            return;
        }

        Register result_reg = get_value(result_reg_result);
        current_result_register_ = result_reg;

        // Emit GETTABLE instruction
        emitter_.emit_abc(OpCode::OP_GETTABLE, result_reg, table_reg, key_reg);

        // Free temporary registers
        register_allocator_.free(table_reg);
        register_allocator_.free(key_reg);
    }

    void CodeGenerator::visit(const frontend::TableConstructorExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for table constructor");

        // Allocate register for the new table
        auto table_reg_result = register_allocator_.allocate();
        if (is_error(table_reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for table constructor");
            return;
        }

        Register table_reg = get_value(table_reg_result);
        current_result_register_ = table_reg;

        // Create new table
        // TODO: Calculate proper array and hash sizes based on fields
        emitter_.emit_abc(OpCode::OP_NEWTABLE, table_reg, 0, 0);

        // TODO: Implement field initialization
        // For now, just create an empty table
        const auto& fields = node.fields();
        for (Size i = 0; i < fields.size(); ++i) {
            // This is a placeholder - proper field handling would require
            // analyzing field types (array vs hash) and generating appropriate
            // SETTABLE or SETLIST instructions
        }
    }

    void CodeGenerator::visit(const frontend::FunctionExpression& node) {
        // TODO: Implement function expression code generation
        CODEGEN_LOG_DEBUG("Generating code for function expression");
    }

    void CodeGenerator::visit(const frontend::VarargExpression& node) {
        // TODO: Implement vararg expression code generation
        CODEGEN_LOG_DEBUG("Generating code for vararg expression");
    }

    void CodeGenerator::visit(const frontend::ParenthesizedExpression& node) {
        // TODO: Implement parenthesized expression code generation
        CODEGEN_LOG_DEBUG("Generating code for parenthesized expression");
        node.expression().accept(*this);
    }

    void CodeGenerator::visit(const frontend::LocalDeclarationStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for local declaration");

        const auto& names = node.names();
        const auto& values = node.values();

        // Generate code for all values first
        std::vector<Register> value_registers;
        for (const auto& value : values) {
            value->accept(*this);
            if (current_result_register_.has_value()) {
                value_registers.push_back(current_result_register_.value());
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
            if (i < value_registers.size()) {
                // Move value to local register
                emitter_.emit_abc(OpCode::OP_MOVE, local_reg, value_registers[i], 0);
                // Free the temporary value register
                register_allocator_.free(value_registers[i]);
            } else {
                // Initialize with nil if no value provided
                emitter_.emit_abc(OpCode::OP_LOADNIL, local_reg, 0, 0);
            }
        }
    }

    void CodeGenerator::visit(const frontend::FunctionDeclarationStatement& node) {
        // TODO: Implement function declaration code generation
        CODEGEN_LOG_DEBUG("Generating code for function declaration");
    }

    void CodeGenerator::visit(const frontend::WhileStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for while statement");

        // Mark loop start
        Size loop_start = jump_manager_.current_instruction();

        // Generate condition
        node.condition().accept(*this);
        Register condition_reg = current_result_register_.value_or(0);

        // Test condition and jump if false (exit loop)
        emitter_.emit_abc(OpCode::OP_TEST, condition_reg, 0, 0);
        Size exit_jump = jump_manager_.emit_jump();  // Forward jump to loop end

        // Generate loop body
        node.body().accept(*this);

        // Jump back to loop start
        jump_manager_.emit_jump(loop_start);

        // Patch exit jump to here (after loop)
        Size loop_end = jump_manager_.current_instruction();
        jump_manager_.patch_jump(exit_jump, loop_end);

        // Free condition register
        register_allocator_.free(condition_reg);
    }

    void CodeGenerator::visit(const frontend::ForNumericStatement& node) {
        // TODO: Implement numeric for statement code generation
        CODEGEN_LOG_DEBUG("Generating code for numeric for statement");
    }

    void CodeGenerator::visit(const frontend::ForGenericStatement& node) {
        // TODO: Implement generic for statement code generation
        CODEGEN_LOG_DEBUG("Generating code for generic for statement");
    }

    void CodeGenerator::visit(const frontend::RepeatStatement& node) {
        // TODO: Implement repeat statement code generation
        CODEGEN_LOG_DEBUG("Generating code for repeat statement");
    }

    void CodeGenerator::visit(const frontend::DoStatement& node) {
        // TODO: Implement do statement code generation
        CODEGEN_LOG_DEBUG("Generating code for do statement");
        node.body().accept(*this);
    }

    void CodeGenerator::visit(const frontend::ReturnStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for return statement");

        const auto& values = node.values();

        if (values.empty()) {
            // Return with no values
            emitter_.emit_abc(OpCode::OP_RETURN, 0, 1, 0);  // Return 0 values
        } else {
            // Generate code for return values
            std::vector<Register> value_registers;
            for (const auto& value : values) {
                value->accept(*this);
                if (current_result_register_.has_value()) {
                    value_registers.push_back(current_result_register_.value());
                }
            }

            // Emit return instruction
            if (value_registers.size() == 1) {
                // Single return value
                emitter_.emit_abc(OpCode::OP_RETURN, value_registers[0], 2, 0);  // Return 1 value
            } else {
                // Multiple return values - need to arrange them in consecutive registers
                // For now, just return the first value
                // TODO: Implement proper multiple return value handling
                if (!value_registers.empty()) {
                    emitter_.emit_abc(OpCode::OP_RETURN, value_registers[0], 2, 0);
                }
            }

            // Free value registers
            for (Register reg : value_registers) {
                register_allocator_.free(reg);
            }
        }
    }

    void CodeGenerator::visit(const frontend::BreakStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for break statement");
        // TODO: Implement break statement with proper loop context tracking
        // For now, just emit a placeholder jump
        jump_manager_.emit_jump();
    }

    void CodeGenerator::visit(const frontend::GotoStatement& node) {
        // TODO: Implement goto statement code generation
        CODEGEN_LOG_DEBUG("Generating code for goto statement: {}", node.label());
    }

    void CodeGenerator::visit(const frontend::LabelStatement& node) {
        // TODO: Implement label statement code generation
        CODEGEN_LOG_DEBUG("Generating code for label statement: {}", node.name());
    }

    void CodeGenerator::visit(const frontend::ExpressionStatement& node) {
        // TODO: Implement expression statement code generation
        CODEGEN_LOG_DEBUG("Generating code for expression statement");
        node.expression().accept(*this);
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

    // Helper method stubs
    Register CodeGenerator::emit_load_constant(const frontend::LiteralExpression::Value& value) {
        // TODO: Implement constant loading
        return 0;
    }

    Register CodeGenerator::emit_binary_operation(frontend::BinaryOpExpression::Operator op,
                                                  Register left,
                                                  Register right) {
        // TODO: Implement binary operation emission
        return 0;
    }

    Register CodeGenerator::emit_unary_operation(frontend::UnaryOpExpression::Operator op,
                                                 Register operand) {
        // TODO: Implement unary operation emission
        return 0;
    }

    void CodeGenerator::emit_assignment(Register target, Register source) {
        // TODO: Implement assignment emission
    }

    void CodeGenerator::emit_conditional_jump(Register condition, Size target) {
        // TODO: Implement conditional jump emission
    }

    Size CodeGenerator::add_constant(const frontend::LiteralExpression::Value& value) {
        // Convert frontend literal value to bytecode constant value
        ConstantValue constant_value = to_constant_value(value);

        // Use the emitter's add_constant method
        return emitter_.add_constant(constant_value);
    }

}  // namespace rangelua::backend
