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
        CODEGEN_LOG_DEBUG("Generating code for method call: {}", node.method_name());

        // Generate code for the object
        node.object().accept(*this);
        Register object_reg = current_result_register_.value_or(0);

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
            CODEGEN_LOG_ERROR("Failed to allocate register for method call result");
            return;
        }

        Register result_reg = get_value(result_reg_result);
        current_result_register_ = result_reg;

        // Method calls in Lua are syntactic sugar for obj:method(args) -> obj.method(obj, args)
        // First, get the method from the object
        Size method_const_index = emitter_.add_constant(node.method_name());
        auto method_reg_result = register_allocator_.allocate();
        if (is_error(method_reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for method");
            return;
        }
        Register method_reg = get_value(method_reg_result);

        emitter_.emit_abc(
            OpCode::OP_GETTABLE, method_reg, object_reg, static_cast<Register>(method_const_index));

        // Emit CALL instruction with object as first argument
        Size total_args = arg_registers.size() + 2;  // +1 for object, +1 for function
        emitter_.emit_abc(OpCode::OP_CALL, method_reg, static_cast<Register>(total_args), 2);

        // Move result to our result register
        emitter_.emit_abc(OpCode::OP_MOVE, result_reg, method_reg, 0);

        // Free registers
        register_allocator_.free(object_reg);
        register_allocator_.free(method_reg);
        for (Register reg : arg_registers) {
            register_allocator_.free(reg);
        }
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
                    Register value_reg = current_result_register_.value_or(0);

                    // Use SETLIST for consecutive array elements (optimization)
                    emitter_.emit_abc(OpCode::OP_SETTABLE,
                                      table_reg,
                                      static_cast<Register>(list_index),
                                      value_reg);

                    register_allocator_.free(value_reg);
                    list_index++;
                    break;
                }
                case frontend::TableConstructorExpression::Field::Type::Array:
                case frontend::TableConstructorExpression::Field::Type::Record: {
                    // Array/Record field: table[key] = value
                    if (field.key) {
                        field.key->accept(*this);
                        Register key_reg = current_result_register_.value_or(0);

                        field.value->accept(*this);
                        Register value_reg = current_result_register_.value_or(0);

                        emitter_.emit_abc(OpCode::OP_SETTABLE, table_reg, key_reg, value_reg);

                        register_allocator_.free(key_reg);
                        register_allocator_.free(value_reg);
                    }
                    break;
                }
            }
        }

        // Result is already in table_reg
        current_result_register_ = table_reg;
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

        // Create a new function prototype
        // For now, we'll create a simple function that returns nil
        // TODO: Implement full nested function compilation

        // Create a new bytecode emitter for the nested function
        BytecodeEmitter nested_emitter("anonymous_function");

        // Enter new scope for function parameters and body
        scope_manager_.enter_scope();

        // Declare parameters as local variables
        bool has_vararg = false;
        for (const auto& param : node.parameters()) {
            if (param.is_vararg) {
                has_vararg = true;
                continue;  // Don't allocate register for vararg parameter
            }

            auto param_reg_result = register_allocator_.allocate();
            if (is_success(param_reg_result)) {
                Register param_reg = get_value(param_reg_result);
                scope_manager_.declare_local(param.name, param_reg);
            }
        }

        // Set up function metadata
        nested_emitter.set_parameter_count(node.parameters().size());
        nested_emitter.set_vararg(has_vararg);

        // Generate code for function body
        // Save current emitter and switch to nested emitter
        [[maybe_unused]] BytecodeEmitter* saved_emitter = &emitter_;
        [[maybe_unused]] BytecodeEmitter* old_jump_emitter = &emitter_;

        // TODO: Implement proper nested function code generation
        // For now, just emit a simple return nil
        nested_emitter.emit_abc(OpCode::OP_LOADNIL, 0, 0, 0);
        nested_emitter.emit_abc(OpCode::OP_RETURN, 0, 2, 0);

        // Get the generated function
        BytecodeFunction nested_function = nested_emitter.get_function();

        // Exit function scope
        scope_manager_.exit_scope();

        // Add the function as a constant and create closure
        Size function_const_index = emitter_.add_constant(ConstantValue{String{"<function>"}});
        emitter_.emit_abx(
            OpCode::OP_CLOSURE, result_reg, static_cast<std::uint32_t>(function_const_index));

        // TODO: Handle upvalues for the closure
        // For each upvalue, emit GETUPVAL or MOVE instructions
    }

    void CodeGenerator::visit([[maybe_unused]] const frontend::VarargExpression& node) {
        CODEGEN_LOG_DEBUG("Generating code for vararg expression");

        // Allocate register for vararg result
        auto reg_result = register_allocator_.allocate();
        if (is_error(reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for vararg expression");
            return;
        }

        Register result_reg = get_value(reg_result);
        current_result_register_ = result_reg;

        // Emit VARARG instruction to load vararg values
        // The second operand indicates how many values to load (0 = all)
        emitter_.emit_abc(OpCode::OP_VARARG, result_reg, 0, 0);
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
        CODEGEN_LOG_DEBUG("Generating code for function declaration");

        // Allocate register for the function closure
        auto func_reg_result = register_allocator_.allocate();
        if (is_error(func_reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for function declaration");
            return;
        }

        Register func_reg = get_value(func_reg_result);

        // For now, create a simple function that returns nil
        // TODO: Implement proper function body compilation
        BytecodeEmitter nested_emitter("declared_function");

        // Set up function metadata
        nested_emitter.set_parameter_count(node.parameters().size());

        // Check for vararg parameters
        bool has_vararg = false;
        for (const auto& param : node.parameters()) {
            if (param.is_vararg) {
                has_vararg = true;
                break;
            }
        }
        nested_emitter.set_vararg(has_vararg);

        // Generate simple function body (return nil for now)
        nested_emitter.emit_abc(OpCode::OP_LOADNIL, 0, 0, 0);
        nested_emitter.emit_abc(OpCode::OP_RETURN, 0, 2, 0);

        // Add function as constant and create closure
        Size function_const_index = emitter_.add_constant(ConstantValue{String{"<declared_function>"}});
        emitter_.emit_abx(OpCode::OP_CLOSURE, func_reg, static_cast<std::uint32_t>(function_const_index));

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
                // TODO: Implement complex function name assignment
                CODEGEN_LOG_DEBUG("Complex function name assignment not yet implemented");
            }
        }

        // Free the function register
        register_allocator_.free(func_reg);
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
        emitter_.emit_abc(OpCode::OP_MOVE, start_reg, current_result_register_.value_or(0), 0);
        if (current_result_register_.has_value()) {
            register_allocator_.free(current_result_register_.value());
        }

        // Generate code for stop expression
        node.stop().accept(*this);
        emitter_.emit_abc(OpCode::OP_MOVE, stop_reg, current_result_register_.value_or(0), 0);
        if (current_result_register_.has_value()) {
            register_allocator_.free(current_result_register_.value());
        }

        // Generate code for step expression (default to 1 if not provided)
        if (node.step()) {
            node.step()->accept(*this);
            emitter_.emit_abc(OpCode::OP_MOVE, step_reg, current_result_register_.value_or(0), 0);
            if (current_result_register_.has_value()) {
                register_allocator_.free(current_result_register_.value());
            }
        } else {
            // Default step is 1
            emitter_.emit_asbx(OpCode::OP_LOADI, step_reg, 1);
        }

        // Declare the loop variable
        scope_manager_.declare_local(node.variable(), var_reg);

        // Emit FORPREP instruction (sets up the loop)
        Size loop_start = emitter_.emit_asbx(OpCode::OP_FORPREP, start_reg, 0);

        // Generate loop body
        node.body().accept(*this);

        // Emit FORLOOP instruction (increments and tests)
        Size loop_end = jump_manager_.current_instruction();
        std::int32_t loop_offset =
            static_cast<std::int32_t>(loop_start) - static_cast<std::int32_t>(loop_end) - 1;
        emitter_.emit_asbx(OpCode::OP_FORLOOP, start_reg, loop_offset);

        // Patch the FORPREP instruction to jump to after the loop
        Size after_loop = jump_manager_.current_instruction();
        std::int32_t prep_offset =
            static_cast<std::int32_t>(after_loop) - static_cast<std::int32_t>(loop_start) - 1;
        emitter_.patch_instruction(
            loop_start,
            InstructionEncoder::encode_asbx(OpCode::OP_FORPREP, start_reg, prep_offset));

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

        // Allocate registers for iterator function, state, and control variable
        auto iter_reg_result = register_allocator_.allocate();
        auto state_reg_result = register_allocator_.allocate();
        auto control_reg_result = register_allocator_.allocate();

        if (is_error(iter_reg_result) || is_error(state_reg_result) || is_error(control_reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate registers for generic for loop");
            scope_manager_.exit_scope();
            return;
        }

        Register iter_reg = get_value(iter_reg_result);
        Register state_reg = get_value(state_reg_result);
        Register control_reg = get_value(control_reg_result);

        // Generate code for iterator expressions
        const auto& expressions = node.expressions();
        if (expressions.size() >= 1) {
            // First expression is the iterator function
            expressions[0]->accept(*this);
            emitter_.emit_abc(OpCode::OP_MOVE, iter_reg, current_result_register_.value_or(0), 0);
            if (current_result_register_.has_value()) {
                register_allocator_.free(current_result_register_.value());
            }
        }

        if (expressions.size() >= 2) {
            // Second expression is the state
            expressions[1]->accept(*this);
            emitter_.emit_abc(OpCode::OP_MOVE, state_reg, current_result_register_.value_or(0), 0);
            if (current_result_register_.has_value()) {
                register_allocator_.free(current_result_register_.value());
            }
        } else {
            // Default state is nil
            emitter_.emit_abc(OpCode::OP_LOADNIL, state_reg, 0, 0);
        }

        if (expressions.size() >= 3) {
            // Third expression is the initial control variable
            expressions[2]->accept(*this);
            emitter_.emit_abc(OpCode::OP_MOVE, control_reg, current_result_register_.value_or(0), 0);
            if (current_result_register_.has_value()) {
                register_allocator_.free(current_result_register_.value());
            }
        } else {
            // Default control variable is nil
            emitter_.emit_abc(OpCode::OP_LOADNIL, control_reg, 0, 0);
        }

        // Allocate registers for loop variables
        std::vector<Register> var_registers;
        for (const auto& var_name : node.variables()) {
            auto var_reg_result = register_allocator_.allocate();
            if (is_success(var_reg_result)) {
                Register var_reg = get_value(var_reg_result);
                var_registers.push_back(var_reg);
                scope_manager_.declare_local(var_name, var_reg);
            }
        }

        // Mark loop start
        Size loop_start = jump_manager_.current_instruction();

        // Call iterator function: iter(state, control)
        // For now, use a simplified approach
        emitter_.emit_abc(OpCode::OP_CALL, iter_reg, 3, static_cast<Register>(var_registers.size() + 1));

        // Test if first return value is nil (end of iteration)
        if (!var_registers.empty()) {
            emitter_.emit_abc(OpCode::OP_TEST, var_registers[0], 0, 0);
            Size exit_jump = jump_manager_.emit_jump();  // Forward jump to loop end

            // Generate loop body
            node.body().accept(*this);

            // Jump back to loop start
            jump_manager_.emit_jump(loop_start);

            // Patch exit jump to here (after loop)
            Size loop_end = jump_manager_.current_instruction();
            jump_manager_.patch_jump(exit_jump, loop_end);
        }

        // Exit scope
        scope_manager_.exit_scope();

        // Free registers
        register_allocator_.free(iter_reg);
        register_allocator_.free(state_reg);
        register_allocator_.free(control_reg);
        for (Register var_reg : var_registers) {
            register_allocator_.free(var_reg);
        }
    }

    void CodeGenerator::visit(const frontend::RepeatStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for repeat statement");

        // Mark loop start
        Size loop_start = jump_manager_.current_instruction();

        // Generate loop body first (repeat-until executes body at least once)
        node.body().accept(*this);

        // Generate condition
        node.condition().accept(*this);
        Register condition_reg = current_result_register_.value_or(0);

        // Test condition and jump back to start if false (continue loop)
        emitter_.emit_abc(OpCode::OP_TEST, condition_reg, 1, 0);  // Test for false
        jump_manager_.emit_jump(loop_start);                      // Jump back to loop start

        // Free condition register
        register_allocator_.free(condition_reg);
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

    void CodeGenerator::visit([[maybe_unused]] const frontend::BreakStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for break statement");

        // TODO: Implement proper loop context tracking
        // For now, emit a forward jump that will need to be patched by the enclosing loop
        Size break_jump = jump_manager_.emit_jump();

        // In a complete implementation, we would:
        // 1. Check if we're inside a loop
        // 2. Add this jump to the loop's break list
        // 3. The loop would patch all break jumps when it ends

        // For now, just log that we need loop context
        CODEGEN_LOG_DEBUG("Break statement emitted jump index: {}", break_jump);
    }

    void CodeGenerator::visit(const frontend::GotoStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for goto statement: {}", node.label());

        // TODO: Implement proper label resolution
        // For now, emit a forward jump that will need to be resolved later
        Size goto_jump = jump_manager_.emit_jump();

        // In a complete implementation, we would:
        // 1. Check if the label has already been defined (backward jump)
        // 2. If yes, calculate the offset and emit the jump
        // 3. If no, add to pending goto list for later resolution

        CODEGEN_LOG_DEBUG("Goto statement to '{}' emitted jump index: {}", node.label(), goto_jump);
    }

    void CodeGenerator::visit(const frontend::LabelStatement& node) {
        CODEGEN_LOG_DEBUG("Generating code for label statement: {}", node.name());

        // Record the current instruction position as the label target
        Size label_position = jump_manager_.current_instruction();

        // TODO: Implement proper label tracking
        // In a complete implementation, we would:
        // 1. Add this label to the label table with its position
        // 2. Resolve any pending goto statements that reference this label
        // 3. Patch the jump instructions to point to this position

        CODEGEN_LOG_DEBUG("Label '{}' defined at instruction: {}", node.name(), label_position);
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

    BytecodeEmitter& CodeGenerator::emitter() noexcept {
        return emitter_;
    }

    // Helper method implementations
    Register CodeGenerator::emit_load_constant(const frontend::LiteralExpression::Value& value) {
        auto reg_result = register_allocator_.allocate();
        if (is_error(reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for constant");
            return 0;
        }

        Register result_reg = get_value(reg_result);

        // Generate appropriate load instruction based on literal type
        std::visit(
            [this, result_reg, &value](const auto& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    emitter_.emit_abc(OpCode::OP_LOADNIL, result_reg, 0, 0);
                } else if constexpr (std::is_same_v<T, bool>) {
                    if (val) {
                        emitter_.emit_abc(OpCode::OP_LOADTRUE, result_reg, 0, 0);
                    } else {
                        emitter_.emit_abc(OpCode::OP_LOADFALSE, result_reg, 0, 0);
                    }
                } else if constexpr (std::is_same_v<T, Int>) {
                    if (val >= -32768 && val <= 32767) {
                        emitter_.emit_asbx(
                            OpCode::OP_LOADI, result_reg, static_cast<std::int32_t>(val));
                    } else {
                        Size const_index = add_constant(value);
                        emitter_.emit_abx(
                            OpCode::OP_LOADK, result_reg, static_cast<std::uint32_t>(const_index));
                    }
                } else {
                    Size const_index = add_constant(value);
                    emitter_.emit_abx(
                        OpCode::OP_LOADK, result_reg, static_cast<std::uint32_t>(const_index));
                }
            },
            value);

        return result_reg;
    }

    Register CodeGenerator::emit_binary_operation(frontend::BinaryOpExpression::Operator op,
                                                  Register left,
                                                  Register right) {
        auto reg_result = register_allocator_.allocate();
        if (is_error(reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for binary operation");
            return 0;
        }

        Register result_reg = get_value(reg_result);

        switch (op) {
            case frontend::BinaryOpExpression::Operator::Add:
                emitter_.emit_abc(OpCode::OP_ADD, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::Subtract:
                emitter_.emit_abc(OpCode::OP_SUB, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::Multiply:
                emitter_.emit_abc(OpCode::OP_MUL, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::Divide:
                emitter_.emit_abc(OpCode::OP_DIV, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::IntegerDivide:
                emitter_.emit_abc(OpCode::OP_IDIV, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::Modulo:
                emitter_.emit_abc(OpCode::OP_MOD, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::Power:
                emitter_.emit_abc(OpCode::OP_POW, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::BitwiseAnd:
                emitter_.emit_abc(OpCode::OP_BAND, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::BitwiseOr:
                emitter_.emit_abc(OpCode::OP_BOR, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::BitwiseXor:
                emitter_.emit_abc(OpCode::OP_BXOR, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::ShiftLeft:
                emitter_.emit_abc(OpCode::OP_SHL, result_reg, left, right);
                break;
            case frontend::BinaryOpExpression::Operator::ShiftRight:
                emitter_.emit_abc(OpCode::OP_SHR, result_reg, left, right);
                break;
            default:
                CODEGEN_LOG_ERROR("Unsupported binary operator in helper");
                break;
        }

        return result_reg;
    }

    Register CodeGenerator::emit_unary_operation(frontend::UnaryOpExpression::Operator op,
                                                 Register operand) {
        auto reg_result = register_allocator_.allocate();
        if (is_error(reg_result)) {
            CODEGEN_LOG_ERROR("Failed to allocate register for unary operation");
            return 0;
        }

        Register result_reg = get_value(reg_result);

        switch (op) {
            case frontend::UnaryOpExpression::Operator::Minus:
                emitter_.emit_abc(OpCode::OP_UNM, result_reg, operand, 0);
                break;
            case frontend::UnaryOpExpression::Operator::Not:
                emitter_.emit_abc(OpCode::OP_NOT, result_reg, operand, 0);
                break;
            case frontend::UnaryOpExpression::Operator::Length:
                emitter_.emit_abc(OpCode::OP_LEN, result_reg, operand, 0);
                break;
            case frontend::UnaryOpExpression::Operator::BitwiseNot:
                emitter_.emit_abc(OpCode::OP_BNOT, result_reg, operand, 0);
                break;
            default:
                CODEGEN_LOG_ERROR("Unsupported unary operator in helper");
                break;
        }

        return result_reg;
    }

    void CodeGenerator::emit_assignment(Register target, Register source) {
        emitter_.emit_abc(OpCode::OP_MOVE, target, source, 0);
    }

    void CodeGenerator::emit_conditional_jump(Register condition, Size target) {
        emitter_.emit_abc(OpCode::OP_TEST, condition, 0, 0);
        if (target == 0) {
            jump_manager_.emit_jump();
        } else {
            jump_manager_.emit_jump(target);
        }
    }

    Size CodeGenerator::add_constant(const frontend::LiteralExpression::Value& value) {
        // Convert frontend literal value to bytecode constant value
        ConstantValue constant_value = to_constant_value(value);

        // Use the emitter's add_constant method
        return emitter_.add_constant(constant_value);
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
