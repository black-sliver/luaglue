#pragma once

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}
#include <gtest/gtest.h>
#include "macros.hpp"


class LuaTestBase : public testing::Test {
protected:
    lua_State* L;

    LuaTestBase()
    {
        L = luaL_newstate();
    }

    ~LuaTestBase() override
    {
        lua_close(L);
        L = nullptr;
    }

    NODISCARD
    bool doString(const char* s) const
    {
        return luaL_dostring(L, s) == LUA_OK;
    }
};
