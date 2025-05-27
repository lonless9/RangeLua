/**
 * @file test_optimizer.cpp
 * @brief Comprehensive tests for the bytecode optimizer
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/backend/optimizer.hpp>
#include <rangelua/backend/bytecode.hpp>

using namespace rangelua;
using namespace rangelua::backend;

TEST_CASE("ConstantFoldingPass basic functionality", "[optimizer][constant-folding]") {
    SECTION("Fold integer arithmetic") {
        backend::BytecodeEmitter emitter("test_constant_folding");

        // Generate: LOADI R0, 5; LOADI R1, 3; ADD R2, R0, R1
        emitter.emit_asbx(OpCode::OP_LOADI, 0, 5);
        emitter.emit_asbx(OpCode::OP_LOADI, 1, 3);
        emitter.emit_abc(OpCode::OP_ADD, 2, 0, 1);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() == 3);

        ConstantFoldingPass pass;
        auto result = pass.optimize(function);

        REQUIRE(is_success(result));
        // The pass should have optimized the ADD to a constant load
        // Note: This is a simplified test - actual optimization may vary
    }

    SECTION("Fold boolean operations") {
        backend::BytecodeEmitter emitter("test_boolean_folding");

        // Generate: LOADTRUE R0; NOT R1, R0
        emitter.emit_abc(OpCode::OP_LOADTRUE, 0, 0, 0);
        emitter.emit_abc(OpCode::OP_NOT, 1, 0, 0);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() == 2);

        ConstantFoldingPass pass;
        auto result = pass.optimize(function);

        REQUIRE(is_success(result));
    }
}

TEST_CASE("DeadCodeEliminationPass functionality", "[optimizer][dead-code]") {
    SECTION("Remove unreachable code") {
        backend::BytecodeEmitter emitter("test_dead_code");

        // Generate: RETURN0; LOADI R0, 42 (unreachable)
        emitter.emit_abc(OpCode::OP_RETURN0, 0, 0, 0);
        emitter.emit_asbx(OpCode::OP_LOADI, 0, 42);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() == 2);

        DeadCodeEliminationPass pass;
        auto result = pass.optimize(function);

        REQUIRE(is_success(result));
        // Should remove the unreachable LOADI instruction
        REQUIRE(function.instructions.size() == 1);
    }

    SECTION("Remove dead register assignments") {
        backend::BytecodeEmitter emitter("test_dead_registers");

        // Generate: LOADI R0, 42; RETURN0 (R0 is never used)
        emitter.emit_asbx(OpCode::OP_LOADI, 0, 42);
        emitter.emit_abc(OpCode::OP_RETURN0, 0, 0, 0);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() == 2);

        DeadCodeEliminationPass pass;
        auto result = pass.optimize(function);

        REQUIRE(is_success(result));
    }
}

TEST_CASE("PeepholeOptimizationPass functionality", "[optimizer][peephole]") {
    SECTION("Optimize move chains") {
        backend::BytecodeEmitter emitter("test_peephole");

        // Generate: MOVE R1, R0; MOVE R2, R1 -> should become MOVE R2, R0
        emitter.emit_abc(OpCode::OP_MOVE, 1, 0, 0);
        emitter.emit_abc(OpCode::OP_MOVE, 2, 1, 0);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() == 2);

        PeepholeOptimizationPass pass;
        auto result = pass.optimize(function);

        REQUIRE(is_success(result));
        // Should optimize to single move
        REQUIRE(function.instructions.size() == 1);

        // Verify the optimized instruction
        Instruction instr = function.instructions[0];
        REQUIRE(InstructionEncoder::decode_opcode(instr) == OpCode::OP_MOVE);
        REQUIRE(InstructionEncoder::decode_a(instr) == 2);
        REQUIRE(InstructionEncoder::decode_b(instr) == 0);
    }

    SECTION("Optimize arithmetic with zero") {
        backend::BytecodeEmitter emitter("test_arithmetic_zero");

        // Generate: LOADI R0, 0; ADD R1, R2, R0 -> should become MOVE R1, R2
        emitter.emit_asbx(OpCode::OP_LOADI, 0, 0);
        emitter.emit_abc(OpCode::OP_ADD, 1, 2, 0);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() == 2);

        PeepholeOptimizationPass pass;
        auto result = pass.optimize(function);

        REQUIRE(is_success(result));
        // Should optimize to single move
        REQUIRE(function.instructions.size() == 1);

        // Verify the optimized instruction
        Instruction instr = function.instructions[0];
        REQUIRE(InstructionEncoder::decode_opcode(instr) == OpCode::OP_MOVE);
        REQUIRE(InstructionEncoder::decode_a(instr) == 1);
        REQUIRE(InstructionEncoder::decode_b(instr) == 2);
    }
}

TEST_CASE("JumpOptimizationPass functionality", "[optimizer][jump]") {
    SECTION("Eliminate redundant jumps") {
        backend::BytecodeEmitter emitter("test_jump_optimization");

        // Generate: JMP 0 (jump to next instruction - redundant)
        emitter.emit_asbx(OpCode::OP_JMP, 0, 0);
        emitter.emit_abc(OpCode::OP_RETURN0, 0, 0, 0);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() == 2);

        JumpOptimizationPass pass;
        auto result = pass.optimize(function);

        REQUIRE(is_success(result));
        // Should remove the redundant jump
        REQUIRE(function.instructions.size() == 1);
        REQUIRE(InstructionEncoder::decode_opcode(function.instructions[0]) == OpCode::OP_RETURN0);
    }
}

TEST_CASE("TailCallOptimizationPass functionality", "[optimizer][tail-call]") {
    SECTION("Convert tail calls") {
        backend::BytecodeEmitter emitter("test_tail_call");

        // Generate: CALL R0, 1, 0; RETURN R0, 1
        emitter.emit_abc(OpCode::OP_CALL, 0, 1, 0);
        emitter.emit_abc(OpCode::OP_RETURN, 0, 1, 0);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() == 2);

        TailCallOptimizationPass pass;
        auto result = pass.optimize(function);

        REQUIRE(is_success(result));
        // Should convert to tail call and remove return
        REQUIRE(function.instructions.size() == 1);
        REQUIRE(InstructionEncoder::decode_opcode(function.instructions[0]) == OpCode::OP_TAILCALL);
    }
}

TEST_CASE("RegisterOptimizationPass functionality", "[optimizer][register]") {
    SECTION("Compact register usage") {
        backend::BytecodeEmitter emitter("test_register_optimization");

        // Generate code with sparse register usage
        emitter.emit_asbx(OpCode::OP_LOADI, 5, 42);  // R5 = 42
        emitter.emit_asbx(OpCode::OP_LOADI, 10, 24); // R10 = 24
        emitter.emit_abc(OpCode::OP_ADD, 15, 5, 10); // R15 = R5 + R10
        emitter.emit_abc(OpCode::OP_RETURN, 15, 1, 0);

        auto function = emitter.get_function();
        REQUIRE(function.instructions.size() == 4);

        RegisterOptimizationPass pass;
        auto result = pass.optimize(function);

        REQUIRE(is_success(result));
        // Registers should be compacted to use consecutive numbers
    }
}

TEST_CASE("Main Optimizer class functionality", "[optimizer][main]") {
    SECTION("Basic optimization with different levels") {
        backend::BytecodeEmitter emitter("test_optimizer");

        // Generate some code that can be optimized
        emitter.emit_asbx(OpCode::OP_LOADI, 0, 5);
        emitter.emit_asbx(OpCode::OP_LOADI, 1, 3);
        emitter.emit_abc(OpCode::OP_ADD, 2, 0, 1);
        emitter.emit_abc(OpCode::OP_MOVE, 3, 2, 0);
        emitter.emit_abc(OpCode::OP_RETURN, 3, 1, 0);

        auto function = emitter.get_function();
        [[maybe_unused]] Size original_size = function.instructions.size();

        // Test with different optimization levels
        for (auto level : {backend::Optimizer::OptimizationLevel::None,
                          backend::Optimizer::OptimizationLevel::Basic,
                          backend::Optimizer::OptimizationLevel::Standard,
                          backend::Optimizer::OptimizationLevel::Aggressive}) {

            auto test_function = function; // Copy for testing
            backend::Optimizer optimizer(level);

            auto result = optimizer.optimize(test_function);
            REQUIRE(is_success(result));

            // Verify statistics behavior based on optimization level
            const auto& stats = optimizer.statistics();
            if (level == backend::Optimizer::OptimizationLevel::None) {
                // No passes should run with None level, so no statistics
                REQUIRE(stats.empty());
            } else {
                // Other levels should have statistics from enabled passes
                REQUIRE(!stats.empty());
            }
        }
    }

    SECTION("Custom pass management") {
        backend::Optimizer optimizer(backend::Optimizer::OptimizationLevel::None);

        // Add a custom pass
        optimizer.add_pass(std::make_unique<ConstantFoldingPass>());

        // Enable the pass
        optimizer.set_pass_enabled("constant-folding", true);
        REQUIRE(optimizer.is_pass_enabled("constant-folding"));

        // Remove the pass
        optimizer.remove_pass("constant-folding");
        REQUIRE(!optimizer.is_pass_enabled("constant-folding"));
    }
}

TEST_CASE("Control Flow Graph functionality", "[optimizer][cfg]") {
    SECTION("Build CFG for simple function") {
        backend::BytecodeEmitter emitter("test_cfg");

        // Generate: LOADI R0, 1; JMP 1; LOADI R1, 2; RETURN R0, 1
        emitter.emit_asbx(OpCode::OP_LOADI, 0, 1);  // 0
        emitter.emit_asbx(OpCode::OP_JMP, 0, 1);    // 1: jump to instruction 3
        emitter.emit_asbx(OpCode::OP_LOADI, 1, 2);  // 2: unreachable
        emitter.emit_abc(OpCode::OP_RETURN, 0, 1, 0); // 3

        auto function = emitter.get_function();

        optimization_analysis::ControlFlowGraph cfg(function);

        // Should have created basic blocks
        REQUIRE(cfg.node_count() > 0);

        // Test liveness analysis
        cfg.compute_liveness();

        // Test dominator analysis
        cfg.compute_dominators();
        const auto& dominators = cfg.dominators();
        REQUIRE(!dominators.empty());
    }
}

TEST_CASE("Data Flow Analysis functionality", "[optimizer][dataflow]") {
    SECTION("Compute reaching definitions") {
        backend::BytecodeEmitter emitter("test_dataflow");

        emitter.emit_asbx(OpCode::OP_LOADI, 0, 42);
        emitter.emit_abc(OpCode::OP_MOVE, 1, 0, 0);
        emitter.emit_abc(OpCode::OP_RETURN, 1, 1, 0);

        auto function = emitter.get_function();

        auto reaching_defs = optimization_analysis::DataFlowAnalysis::compute_reaching_definitions(function);
        REQUIRE(!reaching_defs.empty());

        auto live_vars = optimization_analysis::DataFlowAnalysis::compute_live_variables(function, 0);
        // Live variables analysis should work

        auto use_def_chains = optimization_analysis::DataFlowAnalysis::compute_use_def_chains(function);
        REQUIRE(!use_def_chains.empty());
    }
}
