/**
 * @file optimizer.cpp
 * @brief Comprehensive bytecode optimization implementation with modern C++20 features
 * @version 0.1.0
 */

#include <rangelua/backend/optimizer.hpp>
#include <rangelua/utils/logger.hpp>

#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <queue>
#include <stack>
#include <chrono>

namespace rangelua::backend {

    // ConstantFoldingPass Implementation
    Status ConstantFoldingPass::optimize(BytecodeFunction& function) {
        OPTIMIZER_LOG_DEBUG("Starting constant folding optimization");

        bool changed = false;
        auto& instructions = function.instructions;

        // Track constant values for each register
        std::unordered_map<Register, ConstantValue> constants;

        for (Size i = 0; i < instructions.size(); ++i) {
            Instruction instr = instructions[i];
            OpCode op = InstructionEncoder::decode_opcode(instr);
            Register a = InstructionEncoder::decode_a(instr);

            switch (op) {
                case OpCode::OP_LOADI: {
                    std::int32_t value = InstructionEncoder::decode_sbx(instr);
                    constants[a] = ConstantValue{static_cast<Int>(value)};
                    break;
                }

                case OpCode::OP_LOADF: {
                    std::int32_t value = InstructionEncoder::decode_sbx(instr);
                    constants[a] = ConstantValue{static_cast<Number>(value)};
                    break;
                }

                case OpCode::OP_LOADTRUE: {
                    constants[a] = ConstantValue{true};
                    break;
                }

                case OpCode::OP_LOADFALSE: {
                    constants[a] = ConstantValue{false};
                    break;
                }

                case OpCode::OP_LOADNIL: {
                    constants[a] = ConstantValue{};
                    break;
                }

                case OpCode::OP_ADD:
                case OpCode::OP_SUB:
                case OpCode::OP_MUL:
                case OpCode::OP_DIV:
                case OpCode::OP_MOD:
                case OpCode::OP_POW: {
                    Register b = InstructionEncoder::decode_b(instr);
                    Register c = InstructionEncoder::decode_c(instr);

                    auto b_const = constants.find(b);
                    auto c_const = constants.find(c);

                    if (b_const != constants.end() && c_const != constants.end()) {
                        auto result = evaluate_binary_op(op, b_const->second, c_const->second);
                        if (result.has_value()) {
                            constants[a] = result.value();

                            // Replace with appropriate load instruction
                            if (result->is_integer()) {
                                Int int_val = result->get<Int>();
                                if (int_val >= -32768 && int_val <= 32767) {
                                    instructions[i] = InstructionEncoder::encode_asbx(
                                        OpCode::OP_LOADI, a, static_cast<std::int32_t>(int_val));
                                    changed = true;
                                    OPTIMIZER_LOG_DEBUG("Folded integer arithmetic to LOADI");
                                }
                            } else if (result->is_number()) {
                                Number num_val = result->get<Number>();
                                auto int_val = static_cast<std::int32_t>(num_val);
                                if (static_cast<Number>(int_val) == num_val &&
                                    int_val >= -32768 && int_val <= 32767) {
                                    instructions[i] = InstructionEncoder::encode_asbx(
                                        OpCode::OP_LOADF, a, int_val);
                                    changed = true;
                                    OPTIMIZER_LOG_DEBUG("Folded float arithmetic to LOADF");
                                }
                            }
                        }
                    } else {
                        constants.erase(a);
                    }
                    break;
                }

                case OpCode::OP_UNM:
                case OpCode::OP_NOT:
                case OpCode::OP_BNOT: {
                    Register b = InstructionEncoder::decode_b(instr);
                    auto b_const = constants.find(b);

                    if (b_const != constants.end()) {
                        auto result = evaluate_unary_op(op, b_const->second);
                        if (result.has_value()) {
                            constants[a] = result.value();

                            // Replace with appropriate load instruction
                            if (result->is_boolean()) {
                                bool bool_val = result->get<bool>();
                                instructions[i] = InstructionEncoder::encode_abc(
                                    bool_val ? OpCode::OP_LOADTRUE : OpCode::OP_LOADFALSE, a, 0, 0);
                                changed = true;
                                OPTIMIZER_LOG_DEBUG("Folded unary operation to boolean load");
                            }
                        }
                    } else {
                        constants.erase(a);
                    }
                    break;
                }

                default:
                    // For other instructions, invalidate the target register
                    if (InstructionEncoder::decode_a(instr) < 255) {
                        constants.erase(a);
                    }
                    break;
            }
        }

        OPTIMIZER_LOG_INFO("Constant folding completed, changed: {}", changed);
        return changed ? std::monostate{} : std::monostate{};
    }

    Optional<ConstantFoldingPass::ConstantValue>
    ConstantFoldingPass::evaluate_binary_op(OpCode op, const ConstantValue& left, const ConstantValue& right) {
        try {
            switch (op) {
                case OpCode::OP_ADD:
                    if (left.is_integer() && right.is_integer()) {
                        return ConstantValue{left.get<Int>() + right.get<Int>()};
                    } else if (left.is_number() && right.is_number()) {
                        return ConstantValue{left.get<Number>() + right.get<Number>()};
                    } else if (left.is_integer() && right.is_number()) {
                        return ConstantValue{static_cast<Number>(left.get<Int>()) + right.get<Number>()};
                    } else if (left.is_number() && right.is_integer()) {
                        return ConstantValue{left.get<Number>() + static_cast<Number>(right.get<Int>())};
                    }
                    break;

                case OpCode::OP_SUB:
                    if (left.is_integer() && right.is_integer()) {
                        return ConstantValue{left.get<Int>() - right.get<Int>()};
                    } else if (left.is_number() && right.is_number()) {
                        return ConstantValue{left.get<Number>() - right.get<Number>()};
                    } else if (left.is_integer() && right.is_number()) {
                        return ConstantValue{static_cast<Number>(left.get<Int>()) - right.get<Number>()};
                    } else if (left.is_number() && right.is_integer()) {
                        return ConstantValue{left.get<Number>() - static_cast<Number>(right.get<Int>())};
                    }
                    break;

                case OpCode::OP_MUL:
                    if (left.is_integer() && right.is_integer()) {
                        return ConstantValue{left.get<Int>() * right.get<Int>()};
                    } else if (left.is_number() && right.is_number()) {
                        return ConstantValue{left.get<Number>() * right.get<Number>()};
                    } else if (left.is_integer() && right.is_number()) {
                        return ConstantValue{static_cast<Number>(left.get<Int>()) * right.get<Number>()};
                    } else if (left.is_number() && right.is_integer()) {
                        return ConstantValue{left.get<Number>() * static_cast<Number>(right.get<Int>())};
                    }
                    break;

                case OpCode::OP_DIV:
                    if (right.is_integer() && right.get<Int>() == 0) return std::nullopt;
                    if (right.is_number() && right.get<Number>() == 0.0) return std::nullopt;

                    if (left.is_integer() && right.is_integer()) {
                        return ConstantValue{static_cast<Number>(left.get<Int>()) / static_cast<Number>(right.get<Int>())};
                    } else if (left.is_number() && right.is_number()) {
                        return ConstantValue{left.get<Number>() / right.get<Number>()};
                    } else if (left.is_integer() && right.is_number()) {
                        return ConstantValue{static_cast<Number>(left.get<Int>()) / right.get<Number>()};
                    } else if (left.is_number() && right.is_integer()) {
                        return ConstantValue{left.get<Number>() / static_cast<Number>(right.get<Int>())};
                    }
                    break;

                default:
                    break;
            }
        } catch (...) {
            // Arithmetic overflow or other errors
            return std::nullopt;
        }

        return std::nullopt;
    }

