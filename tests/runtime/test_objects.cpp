/**
 * @file test_objects.cpp
 * @brief Comprehensive unit tests for runtime objects (Table, Function, Userdata, Coroutine)
 * @version 0.1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <rangelua/runtime/objects.hpp>
#include <rangelua/runtime/value.hpp>
#include <rangelua/runtime/gc.hpp>

using rangelua::runtime::Table;
using rangelua::runtime::Function;
using rangelua::runtime::Userdata;
using rangelua::runtime::Coroutine;
using rangelua::runtime::Value;
using rangelua::runtime::ValueType;
using rangelua::runtime::makeGCObject;
using rangelua::runtime::GCPtr;
using rangelua::runtime::GCObject;
using rangelua::String;
using rangelua::Number;
using rangelua::Size;

namespace vf = rangelua::runtime::value_factory;

TEST_CASE("Table basic operations", "[objects][table]") {
    SECTION("Table creation and basic operations") {
        auto table = makeGCObject<Table>();

        REQUIRE(table);
        REQUIRE(table->arraySize() == 0);
        REQUIRE(table->hashSize() == 0);
        REQUIRE(table->totalSize() == 0);
    }

    SECTION("Array operations") {
        auto table = makeGCObject<Table>();

        // Test array indexing (1-based)
        Value val1(42.0);
        Value val2("hello");

        table->setArray(1, val1);
        table->setArray(2, val2);

        REQUIRE(table->arraySize() == 2);

        Value retrieved1 = table->getArray(1);
        REQUIRE(retrieved1.is_number());
        auto num_result = retrieved1.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(42.0));

        Value retrieved2 = table->getArray(2);
        REQUIRE(retrieved2.is_string());
        auto str_result = retrieved2.to_string();
        REQUIRE(std::holds_alternative<String>(str_result));
        REQUIRE(std::get<String>(str_result) == "hello");

        // Test out-of-bounds access
        Value nil_val = table->getArray(10);
        REQUIRE(nil_val.is_nil());

        // Test 0-index (should be ignored)
        table->setArray(0, Value(999.0));
        REQUIRE(table->arraySize() == 2);  // Should not change
    }

    SECTION("Hash operations") {
        auto table = makeGCObject<Table>();

        Value key1("name");
        Value val1("RangeLua");
        Value key2(42.0);
        Value val2("answer");

        table->set(key1, val1);
        table->set(key2, val2);

        REQUIRE(table->hashSize() == 2);

        Value retrieved1 = table->get(key1);
        REQUIRE(retrieved1.is_string());
        auto str_result = retrieved1.to_string();
        REQUIRE(std::holds_alternative<String>(str_result));
        REQUIRE(std::get<String>(str_result) == "RangeLua");

        Value retrieved2 = table->get(key2);
        REQUIRE(retrieved2.is_string());
        auto str_result2 = retrieved2.to_string();
        REQUIRE(std::holds_alternative<String>(str_result2));
        REQUIRE(std::get<String>(str_result2) == "answer");

        // Test has() method
        REQUIRE(table->has(key1));
        REQUIRE(table->has(key2));
        REQUIRE_FALSE(table->has(Value("nonexistent")));

        // Test remove
        table->remove(key1);
        REQUIRE_FALSE(table->has(key1));
        REQUIRE(table->hashSize() == 1);
    }

    SECTION("Mixed array and hash operations") {
        auto table = makeGCObject<Table>();

        // Add array elements
        table->setArray(1, Value(10.0));
        table->setArray(2, Value(20.0));

        // Add hash elements
        table->set(Value("key"), Value("value"));

        REQUIRE(table->arraySize() == 2);
        REQUIRE(table->hashSize() == 1);
        REQUIRE(table->totalSize() == 3);

        // Test that numeric keys go to array when appropriate
        table->set(Value(3.0), Value(30.0));  // Should go to array
        REQUIRE(table->arraySize() == 3);

        Value retrieved = table->getArray(3);
        REQUIRE(retrieved.is_number());
        auto num_result = retrieved.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(30.0));
    }
}

TEST_CASE("Table iteration", "[objects][table][iteration]") {
    SECTION("Empty table iteration") {
        auto table = makeGCObject<Table>();

        auto begin_iter = table->begin();
        auto end_iter = table->end();

        REQUIRE(begin_iter == end_iter);
    }

    SECTION("Array-only iteration") {
        auto table = makeGCObject<Table>();

        table->setArray(1, Value(10.0));
        table->setArray(2, Value(20.0));
        table->setArray(3, Value(30.0));

        std::vector<std::pair<Value, Value>> collected;
        for (auto iter = table->begin(); iter != table->end(); ++iter) {
            collected.push_back(*iter);
        }

        REQUIRE(collected.size() == 3);

        // Check first element
        REQUIRE(collected[0].first.is_number());
        auto key_result = collected[0].first.to_number();
        REQUIRE(std::holds_alternative<Number>(key_result));
        REQUIRE(std::get<Number>(key_result) == Catch::Approx(1.0));

        REQUIRE(collected[0].second.is_number());
        auto val_result = collected[0].second.to_number();
        REQUIRE(std::holds_alternative<Number>(val_result));
        REQUIRE(std::get<Number>(val_result) == Catch::Approx(10.0));
    }

    SECTION("Hash-only iteration") {
        auto table = makeGCObject<Table>();

        table->set(Value("key1"), Value("value1"));
        table->set(Value("key2"), Value("value2"));

        std::vector<std::pair<Value, Value>> collected;
        for (const auto& pair : *table) {
            collected.push_back(pair);
        }

        REQUIRE(collected.size() == 2);
        // Note: Hash iteration order is not guaranteed
    }
}

TEST_CASE("Table metatable operations", "[objects][table][metatable]") {
    SECTION("Metatable setting and getting") {
        auto table = makeGCObject<Table>();
        auto metatable = makeGCObject<Table>();

        // Initially no metatable
        REQUIRE_FALSE(table->metatable());

        // Set metatable
        table->setMetatable(metatable);
        REQUIRE(table->metatable() == metatable);

        // Clear metatable
        table->setMetatable(GCPtr<Table>{});
        REQUIRE_FALSE(table->metatable());
    }
}

TEST_CASE("Function basic operations", "[objects][function]") {
    SECTION("C function creation and execution") {
        auto c_func = [](const std::vector<Value>& args) -> std::vector<Value> {
            if (args.empty()) return {Value(0.0)};

            // Sum all numeric arguments
            Number sum = 0.0;
            for (const auto& arg : args) {
                if (arg.is_number()) {
                    auto num_result = arg.to_number();
                    if (std::holds_alternative<Number>(num_result)) {
                        sum += std::get<Number>(num_result);
                    }
                }
            }
            return {Value(sum)};
        };

        auto function = makeGCObject<Function>(c_func);

        REQUIRE(function->isCFunction());
        REQUIRE_FALSE(function->isLuaFunction());
        REQUIRE(function->type() == Function::Type::C_FUNCTION);
        REQUIRE(function->parameterCount() == 0);  // Default for C functions

        // Test function call
        std::vector<Value> args = {Value(10.0), Value(20.0), Value(30.0)};
        auto result = function->call(args);

        REQUIRE(result.size() == 1);
        REQUIRE(result[0].is_number());
        auto num_result = result[0].to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(60.0));
    }

    SECTION("Lua function creation") {
        std::vector<rangelua::Instruction> bytecode = {0x12345678, 0x87654321};
        auto function = makeGCObject<Function>(std::move(bytecode), 2);

        REQUIRE_FALSE(function->isCFunction());
        REQUIRE(function->isLuaFunction());
        REQUIRE(function->type() == Function::Type::LUA_FUNCTION);
        REQUIRE(function->parameterCount() == 2);

        const auto& code = function->bytecode();
        REQUIRE(code.size() == 2);
        REQUIRE(code[0] == 0x12345678);
        REQUIRE(code[1] == 0x87654321);
    }
}

TEST_CASE("Function upvalue operations", "[objects][function][upvalues]") {
    SECTION("Upvalue management") {
        auto function = makeGCObject<Function>(std::vector<rangelua::Instruction>{}, 0);

        REQUIRE(function->upvalueCount() == 0);

        // Add upvalues
        function->addUpvalue(Value(42.0));
        function->addUpvalue(Value("hello"));

        REQUIRE(function->upvalueCount() == 2);
        REQUIRE(function->type() == Function::Type::CLOSURE);  // Should become closure

        // Get upvalues
        Value upval1 = function->getUpvalue(0);
        REQUIRE(upval1.is_number());
        auto num_result = upval1.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(42.0));

        Value upval2 = function->getUpvalue(1);
        REQUIRE(upval2.is_string());
        auto str_result = upval2.to_string();
        REQUIRE(std::holds_alternative<String>(str_result));
        REQUIRE(std::get<String>(str_result) == "hello");

        // Test out-of-bounds access
        Value nil_upval = function->getUpvalue(10);
        REQUIRE(nil_upval.is_nil());

        // Set upvalue
        function->setUpvalue(0, Value(99.0));
        Value modified_upval = function->getUpvalue(0);
        REQUIRE(modified_upval.is_number());
        auto mod_num_result = modified_upval.to_number();
        REQUIRE(std::holds_alternative<Number>(mod_num_result));
        REQUIRE(std::get<Number>(mod_num_result) == Catch::Approx(99.0));
    }
}

TEST_CASE("Userdata basic operations", "[objects][userdata]") {
    SECTION("Userdata creation and basic access") {
        int test_data = 42;
        auto userdata = makeGCObject<Userdata>(&test_data, sizeof(int), "int");

        REQUIRE(userdata->data() == &test_data);
        REQUIRE(userdata->size() == sizeof(int));
        REQUIRE(userdata->typeName() == "int");
        REQUIRE(userdata->userValueCount() == 0);

        // Initially no metatable
        REQUIRE_FALSE(userdata->metatable());
    }

    SECTION("Type-safe access") {
        struct TestStruct {
            int value;
            std::string name;
        };

        TestStruct test_obj{123, "test"};
        auto userdata = makeGCObject<Userdata>(&test_obj, sizeof(TestStruct), typeid(TestStruct).name());

        // Test type checking
        REQUIRE(userdata->is<TestStruct>());
        REQUIRE_FALSE(userdata->is<int>());

        // Test type-safe access
        TestStruct* ptr = userdata->as<TestStruct>();
        REQUIRE(ptr != nullptr);
        REQUIRE(ptr->value == 123);
        REQUIRE(ptr->name == "test");

        // Test invalid type access
        int* invalid_ptr = userdata->as<int>();
        REQUIRE(invalid_ptr == nullptr);
    }

    SECTION("User values") {
        int test_data = 42;
        auto userdata = makeGCObject<Userdata>(&test_data, sizeof(int), "int");

        // Set user values
        userdata->setUserValue(0, Value(100.0));
        userdata->setUserValue(1, Value("user_value"));

        REQUIRE(userdata->userValueCount() == 2);

        // Get user values
        Value uval1 = userdata->getUserValue(0);
        REQUIRE(uval1.is_number());
        auto num_result = uval1.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(100.0));

        Value uval2 = userdata->getUserValue(1);
        REQUIRE(uval2.is_string());
        auto str_result = uval2.to_string();
        REQUIRE(std::holds_alternative<String>(str_result));
        REQUIRE(std::get<String>(str_result) == "user_value");

        // Test out-of-bounds access
        Value nil_uval = userdata->getUserValue(10);
        REQUIRE(nil_uval.is_nil());
    }

    SECTION("Metatable operations") {
        int test_data = 42;
        auto userdata = makeGCObject<Userdata>(&test_data, sizeof(int), "int");
        auto metatable = makeGCObject<Table>();

        // Set metatable
        userdata->setMetatable(metatable);
        REQUIRE(userdata->metatable() == metatable);

        // Clear metatable
        userdata->setMetatable(GCPtr<Table>{});
        REQUIRE_FALSE(userdata->metatable());
    }
}

TEST_CASE("Coroutine basic operations", "[objects][coroutine]") {
    SECTION("Coroutine creation and status") {
        auto coroutine = makeGCObject<Coroutine>(100);

        REQUIRE(coroutine->status() == Coroutine::Status::SUSPENDED);
        REQUIRE(coroutine->isResumable());
        REQUIRE(coroutine->stackEmpty());
        REQUIRE(coroutine->stackSize() == 0);
        REQUIRE_FALSE(coroutine->hasError());
    }

    SECTION("Stack operations") {
        auto coroutine = makeGCObject<Coroutine>(100);

        // Push values onto stack
        coroutine->push(Value(42.0));
        coroutine->push(Value("hello"));
        coroutine->push(Value(true));

        REQUIRE(coroutine->stackSize() == 3);
        REQUIRE_FALSE(coroutine->stackEmpty());

        // Check top value
        Value top = coroutine->top();
        REQUIRE(top.is_boolean());
        auto bool_result = top.to_boolean();
        REQUIRE(std::holds_alternative<bool>(bool_result));
        REQUIRE(std::get<bool>(bool_result) == true);

        // Pop values
        Value popped1 = coroutine->pop();
        REQUIRE(popped1.is_boolean());
        REQUIRE(coroutine->stackSize() == 2);

        Value popped2 = coroutine->pop();
        REQUIRE(popped2.is_string());
        auto str_result = popped2.to_string();
        REQUIRE(std::holds_alternative<String>(str_result));
        REQUIRE(std::get<String>(str_result) == "hello");

        Value popped3 = coroutine->pop();
        REQUIRE(popped3.is_number());
        auto num_result = popped3.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(42.0));

        REQUIRE(coroutine->stackEmpty());

        // Pop from empty stack should return nil
        Value empty_pop = coroutine->pop();
        REQUIRE(empty_pop.is_nil());
    }

    SECTION("Yield and resume operations") {
        auto coroutine = makeGCObject<Coroutine>(100);

        // Test basic resume (should transition to DEAD for now)
        auto result1 = coroutine->resume();
        REQUIRE(coroutine->status() == Coroutine::Status::DEAD);
        REQUIRE_FALSE(coroutine->isResumable());

        // Create new coroutine for yield test
        auto coroutine2 = makeGCObject<Coroutine>(100);

        // Test resume with arguments
        std::vector<Value> args = {Value(1.0), Value(2.0), Value(3.0)};
        auto result2 = coroutine2->resume(args);

        // Arguments should be on the stack
        REQUIRE(coroutine2->stackSize() == 3);
    }

    SECTION("Error handling") {
        auto coroutine = makeGCObject<Coroutine>(100);

        REQUIRE_FALSE(coroutine->hasError());
        REQUIRE(coroutine->error().empty());

        // Set error
        coroutine->setError("Test error message");

        REQUIRE(coroutine->hasError());
        REQUIRE(coroutine->error() == "Test error message");
        REQUIRE(coroutine->status() == Coroutine::Status::DEAD);
    }
}

TEST_CASE("Object GC integration", "[objects][gc][integration]") {
    SECTION("Object creation and GC registration") {
        // Test that objects are properly registered with GC
        auto table = makeGCObject<Table>();
        auto function = makeGCObject<Function>([](const std::vector<Value>&) { return std::vector<Value>{}; });
        auto userdata = makeGCObject<Userdata>(nullptr, 0, "test");
        auto coroutine = makeGCObject<Coroutine>();

        REQUIRE(table);
        REQUIRE(function);
        REQUIRE(userdata);
        REQUIRE(coroutine);

        // Test object sizes
        REQUIRE(table->objectSize() >= sizeof(Table));
        REQUIRE(function->objectSize() >= sizeof(Function));
        REQUIRE(userdata->objectSize() >= sizeof(Userdata));
        REQUIRE(coroutine->objectSize() >= sizeof(Coroutine));
    }

    SECTION("Object traversal for GC") {
        auto table = makeGCObject<Table>();
        auto nested_table = makeGCObject<Table>();

        // Create references between objects
        table->set(Value("nested"), Value(nested_table));

        // Test traversal
        std::vector<GCObject*> visited;
        table->traverse([&visited](GCObject* obj) {
            visited.push_back(obj);
        });

        // Should visit the nested table
        REQUIRE(visited.size() == 1);
        REQUIRE(visited[0] == nested_table.get());
    }

    SECTION("Circular reference handling") {
        auto table1 = makeGCObject<Table>();
        auto table2 = makeGCObject<Table>();

        // Create circular reference
        table1->set(Value("ref"), Value(table2));
        table2->set(Value("back_ref"), Value(table1));

        // Objects should still be accessible
        REQUIRE(table1->has(Value("ref")));
        REQUIRE(table2->has(Value("back_ref")));

        // Test traversal doesn't infinite loop (basic test)
        std::vector<GCObject*> visited;
        table1->traverse([&visited](GCObject* obj) {
            visited.push_back(obj);
        });

        REQUIRE(visited.size() == 1);
        REQUIRE(visited[0] == table2.get());

        // Manually break the circular references to prevent memory leaks
        // This simulates what the GC cycle detection should do
        table1->remove(Value("ref"));
        table2->remove(Value("back_ref"));

        // Verify references are broken
        REQUIRE_FALSE(table1->has(Value("ref")));
        REQUIRE_FALSE(table2->has(Value("back_ref")));

        // Clear the GCPtr references to allow proper cleanup
        // This ensures the objects can be properly destroyed when their reference count reaches 0
        table1.reset();
        table2.reset();
    }
}

TEST_CASE("Object edge cases and error handling", "[objects][edge_cases]") {
    SECTION("Table edge cases") {
        auto table = makeGCObject<Table>();

        // Test setting nil values
        table->set(Value("key"), Value{});
        REQUIRE(table->has(Value("key")));

        Value retrieved = table->get(Value("key"));
        REQUIRE(retrieved.is_nil());

        // Test very large array indices
        table->setArray(1000, Value(42.0));
        REQUIRE(table->arraySize() == 1000);

        Value large_val = table->getArray(1000);
        REQUIRE(large_val.is_number());
        auto num_result = large_val.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(42.0));
    }

    SECTION("Function edge cases") {
        // Test function with no parameters
        auto func = makeGCObject<Function>([](const std::vector<Value>&) {
            return std::vector<Value>{Value("no_params")};
        });

        auto result = func->call({});
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].is_string());

        // Test function with many upvalues
        for (int i = 0; i < 100; ++i) {
            func->addUpvalue(Value(static_cast<Number>(i)));
        }

        REQUIRE(func->upvalueCount() == 100);

        Value upval_50 = func->getUpvalue(50);
        REQUIRE(upval_50.is_number());
        auto num_result = upval_50.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(50.0));
    }

    SECTION("Userdata edge cases") {
        // Test userdata with zero size
        auto userdata = makeGCObject<Userdata>(nullptr, 0, "empty");
        REQUIRE(userdata->size() == 0);
        REQUIRE(userdata->data() == nullptr);

        // Test many user values
        for (int i = 0; i < 50; ++i) {
            userdata->setUserValue(i, Value(static_cast<Number>(i * 2)));
        }

        REQUIRE(userdata->userValueCount() == 50);

        Value uval_25 = userdata->getUserValue(25);
        REQUIRE(uval_25.is_number());
        auto num_result = uval_25.to_number();
        REQUIRE(std::holds_alternative<Number>(num_result));
        REQUIRE(std::get<Number>(num_result) == Catch::Approx(50.0));
    }

    SECTION("Coroutine edge cases") {
        auto coroutine = makeGCObject<Coroutine>(10);  // Small stack

        // Fill stack to capacity
        for (int i = 0; i < 10; ++i) {
            coroutine->push(Value(static_cast<Number>(i)));
        }

        REQUIRE(coroutine->stackSize() == 10);

        // Test stack overflow handling (should not crash)
        coroutine->push(Value(999.0));
        REQUIRE(coroutine->stackSize() == 11);  // Should grow

        // Test multiple errors
        coroutine->setError("First error");
        coroutine->setError("Second error");
        REQUIRE(coroutine->error() == "Second error");  // Should overwrite
    }
}
