#ifndef _LUAGLUE_LUATYPE_H
#define _LUAGLUE_LUATYPE_H

#include "lua_include.h"
#include "luaref.h"

/* Interface to have c++ push a value to lua, see luainterface.h for the actual magic */
class LuaType {
protected:
    LuaType(){}
public:
    virtual void Lua_Push(lua_State *L) const = 0;
    virtual ~LuaType() {}
};

#endif // _LUAGLUE_LUATYPE_H
