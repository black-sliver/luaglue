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
        { statement; EXPECT_TRUE(typeCheck(L, -1)); }
#endif


class JsonToLuaTest : public LuaTestBase {};

TEST_F(JsonToLuaTest, Array) {
    json_to_lua(L, json::parse("[2]"));
    EXPECT_EQ(lua_type(L, -1), LUA_TTABLE);
    lua_geti(L, -1, 1);
    EXPECT_EQ(lua_tointeger(L, -1), 2);
}

TEST_F(JsonToLuaTest, Dict) {
    json_to_lua(L, json::parse("{\"a\": 2}"));
    EXPECT_EQ(lua_type(L, -1), LUA_TTABLE);
    lua_getfield(L, -1, "a");
    EXPECT_EQ(lua_tointeger(L, -1), 2);
}

TEST_F(JsonToLuaTest, String) {
    const std::string val = "Test";
    json_to_lua(L, val);
    EXPECT_EQ(lua_tostring(L, -1), val);
}

TEST_F(JsonToLuaTest, Uint) {
    // push uint that doesn't fit into lua_Integer
    if constexpr(sizeof(lua_Number) == sizeof(double) && sizeof(lua_Integer) == sizeof(uint64_t)) {
        constexpr uint64_t n = std::numeric_limits<uint64_t>::max();
        json_to_lua(L, n);
        EXPECT_DOUBLE_EQ(lua_tonumber(L, -1), static_cast<lua_Number>(n));
    } else if constexpr(sizeof(lua_Number) == sizeof(double) && sizeof(lua_Integer) == sizeof(uint32_t)) {
        constexpr uint64_t n = std::numeric_limits<uint32_t>::max();
        json_to_lua(L, n);
        EXPECT_DOUBLE_EQ(lua_tonumber(L, -1), static_cast<lua_Number>(n));
    } else if constexpr(sizeof(lua_Number) == sizeof(float) && sizeof(lua_Integer) == sizeof(uint32_t)) {
        constexpr uint64_t n = std::numeric_limits<uint32_t>::max();
        json_to_lua(L, n);
        EXPECT_FLOAT_EQ(lua_tonumber(L, -1), static_cast<lua_Number>(n));
    } else {
        FAIL() << "Unsupported lua_Number ir lua_Integer size";
    }

    if constexpr (sizeof(lua_Integer) == sizeof(int64_t)) {
        constexpr uint64_t m = std::numeric_limits<int64_t>::max();
        json_to_lua(L, m);
        EXPECT_TRUE(lua_isinteger(L, -1));
        EXPECT_EQ(lua_tointeger(L, -1), std::numeric_limits<lua_Integer>::max());
    } else if constexpr (sizeof(lua_Integer) == sizeof(int32_t)) {
        constexpr uint64_t m = std::numeric_limits<int32_t>::max();
        json_to_lua(L, m);
        EXPECT_TRUE(lua_isinteger(L, -1));
        EXPECT_EQ(lua_tointeger(L, -1), std::numeric_limits<lua_Integer>::max());
    } else {
        FAIL() << "Unsupported lua_Integer size";
    }
}

TEST_F(JsonToLuaTest, Int) {
    if constexpr (sizeof(lua_Integer) == sizeof(int64_t)) {
        int64_t m = std::numeric_limits<int64_t>::min();
        json_to_lua(L, m);
        EXPECT_TRUE(lua_isinteger(L, -1));
        EXPECT_EQ(lua_tointeger(L, -1), std::numeric_limits<lua_Integer>::min());
    } else if constexpr (sizeof(lua_Integer) == sizeof(int32_t)) {
        int64_t m = std::numeric_limits<int32_t>::min();
        json_to_lua(L, m);
        EXPECT_TRUE(lua_isinteger(L, -1));
        EXPECT_EQ(lua_tointeger(L, -1), std::numeric_limits<lua_Integer>::min());
    } else {
        FAIL() << "Unsupported lua_Integer size";
    }
}

TEST_F(JsonToLuaTest, Number) {
    if constexpr (sizeof(lua_Number) == sizeof(float)) {
        json_to_lua(L, 1.23f);
        EXPECT_FLOAT_EQ(lua_tonumber(L, -1), 1.23f);
    } else if constexpr (sizeof(lua_Number) == sizeof(double)) {
        json_to_lua(L, 1.23);
        EXPECT_DOUBLE_EQ(lua_tonumber(L, -1), 1.23);
    } else {
        FAIL() << "Unsupported lua_Number size";
    }
}

TEST_F(JsonToLuaTest, ArrayTooDeep) {
    std::string s = std::string(100000, '[') + std::string(100000, ']');
    EXPECT_RECURSIVE(json_to_lua(L, json::parse(s)), lua_istable);
    EXPECT_LE(lua_gettop(L), 1);
}

TEST_F(JsonToLuaTest, DictTooDeep) {
    std::string s;
    for (unsigned n = 0; n < 100000; n++)
        s += "{\"a\":";
    s += "{}";
    for (unsigned n = 0; n < 100000; n++)
        s += "}";
    EXPECT_RECURSIVE(json_to_lua(L, json::parse(s)), lua_istable);
    EXPECT_LE(lua_gettop(L), 1);
}

TEST_F(JsonToLuaTest, MixedTooDeep) {
    std::string s;
    for (unsigned n = 0; n < 100000; n++)
        s += "{\"a\":[";
    for (unsigned n = 0; n < 100000; n++)
        s += "]}";
    EXPECT_RECURSIVE(json_to_lua(L, json::parse(s)), lua_istable);
    EXPECT_LE(lua_gettop(L), 1);
}
