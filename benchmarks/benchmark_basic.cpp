/**
 * @file benchmark_basic.cpp
 * @brief Basic benchmarks for RangeLua
 * @version 0.1.0
 */

#include <benchmark/benchmark.h>
#include <rangelua/rangelua.hpp>

static void BM_ValueCreation(benchmark::State& state) {
    rangelua::initialize();
    
    for (auto _ : state) {
        rangelua::runtime::Value val(42.0);
        benchmark::DoNotOptimize(val);
    }
    
    rangelua::cleanup();
}
BENCHMARK(BM_ValueCreation);

static void BM_ValueComparison(benchmark::State& state) {
    rangelua::initialize();
    
    rangelua::runtime::Value val1(42.0);
    rangelua::runtime::Value val2(42.0);
    
    for (auto _ : state) {
        bool result = val1 == val2;
        benchmark::DoNotOptimize(result);
    }
    
    rangelua::cleanup();
}
BENCHMARK(BM_ValueComparison);

static void BM_StateCreation(benchmark::State& state) {
    rangelua::initialize();
    
    for (auto _ : state) {
        rangelua::api::State lua_state;
        benchmark::DoNotOptimize(lua_state);
    }
    
    rangelua::cleanup();
}
BENCHMARK(BM_StateCreation);

BENCHMARK_MAIN();