    Optional<ConstantFoldingPass::ConstantValue>
    ConstantFoldingPass::evaluate_unary_op(OpCode op, const ConstantValue& operand) {
        try {
            switch (op) {
                case OpCode::OP_UNM:
                    if (operand.is_integer()) {
                        return ConstantValue{-operand.get<Int>()};
                    } else if (operand.is_number()) {
                        return ConstantValue{-operand.get<Number>()};
                    }
                    break;

                case OpCode::OP_NOT:
                    if (operand.is_boolean()) {
                        return ConstantValue{!operand.get<bool>()};
                    } else if (operand.is_nil()) {
                        return ConstantValue{true};
                    } else {
                        return ConstantValue{false};
                    }
                    break;

                case OpCode::OP_BNOT:
                    if (operand.is_integer()) {
                        return ConstantValue{~operand.get<Int>()};
                    }
                    break;

                default:
                    break;
            }
        } catch (...) {
            return std::nullopt;
        }

        return std::nullopt;
    }

    bool ConstantFoldingPass::is_constant_instruction(OpCode op) const noexcept {
        switch (op) {
            case OpCode::OP_LOADI:
            case OpCode::OP_LOADF:
            case OpCode::OP_LOADK:
            case OpCode::OP_LOADKX:
            case OpCode::OP_LOADTRUE:
            case OpCode::OP_LOADFALSE:
            case OpCode::OP_LOADNIL:
                return true;
            default:
                return false;
        }
    }

    Optional<ConstantFoldingPass::ConstantValue>
    ConstantFoldingPass::get_constant_value(const BytecodeFunction& function, Register reg) {
        // This would need to track the instruction that defines the register
        // For now, return nullopt as this requires more complex analysis
        return std::nullopt;
    }

    // DeadCodeEliminationPass Implementation
    Status DeadCodeEliminationPass::optimize(BytecodeFunction& function) {
        OPTIMIZER_LOG_DEBUG("Starting dead code elimination optimization");

        auto reachable = find_reachable_instructions(function);
        auto live_registers = find_live_registers(function);

        bool changed = false;
        auto& instructions = function.instructions;

        // Mark instructions for removal
        std::vector<bool> to_remove(instructions.size(), false);

        for (Size i = 0; i < instructions.size(); ++i) {
            if (reachable.find(i) == reachable.end()) {
                // Unreachable instruction
                to_remove[i] = true;
                changed = true;
                OPTIMIZER_LOG_DEBUG("Marking unreachable instruction {} for removal", i);
                continue;
            }

            Instruction instr = instructions[i];
            OpCode op = InstructionEncoder::decode_opcode(instr);
            Register a = InstructionEncoder::decode_a(instr);

            // Check if instruction writes to a dead register
            if (is_side_effect_free(op) && live_registers.find(a) == live_registers.end()) {
                to_remove[i] = true;
                changed = true;
                OPTIMIZER_LOG_DEBUG("Marking dead instruction {} for removal (writes to dead register {})", i, a);
            }
        }

        // Remove marked instructions (in reverse order to maintain indices)
        for (Size i = instructions.size(); i > 0; --i) {
            if (to_remove[i - 1]) {
                instructions.erase(instructions.begin() + (i - 1));
            }
        }

        OPTIMIZER_LOG_INFO("Dead code elimination completed, changed: {}", changed);
        return changed ? std::monostate{} : std::monostate{};
    }

    std::unordered_set<Size> DeadCodeEliminationPass::find_reachable_instructions(const BytecodeFunction& function) {
        std::unordered_set<Size> reachable;
        std::queue<Size> worklist;

        // Start from entry point
        if (!function.instructions.empty()) {
            worklist.push(0);
            reachable.insert(0);
        }

        while (!worklist.empty()) {
            Size current = worklist.front();
            worklist.pop();

            if (current >= function.instructions.size()) continue;

            Instruction instr = function.instructions[current];
            OpCode op = InstructionEncoder::decode_opcode(instr);

            // Add next instruction (if not a jump)
            if (!is_control_flow_instruction(op) || op == OpCode::OP_TEST || op == OpCode::OP_TESTSET) {
                Size next = current + 1;
                if (next < function.instructions.size() && reachable.find(next) == reachable.end()) {
                    reachable.insert(next);
                    worklist.push(next);
                }
            }

            // Add jump targets
            if (op == OpCode::OP_JMP) {
                std::int32_t offset = InstructionEncoder::decode_sbx(instr);
                Size target = current + 1 + offset;
                if (target < function.instructions.size() && reachable.find(target) == reachable.end()) {
                    reachable.insert(target);
                    worklist.push(target);
                }
            } else if (op == OpCode::OP_FORLOOP || op == OpCode::OP_FORPREP || op == OpCode::OP_TFORLOOP) {
                std::uint32_t offset = InstructionEncoder::decode_bx(instr);
                Size target = current + 1 + offset;
                if (target < function.instructions.size() && reachable.find(target) == reachable.end()) {
                    reachable.insert(target);
                    worklist.push(target);
                }
            }
        }

        return reachable;
    }

