#pragma once

/**
 * @file optimizer.hpp
 * @brief Bytecode optimization passes and transformations
 * @version 0.1.0
 */

#include "../core/types.hpp"
#include "../core/error.hpp"
#include "../core/instruction.hpp"
#include "../core/concepts.hpp"
#include "bytecode.hpp"

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace rangelua::backend {

/**
 * @brief Optimization pass interface
 */
class OptimizationPass {
public:
    virtual ~OptimizationPass() = default;

    // Non-copyable, movable
    OptimizationPass(const OptimizationPass&) = delete;
    OptimizationPass& operator=(const OptimizationPass&) = delete;
    OptimizationPass(OptimizationPass&&) noexcept = default;
    OptimizationPass& operator=(OptimizationPass&&) noexcept = default;

    /**
     * @brief Apply optimization to bytecode function
     * @param function Bytecode function to optimize
     * @return Success or error
     */
    virtual Status optimize(BytecodeFunction& function) = 0;

    /**
     * @brief Get pass name
     * @return Pass name
     */
    [[nodiscard]] virtual StringView name() const noexcept = 0;

    /**
     * @brief Check if pass modifies bytecode
     * @return true if pass modifies bytecode
     */
    [[nodiscard]] virtual bool is_transformative() const noexcept = 0;

protected:
    OptimizationPass() = default;
};

/**
 * @brief Constant folding optimization pass
 */
class ConstantFoldingPass : public OptimizationPass {
public:
    Status optimize(BytecodeFunction& function) override;
    [[nodiscard]] StringView name() const noexcept override { return "constant-folding"; }
    [[nodiscard]] bool is_transformative() const noexcept override { return true; }

private:
    struct ConstantValue {
        using ValueType = std::variant<std::monostate, Int, Number, String, bool>;
        ValueType value;

        ConstantValue() : value(std::monostate{}) {}
        explicit ConstantValue(Int i) : value(i) {}
        explicit ConstantValue(Number n) : value(n) {}
        explicit ConstantValue(String s) : value(std::move(s)) {}
        explicit ConstantValue(bool b) : value(b) {}

        template<typename T>
        [[nodiscard]] bool holds() const noexcept {
            return std::holds_alternative<T>(value);
        }

        template<typename T>
        [[nodiscard]] const T& get() const {
            return std::get<T>(value);
        }

        [[nodiscard]] bool is_nil() const noexcept { return holds<std::monostate>(); }
        [[nodiscard]] bool is_integer() const noexcept { return holds<Int>(); }
        [[nodiscard]] bool is_number() const noexcept { return holds<Number>(); }
        [[nodiscard]] bool is_string() const noexcept { return holds<String>(); }
        [[nodiscard]] bool is_boolean() const noexcept { return holds<bool>(); }
    };

    Optional<ConstantValue> evaluate_binary_op(OpCode op, const ConstantValue& left, const ConstantValue& right);
    Optional<ConstantValue> evaluate_unary_op(OpCode op, const ConstantValue& operand);
    [[nodiscard]] bool is_constant_instruction(OpCode op) const noexcept;
    Optional<ConstantValue> get_constant_value(const BytecodeFunction& function, Register reg);
};

/**
 * @brief Dead code elimination pass
 */
class DeadCodeEliminationPass : public OptimizationPass {
public:
    Status optimize(BytecodeFunction& function) override;
    [[nodiscard]] StringView name() const noexcept override { return "dead-code-elimination"; }
    [[nodiscard]] bool is_transformative() const noexcept override { return true; }

private:
    std::unordered_set<Size> find_reachable_instructions(const BytecodeFunction& function);
    std::unordered_set<Register> find_live_registers(const BytecodeFunction& function);
    [[nodiscard]] bool is_side_effect_free(OpCode op) const noexcept;
    [[nodiscard]] bool is_control_flow_instruction(OpCode op) const noexcept;
};

/**
 * @brief Peephole optimization pass
 */
class PeepholeOptimizationPass : public OptimizationPass {
public:
    Status optimize(BytecodeFunction& function) override;
    [[nodiscard]] StringView name() const noexcept override { return "peephole-optimization"; }
    [[nodiscard]] bool is_transformative() const noexcept override { return true; }

private:
    struct Pattern {
        std::vector<OpCode> opcodes;
        std::function<bool(const std::vector<Instruction>&)> condition;
        std::function<std::vector<Instruction>(const std::vector<Instruction>&)> replacement;
    };

    std::vector<Pattern> patterns_;
    void initialize_patterns();
    bool apply_pattern(std::vector<Instruction>& instructions, Size start, const Pattern& pattern);
};

/**
 * @brief Register allocation optimization pass
 */
class RegisterOptimizationPass : public OptimizationPass {
public:
    Status optimize(BytecodeFunction& function) override;
    [[nodiscard]] StringView name() const noexcept override { return "register-optimization"; }
    [[nodiscard]] bool is_transformative() const noexcept override { return true; }

private:
    struct RegisterInfo {
        Size first_use = SIZE_MAX;
        Size last_use = 0;
        std::vector<Size> uses;
        bool is_parameter = false;
        bool is_local = false;
    };

    std::unordered_map<Register, RegisterInfo> analyze_register_usage(const BytecodeFunction& function);
    std::unordered_map<Register, Register> compute_register_mapping(const std::unordered_map<Register, RegisterInfo>& usage);
    void remap_registers(BytecodeFunction& function, const std::unordered_map<Register, Register>& mapping);
};

/**
 * @brief Jump optimization pass
 */
class JumpOptimizationPass : public OptimizationPass {
public:
    Status optimize(BytecodeFunction& function) override;
    [[nodiscard]] StringView name() const noexcept override { return "jump-optimization"; }
    [[nodiscard]] bool is_transformative() const noexcept override { return true; }

private:
    bool optimize_jump_chains(BytecodeFunction& function);
    bool eliminate_redundant_jumps(BytecodeFunction& function);
    bool optimize_conditional_jumps(BytecodeFunction& function);
    Size follow_jump_chain(const BytecodeFunction& function, Size start);
};

/**
 * @brief Tail call optimization pass
 */
class TailCallOptimizationPass : public OptimizationPass {
public:
    Status optimize(BytecodeFunction& function) override;
    [[nodiscard]] StringView name() const noexcept override { return "tail-call-optimization"; }
    [[nodiscard]] bool is_transformative() const noexcept override { return true; }

private:
    bool is_tail_position(const BytecodeFunction& function, Size instruction_index);
    bool can_optimize_call(const BytecodeFunction& function, Size call_index);
    void convert_to_tail_call(BytecodeFunction& function, Size call_index);
};

/**
 * @brief Main optimizer class that manages optimization passes
 */
class Optimizer {
public:
    enum class OptimizationLevel : std::uint8_t {
        None = 0,
        Basic = 1,
        Standard = 2,
        Aggressive = 3
    };

    explicit Optimizer(OptimizationLevel level = OptimizationLevel::Standard);

    ~Optimizer() = default;

    // Non-copyable, movable
    Optimizer(const Optimizer&) = delete;
    Optimizer& operator=(const Optimizer&) = delete;
    Optimizer(Optimizer&&) noexcept = default;
    Optimizer& operator=(Optimizer&&) noexcept = delete;

    /**
     * @brief Optimize bytecode function
     * @param function Function to optimize
     * @return Success or error
     */
    Status optimize(BytecodeFunction& function);

    /**
     * @brief Add custom optimization pass
     * @param pass Optimization pass
     */
    void add_pass(UniquePtr<OptimizationPass> pass);

    /**
     * @brief Remove optimization pass by name
     * @param name Pass name
     */
    void remove_pass(StringView name);

    /**
     * @brief Set optimization level
     * @param level Optimization level
     */
    void set_optimization_level(OptimizationLevel level);

    /**
     * @brief Get optimization level
     * @return Current optimization level
     */
    [[nodiscard]] OptimizationLevel optimization_level() const noexcept;

    /**
     * @brief Enable/disable specific optimization
     * @param name Pass name
     * @param enabled Enable flag
     */
    void set_pass_enabled(StringView name, bool enabled);

    /**
     * @brief Check if pass is enabled
     * @param name Pass name
     * @return true if enabled
     */
    [[nodiscard]] bool is_pass_enabled(StringView name) const noexcept;

    /**
     * @brief Get optimization statistics
     * @return Statistics map
     */
    [[nodiscard]] const std::unordered_map<String, Size>& statistics() const noexcept;

    /**
     * @brief Reset statistics
     */
    void reset_statistics();

private:
    OptimizationLevel level_;
    std::vector<UniquePtr<OptimizationPass>> passes_;
    std::unordered_map<String, bool> pass_enabled_;
    std::unordered_map<String, Size> statistics_;

    void initialize_default_passes();
    void configure_passes_for_level(OptimizationLevel level);
};

/**
 * @brief Optimization analysis utilities
 */
namespace optimization_analysis {

/**
 * @brief Control flow graph node
 */
struct CFGNode {
    Size start_instruction;
    Size end_instruction;
    std::vector<Size> predecessors;
    std::vector<Size> successors;
    std::unordered_set<Register> live_in;
    std::unordered_set<Register> live_out;
    std::unordered_set<Register> def;
    std::unordered_set<Register> use;
};

/**
 * @brief Control flow graph
 */
class ControlFlowGraph {
public:
    explicit ControlFlowGraph(const BytecodeFunction& function);

    [[nodiscard]] const std::vector<CFGNode>& nodes() const noexcept { return nodes_; }
    [[nodiscard]] Size node_count() const noexcept { return nodes_.size(); }

    void compute_liveness();
    void compute_dominators();

    [[nodiscard]] const std::vector<Size>& dominators() const noexcept { return dominators_; }
    [[nodiscard]] const std::vector<std::vector<Size>>& dominator_tree() const noexcept { return dominator_tree_; }

private:
    std::vector<CFGNode> nodes_;
    std::vector<Size> dominators_;
    std::vector<std::vector<Size>> dominator_tree_;

    void build_cfg(const BytecodeFunction& function);
    void compute_def_use_sets(const BytecodeFunction& function);
};

/**
 * @brief Data flow analysis utilities
 */
class DataFlowAnalysis {
public:
    static std::unordered_map<Register, std::vector<Size>> compute_reaching_definitions(const BytecodeFunction& function);
    static std::unordered_set<Register> compute_live_variables(const BytecodeFunction& function, Size instruction);
    static std::unordered_map<Register, std::vector<Size>> compute_use_def_chains(const BytecodeFunction& function);
};

} // namespace optimization_analysis

} // namespace rangelua::backend

// Concept verification (commented out until concepts are properly included)
// static_assert(rangelua::concepts::Optimizer<rangelua::backend::Optimizer>);
