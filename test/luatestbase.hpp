#pragma once

#include <gtest/gtest.h>
#include "macros.hpp"
#include "../luacompat.h"
#include "../lua_include.h"


// some internal back compat for tests
#ifndef LUA_GNAME
#define LUA_GNAME "_G"
#endif

#if LUA_VERSION_NUM < 502
static void luaL_requiref(lua_State *L, const char *modname, lua_CFunction openf, int glb)
{
    lua_pushcfunction(L, openf);
    lua_pushstring(L, modname);
    lua_call(L, 1, 1);
    if (glb) {
        lua_pushvalue(L, -1);
        lua_setglobal(L, modname);
    }
}
#endif


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