    std::unordered_set<Register> DeadCodeEliminationPass::find_live_registers(const BytecodeFunction& function) {
        std::unordered_set<Register> live;

        // Simple backward analysis - mark all used registers as live
        for (const auto& instr : function.instructions) {
            OpCode op = InstructionEncoder::decode_opcode(instr);

            // Mark source registers as live
            switch (op) {
                case OpCode::OP_MOVE:
                case OpCode::OP_UNM:
                case OpCode::OP_NOT:
                case OpCode::OP_LEN:
                case OpCode::OP_BNOT: {
                    Register b = InstructionEncoder::decode_b(instr);
                    live.insert(b);
                    break;
                }

                case OpCode::OP_ADD:
                case OpCode::OP_SUB:
                case OpCode::OP_MUL:
                case OpCode::OP_DIV:
                case OpCode::OP_MOD:
                case OpCode::OP_POW:
                case OpCode::OP_BAND:
                case OpCode::OP_BOR:
                case OpCode::OP_BXOR:
                case OpCode::OP_SHL:
                case OpCode::OP_SHR:
                case OpCode::OP_GETTABLE:
                case OpCode::OP_SETTABLE: {
                    Register b = InstructionEncoder::decode_b(instr);
                    Register c = InstructionEncoder::decode_c(instr);
                    live.insert(b);
                    live.insert(c);
                    break;
                }

                case OpCode::OP_RETURN:
                case OpCode::OP_RETURN1: {
                    Register a = InstructionEncoder::decode_a(instr);
                    live.insert(a);
                    break;
                }

                default:
                    break;
            }
        }

        return live;
    }

    bool DeadCodeEliminationPass::is_side_effect_free(OpCode op) const noexcept {
        switch (op) {
            case OpCode::OP_MOVE:
            case OpCode::OP_LOADI:
            case OpCode::OP_LOADF:
            case OpCode::OP_LOADK:
            case OpCode::OP_LOADKX:
            case OpCode::OP_LOADTRUE:
            case OpCode::OP_LOADFALSE:
            case OpCode::OP_LOADNIL:
            case OpCode::OP_ADD:
            case OpCode::OP_SUB:
            case OpCode::OP_MUL:
            case OpCode::OP_DIV:
            case OpCode::OP_MOD:
            case OpCode::OP_POW:
            case OpCode::OP_BAND:
            case OpCode::OP_BOR:
            case OpCode::OP_BXOR:
            case OpCode::OP_SHL:
            case OpCode::OP_SHR:
            case OpCode::OP_UNM:
            case OpCode::OP_NOT:
            case OpCode::OP_BNOT:
            case OpCode::OP_LEN:
                return true;
            default:
                return false;
        }
    }

    bool DeadCodeEliminationPass::is_control_flow_instruction(OpCode op) const noexcept {
        switch (op) {
            case OpCode::OP_JMP:
            case OpCode::OP_FORLOOP:
            case OpCode::OP_FORPREP:
            case OpCode::OP_TFORLOOP:
            case OpCode::OP_EQ:
            case OpCode::OP_LT:
            case OpCode::OP_LE:
            case OpCode::OP_TEST:
            case OpCode::OP_TESTSET:
            case OpCode::OP_RETURN:
            case OpCode::OP_RETURN0:
            case OpCode::OP_RETURN1:
            case OpCode::OP_TAILCALL:
                return true;
            default:
                return false;
        }
    }

    // PeepholeOptimizationPass Implementation
    Status PeepholeOptimizationPass::optimize(BytecodeFunction& function) {
        OPTIMIZER_LOG_DEBUG("Starting peephole optimization");

        if (patterns_.empty()) {
            initialize_patterns();
        }

        bool changed = false;
        auto& instructions = function.instructions;

        // Apply patterns repeatedly until no more changes
        bool pattern_applied;
        do {
            pattern_applied = false;

            for (Size i = 0; i < instructions.size(); ++i) {
                for (const auto& pattern : patterns_) {
                    if (apply_pattern(instructions, i, pattern)) {
                        pattern_applied = true;
                        changed = true;
                        break; // Start over after any change
                    }
                }
                if (pattern_applied) break;
            }
        } while (pattern_applied);

        OPTIMIZER_LOG_INFO("Peephole optimization completed, changed: {}", changed);
        return std::monostate{};
    }

