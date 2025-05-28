/**
 * @file test_closure.cpp
 * @brief Test closure functionality
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <rangelua/backend/bytecode.hpp>
#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/vm.hpp>

using rangelua::runtime::Value;
using rangelua::runtime::Upvalue;
using rangelua::runtime::Function;
using rangelua::runtime::makeGCObject;
using rangelua::backend::BytecodeEmitter;
using rangelua::backend::UpvalueDescriptor;
using rangelua::backend::FunctionPrototype;
using rangelua::Instruction;
using rangelua::Size;
using rangelua::Number;
using rangelua::String;

namespace vf = rangelua::runtime::value_factory;

TEST_CASE("Upvalue creation and management", "[closure][upvalue]") {
    SECTION("Create open upvalue") {
        Value stack_value = vf::number(42.0);
        auto upvalue = makeGCObject<Upvalue>(&stack_value);

        REQUIRE(upvalue->isOpen());
        REQUIRE_FALSE(upvalue->isClosed());
        REQUIRE(upvalue->getValue().is_number());
        auto num_result = upvalue->getValue().to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == 42.0);
    }

    SECTION("Create closed upvalue") {
        Value value = vf::string("hello");
        auto upvalue = makeGCObject<Upvalue>(value);

        REQUIRE_FALSE(upvalue->isOpen());
        REQUIRE(upvalue->isClosed());
        REQUIRE(upvalue->getValue().is_string());
        auto str_result = upvalue->getValue().to_string();
        REQUIRE(std::holds_alternative<String>(str_result));
        REQUIRE(std::get<String>(str_result) == "hello");
    }

    SECTION("Close open upvalue") {
        Value stack_value = vf::number(123.0);
        auto upvalue = makeGCObject<Upvalue>(&stack_value);

        REQUIRE(upvalue->isOpen());

        upvalue->close();

        REQUIRE(upvalue->isClosed());
        REQUIRE(upvalue->getValue().is_number());
        auto num_result2 = upvalue->getValue().to_number();
        REQUIRE(std::holds_alternative<Number>(num_result2));
        REQUIRE(std::get<Number>(num_result2) == 123.0);
    }
}

TEST_CASE("Function closure creation", "[closure][function]") {
    SECTION("Create closure with upvalues") {
        std::vector<Instruction> bytecode = {0x12345678};  // Dummy bytecode
        auto closure = vf::closure(bytecode, 1);

        REQUIRE(closure.is_function());
        auto func_ptr = closure.to_function();
        REQUIRE(std::holds_alternative<Value::FunctionPtr>(func_ptr));

        auto function = std::get<Value::FunctionPtr>(func_ptr);
        REQUIRE(function->isClosure());
        REQUIRE(function->parameterCount() == 1);
    }

    SECTION("Add upvalues to closure") {
        std::vector<Instruction> bytecode = {0x12345678};
        std::vector<Value> upvalues = {
            vf::number(42.0),
            vf::string("test")
        };

        auto closure = vf::closure(bytecode, upvalues, 0);
        auto func_ptr = std::get<Value::FunctionPtr>(closure.to_function());

        REQUIRE(func_ptr->upvalueCount() == 2);
        REQUIRE(func_ptr->getUpvalueValue(0).is_number());
        REQUIRE(func_ptr->getUpvalueValue(1).is_string());
    }
}

TEST_CASE("Bytecode upvalue descriptors", "[closure][bytecode]") {
    SECTION("Create upvalue descriptor") {
        UpvalueDescriptor desc("test_var", true, 5);

        REQUIRE(desc.name == "test_var");
        REQUIRE(desc.in_stack == true);
        REQUIRE(desc.index == 5);
    }

    SECTION("Add upvalue descriptor to function prototype") {
        FunctionPrototype prototype;
        prototype.name = "test_function";
        prototype.upvalue_descriptors.emplace_back("x", true, 0);
        prototype.upvalue_descriptors.emplace_back("y", false, 1);

        REQUIRE(prototype.upvalue_descriptors.size() == 2);
        REQUIRE(prototype.upvalue_descriptors[0].name == "x");
        REQUIRE(prototype.upvalue_descriptors[0].in_stack == true);
        REQUIRE(prototype.upvalue_descriptors[1].name == "y");
        REQUIRE(prototype.upvalue_descriptors[1].in_stack == false);
    }
}

TEST_CASE("BytecodeEmitter upvalue support", "[closure][bytecode]") {
    SECTION("Add upvalue descriptors") {
        BytecodeEmitter emitter;

        Size idx1 = emitter.add_upvalue_descriptor("var1", true, 0);
        Size idx2 = emitter.add_upvalue_descriptor("var2", false, 1);

        REQUIRE(idx1 == 0);
        REQUIRE(idx2 == 1);

        auto function = emitter.get_function();
        REQUIRE(function.upvalue_descriptors.size() == 2);
        REQUIRE(function.upvalue_descriptors[0].name == "var1");
        REQUIRE(function.upvalue_descriptors[1].name == "var2");
    }

    SECTION("Add function prototype") {
        BytecodeEmitter emitter;

        FunctionPrototype prototype;
        prototype.name = "nested_func";
        prototype.parameter_count = 2;

        Size idx = emitter.add_prototype(prototype);
        REQUIRE(idx == 0);

        auto function = emitter.get_function();
        REQUIRE(function.prototypes.size() == 1);
        REQUIRE(function.prototypes[0].name == "nested_func");
        REQUIRE(function.prototypes[0].parameter_count == 2);
    }
}
