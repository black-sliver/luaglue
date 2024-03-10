#ifndef _LUAGLUE_LUAPP_H
#define _LUAGLUE_LUAPP_H

#include "lua_include.h"
#include "luatype.h"
#include "luaref.h"
#include <limits>
#include <stdint.h>
#include <stdexcept>


// Note: we basically wrap lua just to have parameter overloading
// we may want to actually fully wrap lua to manage the life cycle?
class Lua final {
private:
#if __cplusplus < 201700L
    // Get not implemented for C++14
#else
    // static_assert helper, since we can not directly assert in if constexpr
    template<bool flag = false>
    static void static_unsupported_type() { static_assert(flag, "unsupported type"); }
#endif

public:
    Lua(lua_State *L) : L(L) {}
    
    template<class T>
    void Register()
    {
        T::Lua_Register(L);
    }
    
    void Push(const LuaType& o)
    {
        o.Lua_Push(L);
    }
    void Push(const LuaType* o)
    {
        o->Lua_Push(L);
    }
    void Push(const char* s)
    {
        lua_pushstring(L, s);
    }
    void Push(const std::string& s)
    {
        lua_pushstring(L, s.c_str());
    }
    void Push(lua_Number n)
    {
        lua_pushnumber(L, n);
    }
    void Push(int32_t i)
    {
        lua_pushinteger(L, i);
    }
    void Push(int64_t n)
    {
        if (n < std::numeric_limits<lua_Integer>::min() || n > std::numeric_limits<lua_Integer>::max())
            lua_pushnumber(L, (lua_Number)n);
        else
            lua_pushinteger(L, (lua_Integer)n);
    }
    void Push(bool b)
    {
        lua_pushboolean(L, b);
    }
    void PushNil()
    {
        lua_pushnil(L);
    }
    void Push(std::nullptr_t)
    {
        lua_pushnil(L);
    }

#if __cplusplus < 201700L
    // Get not implemented for C++14
#else
    template<class T>
    T Get(int idx) {
        if constexpr (std::is_same<T, bool>::value) {
#ifdef LUA_TBOOLEAN
            if (lua_isboolean(L, idx))
                return lua_toboolean(L, idx) != 0;
#endif
            if (lua_isinteger(L, idx))
                return luaL_checkinteger(L, idx) != 0;
#ifndef LUA_TBOOLEAN
            // older Lua uses true != nil
            return lua_toboolean(L, idx);
#else
            // newer Lua can detect wrong type
            throw std::invalid_argument("not a boolean");
#endif
        } else {
            static_unsupported_type();
        }
    }
#endif

private:
    lua_State *L;
};


#endif // _LUAGLUE_LUAPP_H