    void PeepholeOptimizationPass::initialize_patterns() {
        patterns_.clear();

        // Pattern: MOVE R[A], R[B]; MOVE R[C], R[A] -> MOVE R[C], R[B] (if A is not used elsewhere)
        patterns_.push_back({
            {OpCode::OP_MOVE, OpCode::OP_MOVE},
            [](const std::vector<Instruction>& instrs) -> bool {
                if (instrs.size() < 2) return false;

                Register a1 = InstructionEncoder::decode_a(instrs[0]);
                [[maybe_unused]] Register b1 = InstructionEncoder::decode_b(instrs[0]);
                [[maybe_unused]] Register a2 = InstructionEncoder::decode_a(instrs[1]);
                Register b2 = InstructionEncoder::decode_b(instrs[1]);

                return a1 == b2; // Second move uses result of first move
            },
            [](const std::vector<Instruction>& instrs) -> std::vector<Instruction> {
                Register b1 = InstructionEncoder::decode_b(instrs[0]);
                Register a2 = InstructionEncoder::decode_a(instrs[1]);

                // Replace with single move: MOVE R[a2], R[b1]
                return {InstructionEncoder::encode_abc(OpCode::OP_MOVE, a2, b1, 0)};
            }
        });

        // Pattern: LOADI R[A], 0; ADD R[B], R[C], R[A] -> MOVE R[B], R[C]
        patterns_.push_back({
            {OpCode::OP_LOADI, OpCode::OP_ADD},
            [](const std::vector<Instruction>& instrs) -> bool {
                if (instrs.size() < 2) return false;

                std::int32_t value = InstructionEncoder::decode_sbx(instrs[0]);
                Register a1 = InstructionEncoder::decode_a(instrs[0]);
                Register c2 = InstructionEncoder::decode_c(instrs[1]);

                return value == 0 && a1 == c2;
            },
            [](const std::vector<Instruction>& instrs) -> std::vector<Instruction> {
                Register a2 = InstructionEncoder::decode_a(instrs[1]);
                Register b2 = InstructionEncoder::decode_b(instrs[1]);

                return {InstructionEncoder::encode_abc(OpCode::OP_MOVE, a2, b2, 0)};
            }
        });

        // Pattern: LOADI R[A], 1; MUL R[B], R[C], R[A] -> MOVE R[B], R[C]
        patterns_.push_back({
            {OpCode::OP_LOADI, OpCode::OP_MUL},
            [](const std::vector<Instruction>& instrs) -> bool {
                if (instrs.size() < 2) return false;

                std::int32_t value = InstructionEncoder::decode_sbx(instrs[0]);
                Register a1 = InstructionEncoder::decode_a(instrs[0]);
                Register c2 = InstructionEncoder::decode_c(instrs[1]);

                return value == 1 && a1 == c2;
            },
            [](const std::vector<Instruction>& instrs) -> std::vector<Instruction> {
                Register a2 = InstructionEncoder::decode_a(instrs[1]);
                Register b2 = InstructionEncoder::decode_b(instrs[1]);

                return {InstructionEncoder::encode_abc(OpCode::OP_MOVE, a2, b2, 0)};
            }
        });

        // Pattern: NOT R[A], R[B]; NOT R[C], R[A] -> MOVE R[C], R[B]
        patterns_.push_back({
            {OpCode::OP_NOT, OpCode::OP_NOT},
            [](const std::vector<Instruction>& instrs) -> bool {
                if (instrs.size() < 2) return false;

                Register a1 = InstructionEncoder::decode_a(instrs[0]);
                Register b2 = InstructionEncoder::decode_b(instrs[1]);

                return a1 == b2;
            },
            [](const std::vector<Instruction>& instrs) -> std::vector<Instruction> {
                Register b1 = InstructionEncoder::decode_b(instrs[0]);
                Register a2 = InstructionEncoder::decode_a(instrs[1]);

                return {InstructionEncoder::encode_abc(OpCode::OP_MOVE, a2, b1, 0)};
            }
        });
    }

    bool PeepholeOptimizationPass::apply_pattern(std::vector<Instruction>& instructions,
                                                 Size start, const Pattern& pattern) {
        if (start + pattern.opcodes.size() > instructions.size()) {
            return false;
        }

        // Check if pattern matches
        std::vector<Instruction> window;
        for (Size i = 0; i < pattern.opcodes.size(); ++i) {
            Instruction instr = instructions[start + i];
            OpCode op = InstructionEncoder::decode_opcode(instr);

            if (op != pattern.opcodes[i]) {
                return false;
            }

            window.push_back(instr);
        }

        // Check pattern condition
        if (!pattern.condition(window)) {
            return false;
        }

        // Apply replacement
        auto replacement = pattern.replacement(window);

        // Replace instructions
        auto it = instructions.begin() + start;
        instructions.erase(it, it + pattern.opcodes.size());
        instructions.insert(instructions.begin() + start, replacement.begin(), replacement.end());

        OPTIMIZER_LOG_DEBUG("Applied peephole pattern at instruction {}", start);
        return true;
    }

    // JumpOptimizationPass Implementation
    Status JumpOptimizationPass::optimize(BytecodeFunction& function) {
        OPTIMIZER_LOG_DEBUG("Starting jump optimization");

        bool changed = false;

        // Apply different jump optimizations
        changed |= optimize_jump_chains(function);
        changed |= eliminate_redundant_jumps(function);
        changed |= optimize_conditional_jumps(function);

        OPTIMIZER_LOG_INFO("Jump optimization completed, changed: {}", changed);
        return std::monostate{};
    }

    bool JumpOptimizationPass::optimize_jump_chains(BytecodeFunction& function) {
        bool changed = false;
        auto& instructions = function.instructions;

        for (Size i = 0; i < instructions.size(); ++i) {
            Instruction instr = instructions[i];
            OpCode op = InstructionEncoder::decode_opcode(instr);

            if (op == OpCode::OP_JMP) {
                std::int32_t offset = InstructionEncoder::decode_sbx(instr);
                Size target = i + 1 + offset;

                // Follow jump chain
                Size final_target = follow_jump_chain(function, target);

                if (final_target != target && final_target < instructions.size()) {
                    // Update jump to point to final target
                    std::int32_t new_offset = static_cast<std::int32_t>(final_target) - static_cast<std::int32_t>(i + 1);
                    instructions[i] = InstructionEncoder::encode_asbx(OpCode::OP_JMP, 0, new_offset);
                    changed = true;
                    OPTIMIZER_LOG_DEBUG("Optimized jump chain at instruction {}", i);
                }
            }
        }

        return changed;
    }

    bool JumpOptimizationPass::eliminate_redundant_jumps(BytecodeFunction& function) {
        bool changed = false;
        auto& instructions = function.instructions;

        for (Size i = 0; i < instructions.size(); ++i) {
            Instruction instr = instructions[i];
            OpCode op = InstructionEncoder::decode_opcode(instr);

            if (op == OpCode::OP_JMP) {
                std::int32_t offset = InstructionEncoder::decode_sbx(instr);

                // Check for jump to next instruction (redundant jump)
                if (offset == 0) {
                    // Remove this instruction
                    instructions.erase(instructions.begin() + static_cast<std::ptrdiff_t>(i));
                    changed = true;
                    OPTIMIZER_LOG_DEBUG("Eliminated redundant jump at instruction {}", i);
                    --i; // Adjust index after removal
                }
            }
        }

        return changed;
    }

    bool JumpOptimizationPass::optimize_conditional_jumps([[maybe_unused]] BytecodeFunction& function) {
        // TODO: Implement conditional jump optimizations
        // This could include optimizing TEST/JMP patterns, etc.
        return false;
    }

    Size JumpOptimizationPass::follow_jump_chain(const BytecodeFunction& function, Size start) {
        std::unordered_set<Size> visited;
        Size current = start;

        while (current < function.instructions.size() && visited.find(current) == visited.end()) {
            visited.insert(current);

            Instruction instr = function.instructions[current];
            OpCode op = InstructionEncoder::decode_opcode(instr);

            if (op == OpCode::OP_JMP) {
                std::int32_t offset = InstructionEncoder::decode_sbx(instr);
                current = current + 1 + offset;
            } else {
                break;
            }
        }

        return current;
    }

