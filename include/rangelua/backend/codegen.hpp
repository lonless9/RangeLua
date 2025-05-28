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
     * @brief Expression descriptor types (similar to Lua 5.5's expdesc)
     *
     * These represent different ways an expression's value can be stored/accessed
     */
    enum class ExpressionKind {
        VOID,      // No value (empty expression list)
        NIL,       // Constant nil
        TRUE,      // Constant true
        FALSE,     // Constant false
        KINT,      // Integer constant (immediate)
        KFLT,      // Float constant (immediate)
        KSTR,      // String constant (immediate)
        K,         // Constant in constant table
        NONRELOC,  // Value in fixed register
        RELOC,     // Value can be relocated to any register
        LOCAL,     // Local variable
        UPVAL,     // Upvalue
        GLOBAL,    // Global variable
        INDEXED,   // Table access t[k]
        CALL,      // Function call result
        VARARG     // Vararg expression
    };

    /**
     * @brief Expression descriptor (similar to Lua 5.5's expdesc)
     *
     * Represents an expression and how its value is stored/accessed
     */
    struct ExpressionDesc {
        ExpressionKind kind = ExpressionKind::VOID;

        union {
            Int ival;               // For KINT
            Number nval;            // For KFLT
            Size info;              // Generic info (register, constant index, etc.)
            struct {                // For indexed access
                Register table;     // Table register
                Register key;       // Key register or constant index
                bool is_const_key;  // True if key is constant
                bool is_int_key;    // True if key is integer constant (for GETI)
                bool is_string_key;  // True if key is string constant (for GETFIELD)
            } indexed;
        } u{};

        // Jump lists for conditional expressions
        std::vector<Size> true_list;   // Jump when true
        std::vector<Size> false_list;  // Jump when false

        ExpressionDesc() = default;
        explicit ExpressionDesc(ExpressionKind k) : kind(k) {}
    };

    /**
     * @brief Register allocation and management (Lua 5.5 style)
     *
     * This class implements Lua 5.5's register allocation strategy:
     * - Uses freereg pointer for efficient allocation
     * - Supports immediate register reuse
     * - Tracks local variable registers separately
     * - Optimizes for expression evaluation patterns
     */
    class RegisterAllocator {
    public:
        explicit RegisterAllocator(Size max_registers = 255);

        /**
         * @brief Reserve n registers starting from freereg
         * @param n Number of registers to reserve
         * @return Starting register index
         */
        Register reserve_registers(Size n);

        /**
         * @brief Get next free register without allocating
         * @return Next free register index
         */
        [[nodiscard]] Register next_free() const noexcept { return free_reg_; }

        /**
         * @brief Free register if it's not a local variable
         * @param reg Register to free
         * @param local_count Number of local variables (registers 0..local_count-1 are locals)
         */
        void free_register(Register reg, Size local_count);

        /**
         * @brief Free two registers in proper order
         * @param r1 First register
         * @param r2 Second register
         * @param local_count Number of local variables
         */
        void free_registers(Register r1, Register r2, Size local_count);

        /**
         * @brief Check and update stack size
         * @param needed Number of additional registers needed
         */
        void check_stack(Size needed);

        /**
         * @brief Get the highest used register (stack size)
         * @return Maximum stack size needed
         */
        [[nodiscard]] Register stack_size() const noexcept { return max_stack_size_; }

        /**
         * @brief Reset allocator state
         */
        void reset();

        /**
         * @brief Set number of local variables (affects which registers can be freed)
         * @param count Number of local variables
         */
        void set_local_count(Size count) noexcept { local_count_ = count; }

        /**
         * @brief Get number of local variables
         * @return Number of local variables
         */
        [[nodiscard]] Size local_count() const noexcept { return local_count_; }

        // Temporary compatibility methods for migration
        /**
         * @brief Allocate a single register (compatibility method)
         * @return Register index or error
         */
        Result<Register> allocate();

        /**
         * @brief Free a register (compatibility method)
         * @param reg Register to free
         */
        void free(Register reg);

        /**
         * @brief Get high water mark (compatibility method)
         * @return Maximum stack size reached
         */
        [[nodiscard]] Register high_water_mark() const noexcept;

    private:
        Size max_registers_;
        Register free_reg_;        // First free register (Lua's freereg)
        Register max_stack_size_;  // Maximum stack size reached
        Size local_count_;         // Number of local variables
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

        /**
         * @brief Get bytecode emitter
         * @return Bytecode emitter reference
         */
        BytecodeEmitter& emitter() noexcept;

    private:
        BytecodeEmitter& emitter_;
        RegisterAllocator register_allocator_;
        JumpManager jump_manager_;
        ScopeManager scope_manager_;

        // Current expression result (for visitor pattern)
        Optional<ExpressionDesc> current_expression_;

        // Temporary compatibility member for migration
        Optional<Register> current_result_register_;

        // Loop context management
        struct LoopContext {
            JumpManager::JumpList break_jumps;
            JumpManager::JumpList continue_jumps;
            Size loop_start;
            Size scope_depth;
        };
        std::vector<LoopContext> loop_stack_;

        // Label management for goto statements
        struct LabelInfo {
            String name;
            Size position;
            Size scope_depth;
        };
        std::vector<LabelInfo> labels_;
        std::vector<std::pair<String, Size>> pending_gotos_;  // label name, jump index

        // Expression management methods (Lua 5.5 style)
        void discharge_vars(ExpressionDesc& expr);
        void discharge_to_register(ExpressionDesc& expr, Register reg);
        void discharge_to_any_register(ExpressionDesc& expr);
        Register expression_to_any_register(ExpressionDesc& expr);
        Register expression_to_next_register(ExpressionDesc& expr);
        void expression_to_register(ExpressionDesc& expr, Register reg);
        void free_expression(ExpressionDesc& expr);
        void free_expressions(ExpressionDesc& e1, ExpressionDesc& e2);

        // Constant and optimization methods
        bool try_constant_folding(frontend::BinaryOpExpression::Operator op,
                                  ExpressionDesc& e1,
                                  const ExpressionDesc& e2);
        bool expression_to_constant(ExpressionDesc& expr);
        Size add_constant(const frontend::LiteralExpression::Value& value);

        // Helper methods
        void emit_load_constant(ExpressionDesc& expr,
                                const frontend::LiteralExpression::Value& value);
        void emit_binary_operation(ExpressionDesc& result,
                                   frontend::BinaryOpExpression::Operator op,
                                   ExpressionDesc& left,
                                   ExpressionDesc& right);
        void emit_unary_operation(ExpressionDesc& result,
                                  frontend::UnaryOpExpression::Operator op,
                                  ExpressionDesc& operand);
        void emit_assignment(const ExpressionDesc& target, ExpressionDesc& source);
        void emit_conditional_jump(ExpressionDesc& condition, Size target);
        void generate_comparison_with_result(Register left_reg,
                                             Register right_reg,
                                             Register result_reg,
                                             OpCode comparison_op,
                                             bool negate);
        void generate_logical_and(ExpressionDesc& left_expr,
                                  ExpressionDesc& right_expr,
                                  Register result_reg);
        void generate_logical_or(ExpressionDesc& left_expr,
                                 ExpressionDesc& right_expr,
                                 Register result_reg);

        // Loop context helpers
        void enter_loop(Size loop_start);
        void exit_loop();
        bool in_loop() const noexcept;
        void add_break_jump(Size jump_index);
        void add_continue_jump(Size jump_index);

        // Label management helpers
        void define_label(const String& name);
        void emit_goto(const String& label);
        void resolve_pending_gotos();
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
