# C++ Lua Glue Code

This header-only library provides

* `LuaType` in `luatype.h`

        an interface providing Lua_Push(lua_State*) to push a variable to lua

* `LuaVariant` in `luavariant.h`

        a LuaType that is a union of possible types received from lua

* `LuaRef` in `luaref.h`

        a wrapper around int to store lua references (from `luaL_ref`)

* `LuaMethodWrapper` and `LUA_METHOD` macro in `luamethod.h`

        recursive template struct to map c++ methods to Lua functions
        see LuaInterface<class>::MethodMap for use

* `LuaInterface<class>` in `lainterface.h`

        a way to map c++ classes as user types into lua

* `Lua` in `luapp.h`

        a simple wrapper around lua_State that
        provides lua_push* calls through overloaded ::Push

* Helper functions in `lua_utils.h`

        lua_dumpstack(lua_State*) dumps the current stack

* JSON helper functions in `lua_json.h`

        json lua_to_json(lua_State*) creates json from lua stack
        json_to_lua(lua_State*, json&) pushes json to lua stack

* `lua_*` functions in `luacompat.h` are used to be able to target lua5.1 and lua5.2


## TODO

* Member getters/setters: index and newindex for non-methods has to be done manually

* Alternative method signature that captures, passes or validates the lua_State against the stored state for
  multi-threading / coroutines.

* Some way to make variable argument methods work nicely

* A nice way to map static functions


## Usage

See PopTracker/src/core/luaitem.*, et al for now.