    // TailCallOptimizationPass Implementation
    Status TailCallOptimizationPass::optimize(BytecodeFunction& function) {
        OPTIMIZER_LOG_DEBUG("Starting tail call optimization");

        bool changed = false;
        auto& instructions = function.instructions;

        for (Size i = 0; i < instructions.size(); ++i) {
            Instruction instr = instructions[i];
            OpCode op = InstructionEncoder::decode_opcode(instr);

            if (op == OpCode::OP_CALL && is_tail_position(function, i) && can_optimize_call(function, i)) {
                convert_to_tail_call(function, i);
                changed = true;
                OPTIMIZER_LOG_DEBUG("Converted call to tail call at instruction {}", i);
            }
        }

        OPTIMIZER_LOG_INFO("Tail call optimization completed, changed: {}", changed);
        return std::monostate{};
    }

    bool TailCallOptimizationPass::is_tail_position(const BytecodeFunction& function, Size instruction_index) {
        // Check if the call is followed immediately by a return
        if (instruction_index + 1 >= function.instructions.size()) {
            return false;
        }

        Instruction next_instr = function.instructions[instruction_index + 1];
        OpCode next_op = InstructionEncoder::decode_opcode(next_instr);

        return next_op == OpCode::OP_RETURN || next_op == OpCode::OP_RETURN0 || next_op == OpCode::OP_RETURN1;
    }

    bool TailCallOptimizationPass::can_optimize_call([[maybe_unused]] const BytecodeFunction& function,
                                                     [[maybe_unused]] Size call_index) {
        // TODO: Add more sophisticated analysis
        // For now, assume all tail position calls can be optimized
        return true;
    }

    void TailCallOptimizationPass::convert_to_tail_call(BytecodeFunction& function, Size call_index) {
        auto& instructions = function.instructions;
        Instruction call_instr = instructions[call_index];

        Register a = InstructionEncoder::decode_a(call_instr);
        Register b = InstructionEncoder::decode_b(call_instr);

        // Replace CALL with TAILCALL
        instructions[call_index] = InstructionEncoder::encode_abc(OpCode::OP_TAILCALL, a, b, 0);

        // Remove the following RETURN instruction if present
        if (call_index + 1 < instructions.size()) {
            Instruction next_instr = instructions[call_index + 1];
            OpCode next_op = InstructionEncoder::decode_opcode(next_instr);

            if (next_op == OpCode::OP_RETURN || next_op == OpCode::OP_RETURN0 || next_op == OpCode::OP_RETURN1) {
                instructions.erase(instructions.begin() + static_cast<std::ptrdiff_t>(call_index + 1));
            }
        }
    }

    // RegisterOptimizationPass Implementation
    Status RegisterOptimizationPass::optimize(BytecodeFunction& function) {
        OPTIMIZER_LOG_DEBUG("Starting register optimization");

        auto usage = analyze_register_usage(function);
        auto mapping = compute_register_mapping(usage);

        if (!mapping.empty()) {
            remap_registers(function, mapping);
            OPTIMIZER_LOG_INFO("Register optimization completed, remapped {} registers", mapping.size());
        } else {
            OPTIMIZER_LOG_INFO("Register optimization completed, no changes needed");
        }

        return std::monostate{};
    }

    std::unordered_map<Register, RegisterOptimizationPass::RegisterInfo>
    RegisterOptimizationPass::analyze_register_usage(const BytecodeFunction& function) {
        std::unordered_map<Register, RegisterInfo> usage;

        for (Size i = 0; i < function.instructions.size(); ++i) {
            Instruction instr = function.instructions[i];
            OpCode op = InstructionEncoder::decode_opcode(instr);

            // Track register usage based on instruction type
            Register a = InstructionEncoder::decode_a(instr);
            if (a < 255) {  // Valid register
                auto& info = usage[a];
                if (info.first_use == SIZE_MAX) {
                    info.first_use = i;
                }
                info.last_use = i;
                info.uses.push_back(i);
            }

            // Track source registers
            switch (op) {
                case OpCode::OP_MOVE:
                case OpCode::OP_UNM:
                case OpCode::OP_NOT:
                case OpCode::OP_LEN:
                case OpCode::OP_BNOT: {
                    Register b = InstructionEncoder::decode_b(instr);
                    if (b < 255) {
                        auto& info = usage[b];
                        if (info.first_use == SIZE_MAX) {
                            info.first_use = i;
                        }
                        info.last_use = i;
                        info.uses.push_back(i);
                    }
                    break;
                }

                case OpCode::OP_ADD:
                case OpCode::OP_SUB:
                case OpCode::OP_MUL:
                case OpCode::OP_DIV:
                case OpCode::OP_MOD:
                case OpCode::OP_POW:
                case OpCode::OP_BAND:
                case OpCode::OP_BOR:
                case OpCode::OP_BXOR:
                case OpCode::OP_SHL:
                case OpCode::OP_SHR: {
                    Register b = InstructionEncoder::decode_b(instr);
                    Register c = InstructionEncoder::decode_c(instr);

                    if (b < 255) {
                        auto& info = usage[b];
                        if (info.first_use == SIZE_MAX) {
                            info.first_use = i;
                        }
                        info.last_use = i;
                        info.uses.push_back(i);
                    }

                    if (c < 255) {
                        auto& info = usage[c];
                        if (info.first_use == SIZE_MAX) {
                            info.first_use = i;
                        }
                        info.last_use = i;
                        info.uses.push_back(i);
                    }
                    break;
                }

                default:
                    break;
            }
        }

        return usage;
    }

    std::unordered_map<Register, Register>
    RegisterOptimizationPass::compute_register_mapping(const std::unordered_map<Register, RegisterInfo>& usage) {
        std::unordered_map<Register, Register> mapping;

        // Simple register compaction - assign consecutive register numbers
        std::vector<std::pair<Register, RegisterInfo>> sorted_usage;
        for (const auto& [reg, info] : usage) {
            sorted_usage.emplace_back(reg, info);
        }

        // Sort by first use to maintain program order
        std::sort(sorted_usage.begin(), sorted_usage.end(),
                  [](const auto& a, const auto& b) {
                      return a.second.first_use < b.second.first_use;
                  });

        Register next_reg = 0;
        for (const auto& [original_reg, info] : sorted_usage) {
            if (original_reg != next_reg) {
                mapping[original_reg] = next_reg;
            }
            ++next_reg;
        }

        return mapping;
    }

