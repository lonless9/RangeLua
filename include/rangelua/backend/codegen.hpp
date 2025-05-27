#pragma once

/**
 * @file codegen.hpp
 * @brief Code generation with complete control over register allocation and bytecode emission
 * @version 0.1.0
 */

#include <stack>
#include <unordered_map>
#include <vector>

#include "../core/error.hpp"
#include "../core/instruction.hpp"
#include "../core/types.hpp"
#include "../frontend/ast.hpp"
#include "bytecode.hpp"

namespace rangelua::backend {

    /**
     * @brief Register allocation and management
     *
     * This class has COMPLETE control over register allocation, lifetime management,
     * and reuse. The parser should NEVER touch register allocation logic.
     */
    class RegisterAllocator {
    public:
        explicit RegisterAllocator(Size max_registers = 255);

        /**
         * @brief Allocate a new register
         * @return Register index or error if no registers available
         */
        Result<Register> allocate();

        /**
         * @brief Allocate a specific register
         * @param reg Desired register
         * @return Success or error if register unavailable
         */
        Status allocate_specific(Register reg);

        /**
         * @brief Free a register for reuse
         * @param reg Register to free
         */
        void free(Register reg);

        /**
         * @brief Reserve a register (cannot be allocated)
         * @param reg Register to reserve
         */
        void reserve(Register reg);

        /**
         * @brief Get the highest allocated register
         * @return Highest register in use
         */
        [[nodiscard]] Register high_water_mark() const noexcept;

        /**
         * @brief Get number of allocated registers
         * @return Number of registers in use
         */
        [[nodiscard]] Size allocated_count() const noexcept;

        /**
         * @brief Check if register is allocated
         * @param reg Register to check
         * @return true if allocated
         */
        [[nodiscard]] bool is_allocated(Register reg) const noexcept;

        /**
         * @brief Reset allocator state
         */
        void reset();

    private:
        Size max_registers_;
        std::vector<bool> allocated_;
        std::vector<bool> reserved_;
        std::stack<Register> free_list_;
        Register next_register_;
        Register high_water_;
    };

    /**
     * @brief Jump list management for control flow
     *
     * Manages forward and backward jumps, patch lists, and control flow structures.
     * This is entirely separate from parser logic.
     */
    class JumpManager {
    public:
        using JumpList = std::vector<Size>;
        using PatchList = std::vector<std::pair<Size, Size>>;

        /**
         * @brief Create a new jump instruction
         * @param target Target instruction index (or 0 for forward jump)
         * @return Jump instruction index
         */
        Size emit_jump(Size target = 0);

        /**
         * @brief Create a conditional jump instruction
         * @param condition_reg Register containing condition
         * @param target Target instruction index (or 0 for forward jump)
         * @return Jump instruction index
         */
        Size emit_conditional_jump(Register condition_reg, Size target = 0);

        /**
         * @brief Patch a forward jump with target
         * @param jump_index Jump instruction index
         * @param target Target instruction index
         */
        void patch_jump(Size jump_index, Size target);

        /**
         * @brief Patch all jumps in a list
         * @param jumps List of jump indices
         * @param target Target instruction index
         */
        void patch_jump_list(const JumpList& jumps, Size target);

        /**
         * @brief Create a new jump list
         * @return Empty jump list
         */
        JumpList create_jump_list();

        /**
         * @brief Merge two jump lists
         * @param list1 First jump list
         * @param list2 Second jump list
         * @return Merged jump list
         */
        JumpList merge_jump_lists(JumpList list1, const JumpList& list2);

        /**
         * @brief Get current instruction index
         * @return Current instruction index
         */
        [[nodiscard]] Size current_instruction() const noexcept;

        /**
         * @brief Set the bytecode emitter
         * @param emitter Pointer to bytecode emitter
         */
        void set_emitter(BytecodeEmitter* emitter) noexcept;

    private:
        BytecodeEmitter* emitter_ = nullptr;
        PatchList pending_patches_;
    };

    /**
     * @brief Scope management for local variables and upvalues
     */
    class ScopeManager {
    public:
        struct LocalVariable {
            String name;
            Register reg;
            Size start_pc;
            Size end_pc;
            bool is_captured;
        };

        struct Upvalue {
            String name;
            UpvalueIndex index;
            bool is_local;
            Register local_reg;
        };

        /**
         * @brief Enter a new scope
         */
        void enter_scope();

        /**
         * @brief Exit current scope
         */
        void exit_scope();

        /**
         * @brief Declare a local variable
         * @param name Variable name
         * @param reg Register assigned to variable
         * @return Local variable index
         */
        Size declare_local(String name, Register reg);

        /**
         * @brief Variable resolution result
         */
        struct VariableResolution {
            enum class Type { Local, Upvalue, Global };
            Type type;
            std::uint8_t index;  // Register for local, upvalue index for upvalue, unused for global
        };

        /**
         * @brief Resolve a variable name
         * @param name Variable name
         * @return Variable resolution result
         */
        VariableResolution resolve_variable(const String& name);

        /**
         * @brief Get current scope depth
         * @return Scope depth
         */
        Size scope_depth() const noexcept;

        /**
         * @brief Get locals in current scope
         * @return Vector of local variables
         */
        const std::vector<LocalVariable>& current_locals() const noexcept;

    private:
        struct Scope {
            Size start_local;
            Size depth;
        };

