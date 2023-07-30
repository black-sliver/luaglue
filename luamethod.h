#ifndef _LUAGLUE_LUAMETHOD_H
#define _LUAGLUE_LUAMETHOD_H

#include "luapp.h" // provides function overloading for push
#include "luaref.h" // provides reference for lua references (functions, ...)
#include "luavariant.h"
#include "lua_utils.h" // wraps (most) possible lua results
#ifndef NO_LUAMETHOD_JSON
#include "lua_json.h"
#endif
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#ifdef DEBUG_LUA_METHOD
#   define LUAMETHOD_DEBUG_printf printf
#else
#   define LUAMETHOD_DEBUG_printf(...)
#endif

#ifndef STRINGIFY
#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) #s
#endif

// helper to generate a map name => func pointer
// TODO: (argument count to) allow overloads?
#define LUA_METHOD(CLASS, METHOD, ...) { STRINGIFY(METHOD), LuaMethod<CLASS,&CLASS::METHOD,__VA_ARGS__>::Func }
// (Tracker, AddItems, const char*) => {"AddItems", LuaMethod<Tracker, &Tracker::AddItems,const char*>::Func}

// static_assert helper, since we can not directly assert in if constexpr
template<bool flag = false>
static void static_unsupported_type() { static_assert(flag, "unsupported type"); }

// recursive helper. Types of already fetched args in class template,
//   Types of missing args in function template
template <class T, auto F, typename... Prev>
struct LuaMethodHelper
{
    template <typename Next, typename... Rest>
    static int get(lua_State *L, T *o, int n, Prev... prev)
    {
        if constexpr (std::is_same<Next, int>::value) {
            // NOTE: we just default to 0 for optional args at the moment
            int next;
            if (n > lua_gettop(L))
                next = 0;
            else
                next = (int)luaL_checkinteger(L, n);
            LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d %d (%zu done, %zu remaining)\n",
                    n, next, sizeof...(Prev), sizeof...(Rest));
            n++;
            return LuaMethodHelper<T, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
        } else if constexpr (std::is_same<Next, unsigned>::value) {
            unsigned next;
            if (n > lua_gettop(L))
                next = 0;
            else if (lua_isinteger(L, n))
                next = (unsigned)lua_tointeger(L, n);
            else
                next = (unsigned)luaL_checknumber(L, n);
            LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d %u (%zu done, %zu remaining)\n",
                    n, next, sizeof...(Prev), sizeof...(Rest));
            n++;
            return LuaMethodHelper<T, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
        } else if constexpr (std::is_same<Next, int64_t>::value) {
            int64_t next;
            if (lua_isinteger(L, n))
                next = (int64_t)lua_tointeger(L, n);
            else
                next = (int64_t)luaL_checknumber(L, n);
            LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d %lld (%zu done, %zu remaining)\n",
                    n, (long long)next, sizeof...(Prev), sizeof...(Rest));
            n++;
            return LuaMethodHelper<T, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
        } else if constexpr (std::is_same<Next, const char*>::value) {
            const char* next = luaL_checkstring(L, n);
            LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d \"%s\" (%zu done, %zu remaining)\n",
                    n, next, sizeof...(Prev), sizeof...(Rest));
            n++;
            return LuaMethodHelper<T, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
        } else if constexpr (std::is_same<Next, LuaRef>::value) {
            lua_pushvalue(L, n); // make copy on top of stack
            LuaRef next;
            next.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
            LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d ref%d (%zu done, %zu remaining)\n",
                    n, next.ref, sizeof...(Prev), sizeof...(Rest));
            n++;
            return LuaMethodHelper<T, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
        } else if constexpr (std::is_same<Next, LuaVariant>::value) {
            LuaVariant next;
            next.Lua_Get(L,n);
            LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d variant (%zu done, %zu remaining)\n",
                    n, sizeof...(Prev), sizeof...(Rest));
            n++;
            return LuaMethodHelper<T, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
#ifndef NO_LUAMETHOD_JSON
        } else if constexpr (std::is_same<Next, json>::value) {
            json next = lua_to_json(L, n);
            LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d json %s (%zu done, %zu remaining)\n",
                    n, next.dump().c_str(), sizeof...(Prev), sizeof...(Rest));
            n++;
            return LuaMethodHelper<T, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
#endif
        } else {
            static_unsupported_type();
            return 0;
        }
    }

    template <typename... Rest>
    static int run(lua_State *L, T *o, int n, Prev... prev)
    {
        if constexpr (sizeof...(Rest) > 0) {
            // more args to be retrieved
            return LuaMethodHelper<T,F,Prev...>::template get<Rest...>(L,o,n, prev...);
        } else if constexpr (std::is_same<decltype((o->*F)(prev...)), void>::value) {
            // result is void
            (o->*F)(prev...);
            LUAMETHOD_DEBUG_printf("LuaMethod return void\n");
            return 0;
        } else if constexpr (std::is_same<decltype((o->*F)(prev...)), int>::value) {
            // result is int64
            int res = (o->*F)(prev...);
            Lua(L).Push(res);
            LUAMETHOD_DEBUG_printf("LuaMethod pushed int: %d\n", res);
            return 1;
        } else if constexpr (std::is_same<decltype((o->*F)(prev...)), int64_t>::value) {
            // result is int64
            int64_t res = (o->*F)(prev...);
            Lua(L).Push(res);
            LUAMETHOD_DEBUG_printf("LuaMethod pushed int64: %lld\n", (long long)res);
            return 1;
        // TODO: translate c++ exceptions into lua_errors
        // TODO: unsigned result
        // TODO: test if some other return types have conflicts
        // TODO: maybe allow tuples for more than 1 result value?
#ifndef NO_LUAMETHOD_JSON
        } else if constexpr (std::is_same<decltype((o->*F)(prev...)), json>::value) {
            // result is json
            auto res = (o->*F)(prev...);
            json_to_lua(L, res);
            LUAMETHOD_DEBUG_printf("LuaMethod pushed json: %s\n", res.dump().c_str());
            return 1;
#endif
        } else {
            // result is other non-void
            auto res = (o->*F)(prev...);
            lua_settop(L, 0); // FIXME: is this the way to clean the stack?
            Lua(L).Push(res);
            LUAMETHOD_DEBUG_printf("LuaMethod pushed result\n");
            return 1;
        }
    }
};

// struct to provide static code that pulls T object and Args arguments from
// lua stack and then calls method F on that
// NOTE: in the binary, this will just generate one simple, unnamed function
//       per use of this template

template <class T, auto F, typename... Args>
struct LuaMethod {
    template <typename First, typename... Rest>
    constexpr static bool argsIsVoid() {
        return std::is_same<First,void>::value;
    }

    constexpr static bool hasArgs() {
        if constexpr (sizeof...(Args) < 1)
            return false;
        return !argsIsVoid<Args...>();
    }

    static int Func(lua_State *L) {
        T* o = T::luaL_checkthis(L, 1);
        if (!o)
            return 0;
        if constexpr (!hasArgs())
            return LuaMethodHelper<T,F>::template run<>(L, o, 2);
        else
            return LuaMethodHelper<T,F>::template run<Args...>(L, o, 2);
    }

    constexpr static size_t ArgCount = !hasArgs() ? 0 : sizeof...(Args);
};

#endif // _LUAGLUE_LUAMETHOD_H