    void RegisterOptimizationPass::remap_registers(BytecodeFunction& function,
                                                   const std::unordered_map<Register, Register>& mapping) {
        auto& instructions = function.instructions;

        for (auto& instr : instructions) {
            OpCode op = InstructionEncoder::decode_opcode(instr);

            // Remap register A
            Register a = InstructionEncoder::decode_a(instr);
            auto a_it = mapping.find(a);
            if (a_it != mapping.end()) {
                // Reconstruct instruction with new register
                switch (op) {
                    case OpCode::OP_LOADI:
                    case OpCode::OP_LOADF: {
                        std::int32_t sbx = InstructionEncoder::decode_sbx(instr);
                        instr = InstructionEncoder::encode_asbx(op, a_it->second, sbx);
                        break;
                    }

                    case OpCode::OP_LOADK: {
                        std::uint32_t bx = InstructionEncoder::decode_bx(instr);
                        instr = InstructionEncoder::encode_abx(op, a_it->second, bx);
                        break;
                    }

                    default: {
                        Register b = InstructionEncoder::decode_b(instr);
                        Register c = InstructionEncoder::decode_c(instr);

                        // Remap register B if needed
                        auto b_it = mapping.find(b);
                        if (b_it != mapping.end()) {
                            b = b_it->second;
                        }

                        // Remap register C if needed
                        auto c_it = mapping.find(c);
                        if (c_it != mapping.end()) {
                            c = c_it->second;
                        }

                        instr = InstructionEncoder::encode_abc(op, a_it->second, b, c);
                        break;
                    }
                }
            }
        }
    }

    // Main Optimizer class implementation
    Optimizer::Optimizer(OptimizationLevel level) : level_(level) {
        initialize_default_passes();
        configure_passes_for_level(level);
    }

    Status Optimizer::optimize(BytecodeFunction& function) {
        OPTIMIZER_LOG_INFO("Starting optimization with level {}", static_cast<int>(level_));

        reset_statistics();

        bool any_changes = false;
        Size iteration = 0;
        const Size max_iterations = 10; // Prevent infinite loops

        do {
            bool iteration_changes = false;
            ++iteration;

            OPTIMIZER_LOG_DEBUG("Optimization iteration {}", iteration);

            for (const auto& pass : passes_) {
                if (!is_pass_enabled(pass->name())) {
                    continue;
                }

                OPTIMIZER_LOG_DEBUG("Running pass: {}", pass->name());

                auto start_time = std::chrono::high_resolution_clock::now();
                Status result = pass->optimize(function);
                auto end_time = std::chrono::high_resolution_clock::now();

                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

                // Update statistics
                String pass_name{pass->name()};
                statistics_[pass_name + "_runs"]++;
                statistics_[pass_name + "_time_us"] += duration.count();

                if (is_success(result)) {
                    if (pass->is_transformative()) {
                        iteration_changes = true;
                        any_changes = true;
                        statistics_[pass_name + "_changes"]++;
                    }
                } else {
                    OPTIMIZER_LOG_ERROR("Pass {} failed", pass->name());
                    return get_error(result);
                }
            }

            if (!iteration_changes || iteration >= max_iterations) {
                break;
            }

        } while (true);

        OPTIMIZER_LOG_INFO("Optimization completed after {} iterations, changes: {}", iteration, any_changes);

        return std::monostate{};
    }

    void Optimizer::add_pass(UniquePtr<OptimizationPass> pass) {
        String name{pass->name()};
        passes_.push_back(std::move(pass));
        pass_enabled_[name] = true;
    }

    void Optimizer::remove_pass(StringView name) {
        String name_str{name};
        passes_.erase(
            std::remove_if(passes_.begin(), passes_.end(),
                          [&name_str](const auto& pass) {
                              return String{pass->name()} == name_str;
                          }),
            passes_.end());
        pass_enabled_.erase(name_str);
    }

    void Optimizer::set_optimization_level(OptimizationLevel level) {
        level_ = level;
        configure_passes_for_level(level);
    }

    Optimizer::OptimizationLevel Optimizer::optimization_level() const noexcept {
        return level_;
    }

    void Optimizer::set_pass_enabled(StringView name, bool enabled) {
        pass_enabled_[String{name}] = enabled;
    }

    bool Optimizer::is_pass_enabled(StringView name) const noexcept {
        String name_str{name};
        auto it = pass_enabled_.find(name_str);
        return it != pass_enabled_.end() ? it->second : false;
    }

    const std::unordered_map<String, Size>& Optimizer::statistics() const noexcept {
        return statistics_;
    }

    void Optimizer::reset_statistics() {
        statistics_.clear();
    }

    void Optimizer::initialize_default_passes() {
        passes_.clear();
        pass_enabled_.clear();

        // Add all optimization passes
        add_pass(std::make_unique<ConstantFoldingPass>());
        add_pass(std::make_unique<DeadCodeEliminationPass>());
        add_pass(std::make_unique<PeepholeOptimizationPass>());
        add_pass(std::make_unique<RegisterOptimizationPass>());
        add_pass(std::make_unique<JumpOptimizationPass>());
        add_pass(std::make_unique<TailCallOptimizationPass>());
    }

    void Optimizer::configure_passes_for_level(OptimizationLevel level) {
        // Configure which passes are enabled based on optimization level
        switch (level) {
            case OptimizationLevel::None:
                for (auto& [name, enabled] : pass_enabled_) {
                    enabled = false;
                }
                break;

            case OptimizationLevel::Basic:
                set_pass_enabled("constant-folding", true);
                set_pass_enabled("dead-code-elimination", false);
                set_pass_enabled("peephole-optimization", true);
                set_pass_enabled("register-optimization", false);
                set_pass_enabled("jump-optimization", true);
                set_pass_enabled("tail-call-optimization", false);
                break;

            case OptimizationLevel::Standard:
                set_pass_enabled("constant-folding", true);
                set_pass_enabled("dead-code-elimination", true);
                set_pass_enabled("peephole-optimization", true);
                set_pass_enabled("register-optimization", true);
                set_pass_enabled("jump-optimization", true);
                set_pass_enabled("tail-call-optimization", true);
                break;

            case OptimizationLevel::Aggressive:
                // Enable all passes for aggressive optimization
                for (auto& [name, enabled] : pass_enabled_) {
                    enabled = true;
                }
                break;
        }
    }