        std::vector<Scope> scopes_;
        std::vector<LocalVariable> locals_;
        std::vector<Upvalue> upvalues_;
        std::unordered_map<String, Size> local_names_;
    };

    /**
     * @brief Main code generator class
     *
     * This class has COMPLETE responsibility for:
     * - Register allocation and lifetime management
     * - Jump list handling and patching
     * - Bytecode emission and optimization
     * - Local variable and upvalue management
     *
     * The parser should ONLY provide AST nodes and NEVER interfere with
     * low-level code generation concerns.
     */
    class CodeGenerator : public frontend::ASTVisitor {
    public:
        explicit CodeGenerator(BytecodeEmitter& emitter);

        ~CodeGenerator() = default;

        // Non-copyable, movable
        CodeGenerator(const CodeGenerator&) = delete;
        CodeGenerator& operator=(const CodeGenerator&) = delete;
        CodeGenerator(CodeGenerator&&) noexcept = default;
        CodeGenerator& operator=(CodeGenerator&&) noexcept = delete;

        /**
         * @brief Generate code for AST
         * @param ast Program AST
         * @return Success or error
         */
        Status generate(const frontend::Program& ast);

        /**
         * @brief Generate code for expression
         * @param expr Expression AST
         * @return Register containing result or error
         */
        Result<Register> generate_expression(const frontend::Expression& expr);

        /**
         * @brief Generate code for statement
         * @param stmt Statement AST
         * @return Success or error
         */
        Status generate_statement(const frontend::Statement& stmt);

        // AST Visitor implementation - complete Lua 5.5 support
        void visit(const frontend::LiteralExpression& node) override;
        void visit(const frontend::IdentifierExpression& node) override;
        void visit(const frontend::BinaryOpExpression& node) override;
        void visit(const frontend::UnaryOpExpression& node) override;
        void visit(const frontend::FunctionCallExpression& node) override;
        void visit(const frontend::MethodCallExpression& node) override;
        void visit(const frontend::TableAccessExpression& node) override;
        void visit(const frontend::TableConstructorExpression& node) override;
        void visit(const frontend::FunctionExpression& node) override;
        void visit(const frontend::VarargExpression& node) override;
        void visit(const frontend::ParenthesizedExpression& node) override;
        void visit(const frontend::BlockStatement& node) override;
        void visit(const frontend::AssignmentStatement& node) override;
        void visit(const frontend::LocalDeclarationStatement& node) override;
        void visit(const frontend::FunctionDeclarationStatement& node) override;
        void visit(const frontend::IfStatement& node) override;
        void visit(const frontend::WhileStatement& node) override;
        void visit(const frontend::ForNumericStatement& node) override;
        void visit(const frontend::ForGenericStatement& node) override;
        void visit(const frontend::RepeatStatement& node) override;
        void visit(const frontend::DoStatement& node) override;
        void visit(const frontend::ReturnStatement& node) override;
        void visit(const frontend::BreakStatement& node) override;
        void visit(const frontend::GotoStatement& node) override;
        void visit(const frontend::LabelStatement& node) override;
        void visit(const frontend::ExpressionStatement& node) override;
        void visit(const frontend::Program& node) override;

        /**
         * @brief Get register allocator
         * @return Register allocator reference
         */
        RegisterAllocator& register_allocator() noexcept;

        /**
         * @brief Get jump manager
         * @return Jump manager reference
         */
        JumpManager& jump_manager() noexcept;

        /**
         * @brief Get scope manager
         * @return Scope manager reference
         */
        ScopeManager& scope_manager() noexcept;

    private:
        BytecodeEmitter& emitter_;
        RegisterAllocator register_allocator_;
        JumpManager jump_manager_;
        ScopeManager scope_manager_;

        // Current expression result register (for visitor pattern)
        Optional<Register> current_result_register_;

        // Helper methods
        Register emit_load_constant(const frontend::LiteralExpression::Value& value);
        Register emit_binary_operation(frontend::BinaryOpExpression::Operator op,
                                       Register left,
                                       Register right);
        Register emit_unary_operation(frontend::UnaryOpExpression::Operator op, Register operand);
        void emit_assignment(Register target, Register source);
        void emit_conditional_jump(Register condition, Size target);

        // Constant management
        Size add_constant(const frontend::LiteralExpression::Value& value);
    };

    /**
     * @brief Code generation context for nested functions
     */
    class CodeGenContext {
    public:
        explicit CodeGenContext(CodeGenerator& generator);

        /**
         * @brief Enter function context
         * @param name Function name
         * @param parameters Parameter list
         */
        void enter_function(const String& name, const std::vector<String>& parameters);

        /**
         * @brief Exit function context
         * @return Generated function bytecode
         */
        BytecodeFunction exit_function();

        /**
         * @brief Get current function name
         * @return Function name
         */
        [[nodiscard]] const String& function_name() const noexcept;

        /**
         * @brief Check if in function context
         * @return true if in function
         */
        [[nodiscard]] bool in_function() const noexcept;

    private:
        CodeGenerator& generator_;
        std::stack<String> function_stack_;
        std::stack<Size> scope_stack_;
    };

}  // namespace rangelua::backend

// Concept verification (commented out until concepts are properly included)
// static_assert(rangelua::concepts::CodeGenerator<rangelua::backend::CodeGenerator>);
