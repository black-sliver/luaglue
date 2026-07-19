#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <gtest/gtest.h>
#include "macros.hpp"


class LuaTestBase : public testing::Test {
protected:
    lua_State* L;

    LuaTestBase()
    {
        L = luaL_newstate();
        const std::initializer_list<const luaL_Reg> luaLibs = {
            {LUA_GNAME, luaopen_base},
            {LUA_STRLIBNAME, luaopen_string},
          };
        for (const auto& lib: luaLibs) {
            luaL_requiref(L, lib.name, lib.func, 1);
            lua_pop(L, 1);
        }
    }

    ~LuaTestBase() override
    {
        lua_close(L);
        L = nullptr;
    }

    NODISCARD
    bool doString(const char* s) const
    {
        const bool res = luaL_dostring(L, s) == LUA_OK;
        if (!res)
            printf("%s\n", lua_tostring(L, -1));
        return res;
    }
};