    // Control Flow Graph implementation
    namespace optimization_analysis {

        ControlFlowGraph::ControlFlowGraph(const BytecodeFunction& function) {
            build_cfg(function);
            compute_def_use_sets(function);
        }

        void ControlFlowGraph::compute_liveness() {
            // Backward data flow analysis for liveness
            bool changed = true;

            while (changed) {
                changed = false;

                // Process nodes in reverse order
                for (auto it = nodes_.rbegin(); it != nodes_.rend(); ++it) {
                    auto& node = *it;

                    // live_out[n] = union of live_in[s] for all successors s of n
                    std::unordered_set<Register> new_live_out;
                    for (Size successor : node.successors) {
                        if (successor < nodes_.size()) {
                            const auto& succ_live_in = nodes_[successor].live_in;
                            new_live_out.insert(succ_live_in.begin(), succ_live_in.end());
                        }
                    }

                    // live_in[n] = use[n] union (live_out[n] - def[n])
                    std::unordered_set<Register> new_live_in = node.use;
                    for (Register reg : new_live_out) {
                        if (node.def.find(reg) == node.def.end()) {
                            new_live_in.insert(reg);
                        }
                    }

                    // Check for changes
                    if (new_live_in != node.live_in || new_live_out != node.live_out) {
                        node.live_in = std::move(new_live_in);
                        node.live_out = std::move(new_live_out);
                        changed = true;
                    }
                }
            }
        }

        void ControlFlowGraph::compute_dominators() {
            if (nodes_.empty()) return;

            // Initialize dominators
            dominators_.resize(nodes_.size());
            dominator_tree_.resize(nodes_.size());

            // Entry node dominates itself
            dominators_[0] = 0;

            // All other nodes initially dominated by all nodes
            for (Size i = 1; i < nodes_.size(); ++i) {
                dominators_[i] = SIZE_MAX; // Undefined initially
            }

            // Iterative algorithm
            bool changed = true;
            while (changed) {
                changed = false;

                for (Size i = 1; i < nodes_.size(); ++i) {
                    Size new_dom = SIZE_MAX;

                    // Find first processed predecessor
                    for (Size pred : nodes_[i].predecessors) {
                        if (dominators_[pred] != SIZE_MAX) {
                            new_dom = pred;
                            break;
                        }
                    }

                    // Intersect with other processed predecessors
                    for (Size pred : nodes_[i].predecessors) {
                        if (dominators_[pred] != SIZE_MAX && pred != new_dom) {
                            new_dom = intersect_dominators(pred, new_dom);
                        }
                    }

                    if (dominators_[i] != new_dom) {
                        dominators_[i] = new_dom;
                        changed = true;
                    }
                }
            }

            // Build dominator tree
            for (Size i = 0; i < nodes_.size(); ++i) {
                if (i != 0 && dominators_[i] != SIZE_MAX) {
                    dominator_tree_[dominators_[i]].push_back(i);
                }
            }
        }

        void ControlFlowGraph::build_cfg(const BytecodeFunction& function) {
            if (function.instructions.empty()) return;

            // Find basic block boundaries
            std::unordered_set<Size> leaders;
            leaders.insert(0); // First instruction is always a leader

            for (Size i = 0; i < function.instructions.size(); ++i) {
                Instruction instr = function.instructions[i];
                OpCode op = InstructionEncoder::decode_opcode(instr);

                // Jump targets are leaders
                if (op == OpCode::OP_JMP) {
                    std::int32_t offset = InstructionEncoder::decode_sbx(instr);
                    Size target = i + 1 + offset;
                    if (target < function.instructions.size()) {
                        leaders.insert(target);
                    }
                    // Instruction after jump is also a leader
                    if (i + 1 < function.instructions.size()) {
                        leaders.insert(i + 1);
                    }
                } else if (op == OpCode::OP_FORLOOP || op == OpCode::OP_FORPREP || op == OpCode::OP_TFORLOOP) {
                    std::uint32_t offset = InstructionEncoder::decode_bx(instr);
                    Size target = i + 1 + offset;
                    if (target < function.instructions.size()) {
                        leaders.insert(target);
                    }
                    if (i + 1 < function.instructions.size()) {
                        leaders.insert(i + 1);
                    }
                }
            }

            // Create basic blocks
            std::vector<Size> sorted_leaders(leaders.begin(), leaders.end());
            std::sort(sorted_leaders.begin(), sorted_leaders.end());

            nodes_.clear();
            for (Size i = 0; i < sorted_leaders.size(); ++i) {
                CFGNode node;
                node.start_instruction = sorted_leaders[i];

                if (i + 1 < sorted_leaders.size()) {
                    node.end_instruction = sorted_leaders[i + 1] - 1;
                } else {
                    node.end_instruction = function.instructions.size() - 1;
                }

                nodes_.push_back(node);
            }

            // Build edges between basic blocks
            for (Size i = 0; i < nodes_.size(); ++i) {
                auto& node = nodes_[i];
                Size last_instr = node.end_instruction;

                if (last_instr < function.instructions.size()) {
                    Instruction instr = function.instructions[last_instr];
                    OpCode op = InstructionEncoder::decode_opcode(instr);

                    // Add fall-through edge
                    if (op != OpCode::OP_JMP && op != OpCode::OP_RETURN &&
                        op != OpCode::OP_RETURN0 && op != OpCode::OP_RETURN1 &&
                        op != OpCode::OP_TAILCALL) {
                        if (i + 1 < nodes_.size()) {
                            node.successors.push_back(i + 1);
                            nodes_[i + 1].predecessors.push_back(i);
                        }
                    }

                    // Add jump edges
                    if (op == OpCode::OP_JMP) {
                        std::int32_t offset = InstructionEncoder::decode_sbx(instr);
                        Size target = last_instr + 1 + offset;

                        // Find target basic block
                        for (Size j = 0; j < nodes_.size(); ++j) {
                            if (nodes_[j].start_instruction == target) {
                                node.successors.push_back(j);
                                nodes_[j].predecessors.push_back(i);
                                break;
                            }
                        }
                    }
                }
            }
        }

