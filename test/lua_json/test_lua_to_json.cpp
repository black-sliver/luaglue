#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "../luatestbase.hpp"
#include "../macros.hpp"
#include "../../lua_json.h"


#ifdef USE_EXCEPTIONS
#   define EXPECT_RECURSIVE(statement, typeCheck) \
        EXPECT_THROW(statement, std::runtime_error);
#else
#   define EXPECT_RECURSIVE(statement, typeCheck) \
        EXPECT_TRUE(statement.typeCheck());
#endif


class LuaToJsonTest : public LuaTestBase {};

TEST_F(LuaToJsonTest, RecursiveDict) {
    ASSERT_TRUE(doString(R""""(
        x = {}
        x["a"] = x
        return x
    )""""));
    ASSERT_EQ(lua_gettop(L), 1);
    EXPECT_RECURSIVE(lua_to_json(L, -1), is_object);
    EXPECT_EQ(lua_gettop(L), 1);
}

TEST_F(LuaToJsonTest, RecursiveArray) {
    ASSERT_TRUE(doString(R""""(
        x = {}
        x[1] = x
        return x
    )""""));
    ASSERT_EQ(lua_gettop(L), 1);
    EXPECT_RECURSIVE(lua_to_json(L, -1), is_array);
    EXPECT_EQ(lua_gettop(L), 1);
}

TEST_F(LuaToJsonTest, IndirectRecursion) {
    ASSERT_TRUE(doString(R""""(
        x = {}
        y = {}
        x[1] = y
        y["a"] = x
        return x
    )""""));
    ASSERT_EQ(lua_gettop(L), 1);
    EXPECT_RECURSIVE(lua_to_json(L, -1), is_array);
    EXPECT_EQ(lua_gettop(L), 1);
}

TEST_F(LuaToJsonTest, MixedKeysDict) {
    // this will create an object and use an integer key
    ASSERT_TRUE(doString(R""""(
        return {
            ["1"] = 1,
            ["2"] = 2,
            ["3"] = 3,
            ["4"] = 4,
            [5] = 5,
            ["6"] = 6,
            ["7"] = 7,
            ["8"] = 8,
            ["9"] = 9
        }
    )""""));
    ASSERT_EQ(lua_gettop(L), 1);
    auto j = lua_to_json(L, -1);
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["2"], 2);
    EXPECT_EQ(lua_gettop(L), 1);
}


TEST_F(LuaToJsonTest, MixedKeysDict2) {
    // this will first create an array and then convert to object for string keys
    ASSERT_TRUE(doString(R""""(
        return {
            [1] = 1,
            [2] = 2,
            [3] = 3,
            [4] = 4,
            ["5"] = 5,
            [6] = 6,
            [7] = 7,
            [8] = 8,
            [9] = 9
        }
    )""""));
    ASSERT_EQ(lua_gettop(L), 1);
    auto j = lua_to_json(L, -1);
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["2"], 2);
    EXPECT_EQ(lua_gettop(L), 1);
}

TEST_F(LuaToJsonTest, EmptyArray) {
    LuaJson_EmptyArray::Lua_Register(L);
    LuaJson_EmptyArray{}.Lua_Push(L);
    EXPECT_EQ(lua_to_json(L, -1).dump(), "[]");
    EXPECT_EQ(lua_gettop(L), 1);
}

TEST_F(LuaToJsonTest, EmptyDict) {
    lua_newtable(L);
    EXPECT_EQ(lua_to_json(L, -1).dump(), "{}");
    EXPECT_EQ(lua_gettop(L), 1);
}

TEST_F(LuaToJsonTest, MixedDict) {
    // convert a mixed dict and validate the Lua stack is unchanged
    lua_pushinteger(L, 1); // stack pos 1
    ASSERT_TRUE(doString(R""""(
        return {
            "Test",
            {1, 2},
            {a=3, b=4}
        }
    )"""")); // stack pos 2
    lua_pushinteger(L, 2); // stack pos 3
    EXPECT_EQ(lua_gettop(L), 3);
    EXPECT_EQ(lua_to_json(L, -2).dump(), R""""(["Test",[1,2],{"a":3,"b":4}])"""");
    EXPECT_EQ(lua_to_json(L, 2).dump(), R""""(["Test",[1,2],{"a":3,"b":4}])"""");
    EXPECT_EQ(lua_gettop(L), 3);
    EXPECT_EQ(lua_tointeger(L, 1), 1);
    EXPECT_EQ(lua_tointeger(L, -1), 2);
}
