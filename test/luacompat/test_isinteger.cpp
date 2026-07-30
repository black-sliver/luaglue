#include <limits>
#include <gtest/gtest.h>
#include "../luatestbase.hpp"
#include "../../luacompat.h"


class LuaIsIntegerTest : public LuaTestBase
{};

// smallest / biggest integer that can be represented in 32/64bit float
#define MIN_SAFE_INTEGER_32 -16777215 // -(2^24 - 1)
#define MAX_SAFE_INTEGER_32  16777215 // 2^24 - 1
#define MIN_SAFE_INTEGER_64 -9007199254740991ll // -(2^53 - 1)
#define MAX_SAFE_INTEGER_64  9007199254740991ll // 2^53 - 1

TEST_F(LuaIsIntegerTest, ZeroInteger) {
    lua_pushinteger(L, 0);
    EXPECT_TRUE(lua_isinteger(L, -1));
}

TEST_F(LuaIsIntegerTest, ZeroNumber) {
    // NOTE: this is false in Lua >= 5.3, because the type system handles IsInteger,
    //       but it's true in Lua <= 5.2, because we have to guess from the value
    //       Beware that the 5.3+ behavior means that json_to_lua has to push the correct type.
    lua_pushnumber(L, 0.0);
#if LUA_VERSION_NUM >= 503
    EXPECT_FALSE(lua_isinteger(L, -1));
#else
    EXPECT_TRUE(lua_isinteger(L, -1));
#endif
}

TEST_F(LuaIsIntegerTest, MinInt) {
    // FIXME: is this correct? doesn't this depend on the precision?
    //        before 5.3, this should only be true if lua_Integer is 32bit
    lua_pushinteger(L, std::numeric_limits<lua_Integer>::min());
#if LUA_VERSION_NUM >= 503
    // Since 5.3, IsInteger is part of the type
    EXPECT_TRUE(lua_isinteger(L, -1));
#else
    // Before 5.3, we have to guess from value
    if (sizeof(lua_Number) > sizeof(lua_Integer))
        EXPECT_TRUE(lua_isinteger(L, -1));
    else
        EXPECT_FALSE(lua_isinteger(L, -1));
#endif
}

TEST_F(LuaIsIntegerTest, MaxInt) {
    lua_pushinteger(L, std::numeric_limits<lua_Integer>::max());
#if LUA_VERSION_NUM >= 503
    // Since 5.3, IsInteger is part of the type
    EXPECT_TRUE(lua_isinteger(L, -1));
#else
    // Before 5.3, we have to guess from value
    if (sizeof(lua_Number) > sizeof(lua_Integer))
        EXPECT_TRUE(lua_isinteger(L, -1));
    else
        EXPECT_FALSE(lua_isinteger(L, -1));
#endif
}

TEST_F(LuaIsIntegerTest, MinSafeInt) {
    if (sizeof(lua_Integer) == 8) {
        lua_pushinteger(L, MIN_SAFE_INTEGER_64);
    } else if (sizeof(lua_Integer) == 4 && sizeof(lua_Number) == 4) {
        lua_pushinteger(L, MIN_SAFE_INTEGER_32);
    } else if (sizeof(lua_Integer) == 4) {
        lua_pushinteger(L, std::numeric_limits<lua_Integer>::min());
    } else {
        FAIL() << "Unsupported lua_Integer or lua_Number size";
    }
    EXPECT_TRUE(lua_isinteger(L, -1));
}

TEST_F(LuaIsIntegerTest, MaxSafeInt) {
    if (sizeof(lua_Integer) == 8) {
        lua_pushinteger(L, MAX_SAFE_INTEGER_64);
    } else if (sizeof(lua_Integer) == 4 && sizeof(lua_Number) == 4) {
        lua_pushinteger(L, MAX_SAFE_INTEGER_32);
    } else if (sizeof(lua_Integer) == 4) {
        lua_pushinteger(L, std::numeric_limits<lua_Integer>::max());
    } else {
        FAIL() << "Unsupported lua_Integer or lua_Number size";
    }
    EXPECT_TRUE(lua_isinteger(L, -1));
}

TEST_F(LuaIsIntegerTest, Fraction) {
    lua_pushnumber(L, 0.5);
    EXPECT_FALSE(lua_isinteger(L, -1));
}

TEST_F(LuaIsIntegerTest, TooBig) {
    constexpr lua_Number n = static_cast<lua_Number>(std::numeric_limits<lua_Integer>::max()) + 10.0;
    lua_pushnumber(L, n);
    ASSERT_EQ(n, lua_tonumber(L, -1));
    EXPECT_FALSE(lua_isinteger(L, -1));
}

TEST_F(LuaIsIntegerTest, TooSmall) {
    constexpr lua_Number n = static_cast<lua_Number>(std::numeric_limits<lua_Integer>::min()) - 24576.0;
    // -9223372036854800384.0 != -9223372036854775808.0
    ASSERT_TRUE(n < static_cast<lua_Number>(std::numeric_limits<lua_Integer>::min()));
    lua_pushnumber(L, n);
    ASSERT_EQ(n, lua_tonumber(L, -1));
    EXPECT_FALSE(lua_isinteger(L, -1));
}