        void ControlFlowGraph::compute_def_use_sets(const BytecodeFunction& function) {
            for (auto& node : nodes_) {
                for (Size i = node.start_instruction; i <= node.end_instruction && i < function.instructions.size(); ++i) {
                    Instruction instr = function.instructions[i];
                    OpCode op = InstructionEncoder::decode_opcode(instr);

                    // Add definitions
                    Register a = InstructionEncoder::decode_a(instr);
                    if (a < 255) {
                        node.def.insert(a);
                    }

                    // Add uses
                    switch (op) {
                        case OpCode::OP_MOVE:
                        case OpCode::OP_UNM:
                        case OpCode::OP_NOT:
                        case OpCode::OP_LEN:
                        case OpCode::OP_BNOT: {
                            Register b = InstructionEncoder::decode_b(instr);
                            if (b < 255) {
                                node.use.insert(b);
                            }
                            break;
                        }

                        case OpCode::OP_ADD:
                        case OpCode::OP_SUB:
                        case OpCode::OP_MUL:
                        case OpCode::OP_DIV:
                        case OpCode::OP_MOD:
                        case OpCode::OP_POW:
                        case OpCode::OP_BAND:
                        case OpCode::OP_BOR:
                        case OpCode::OP_BXOR:
                        case OpCode::OP_SHL:
                        case OpCode::OP_SHR: {
                            Register b = InstructionEncoder::decode_b(instr);
                            Register c = InstructionEncoder::decode_c(instr);
                            if (b < 255) node.use.insert(b);
                            if (c < 255) node.use.insert(c);
                            break;
                        }

                        default:
                            break;
                    }
                }
            }
        }

        Size ControlFlowGraph::intersect_dominators(Size b1, Size b2) {
            Size finger1 = b1;
            Size finger2 = b2;

            while (finger1 != finger2) {
                while (finger1 > finger2) {
                    finger1 = dominators_[finger1];
                }
                while (finger2 > finger1) {
                    finger2 = dominators_[finger2];
                }
            }

            return finger1;
        }

        // DataFlowAnalysis implementation
        std::unordered_map<Register, std::vector<Size>>
        DataFlowAnalysis::compute_reaching_definitions(const BytecodeFunction& function) {
            std::unordered_map<Register, std::vector<Size>> reaching_defs;

            // Simple implementation - track all definitions for each register
            for (Size i = 0; i < function.instructions.size(); ++i) {
                Instruction instr = function.instructions[i];
                Register a = InstructionEncoder::decode_a(instr);

                if (a < 255) {
                    reaching_defs[a].push_back(i);
                }
            }

            return reaching_defs;
        }

        std::unordered_set<Register>
        DataFlowAnalysis::compute_live_variables(const BytecodeFunction& function, Size instruction) {
            std::unordered_set<Register> live;

            // Simple backward scan from the given instruction
            for (Size i = instruction; i < function.instructions.size(); ++i) {
                Instruction instr = function.instructions[i];
                OpCode op = InstructionEncoder::decode_opcode(instr);

                // Add used registers
                switch (op) {
                    case OpCode::OP_MOVE:
                    case OpCode::OP_UNM:
                    case OpCode::OP_NOT:
                    case OpCode::OP_LEN:
                    case OpCode::OP_BNOT: {
                        Register b = InstructionEncoder::decode_b(instr);
                        if (b < 255) live.insert(b);
                        break;
                    }

                    case OpCode::OP_ADD:
                    case OpCode::OP_SUB:
                    case OpCode::OP_MUL:
                    case OpCode::OP_DIV:
                    case OpCode::OP_MOD:
                    case OpCode::OP_POW:
                    case OpCode::OP_BAND:
                    case OpCode::OP_BOR:
                    case OpCode::OP_BXOR:
                    case OpCode::OP_SHL:
                    case OpCode::OP_SHR: {
                        Register b = InstructionEncoder::decode_b(instr);
                        Register c = InstructionEncoder::decode_c(instr);
                        if (b < 255) live.insert(b);
                        if (c < 255) live.insert(c);
                        break;
                    }

                    default:
                        break;
                }
            }

            return live;
        }

        std::unordered_map<Register, std::vector<Size>>
        DataFlowAnalysis::compute_use_def_chains(const BytecodeFunction& function) {
            std::unordered_map<Register, std::vector<Size>> use_def_chains;

            // Build use-def chains by tracking definitions and uses
            std::unordered_map<Register, Size> last_def;

            for (Size i = 0; i < function.instructions.size(); ++i) {
                Instruction instr = function.instructions[i];
                OpCode op = InstructionEncoder::decode_opcode(instr);

                // Process uses first
                switch (op) {
                    case OpCode::OP_MOVE:
                    case OpCode::OP_UNM:
                    case OpCode::OP_NOT:
                    case OpCode::OP_LEN:
                    case OpCode::OP_BNOT: {
                        Register b = InstructionEncoder::decode_b(instr);
                        if (b < 255 && last_def.find(b) != last_def.end()) {
                            use_def_chains[b].push_back(last_def[b]);
                        }
                        break;
                    }

                    case OpCode::OP_ADD:
                    case OpCode::OP_SUB:
                    case OpCode::OP_MUL:
                    case OpCode::OP_DIV:
                    case OpCode::OP_MOD:
                    case OpCode::OP_POW:
                    case OpCode::OP_BAND:
                    case OpCode::OP_BOR:
                    case OpCode::OP_BXOR:
                    case OpCode::OP_SHL:
                    case OpCode::OP_SHR: {
                        Register b = InstructionEncoder::decode_b(instr);
                        Register c = InstructionEncoder::decode_c(instr);

                        if (b < 255 && last_def.find(b) != last_def.end()) {
                            use_def_chains[b].push_back(last_def[b]);
                        }
                        if (c < 255 && last_def.find(c) != last_def.end()) {
                            use_def_chains[c].push_back(last_def[c]);
                        }
                        break;
                    }

                    default:
                        break;
                }

                // Process definition
                Register a = InstructionEncoder::decode_a(instr);
                if (a < 255) {
                    last_def[a] = i;
                }
            }

            return use_def_chains;
        }

    }  // namespace optimization_analysis

}  // namespace rangelua::backend
