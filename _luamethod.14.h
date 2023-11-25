#pragma once

#ifndef _LUAGLUE_LUAMETHOD_H
#error "Please include luamethod.h instead"
#endif

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
#include "_return_type.h"

#ifdef DEBUG_LUA_METHOD
#   define LUAMETHOD_DEBUG_printf printf
#else
#   define LUAMETHOD_DEBUG_printf(...)
#endif

#ifndef STRINGIFY
#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) #s
#endif

// LUA_METHOD: helper to generate a map name => func pointer
// (Tracker, AddItems, const char*) => {"AddItems", LuaMethod<Tracker, &Tracker::AddItems,const char*>::Func}
// TODO: (argument count to) allow overloads?
#define LUA_METHOD(CLASS, METHOD, ...) { STRINGIFY(METHOD), LuaMethod<CLASS,decltype(&CLASS::METHOD),&CLASS::METHOD,__VA_ARGS__>::Func }

// recursive helper. Types of already fetched args in class template,
//   Types of missing args in function template
template <class T, class FT, FT F, typename... Prev>
struct LuaMethodHelper
{
    template <typename Next, typename... Rest>
    static typename std::enable_if<std::is_same<Next, int>::value, int>::type
    get(lua_State *L, T *o, int n, Prev... prev)
    {
        // NOTE: we just default to 0 for optional args at the moment
        int next;
        if (n > lua_gettop(L))
            next = 0;
        else
            next = (int)luaL_checkinteger(L, n);
        LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d %d (%zu done, %zu remaining)\n",
                n, next, sizeof...(Prev), sizeof...(Rest));
        n++;
        return LuaMethodHelper<T, FT, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
    }

    template <typename Next, typename... Rest>
    static typename std::enable_if<std::is_same<Next, unsigned>::value, int>::type
    get(lua_State *L, T *o, int n, Prev... prev)
    {
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
        return LuaMethodHelper<T, FT, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
    }

    template <typename Next, typename... Rest>
    static typename std::enable_if<std::is_same<Next, int64_t>::value, int>::type
    get(lua_State *L, T *o, int n, Prev... prev)
    {
        int64_t next;
        if (lua_isinteger(L, n))
            next = (int64_t)lua_tointeger(L, n);
        else
            next = (int64_t)luaL_checknumber(L, n);
        LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d %lld (%zu done, %zu remaining)\n",
                n, (long long)next, sizeof...(Prev), sizeof...(Rest));
        n++;
        return LuaMethodHelper<T, FT, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
    }

    template <typename Next, typename... Rest>
    static typename std::enable_if<std::is_same<Next, const char*>::value, int>::type
    get(lua_State *L, T *o, int n, Prev... prev)
    {
        const char* next = luaL_checkstring(L, n);
        LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d \"%s\" (%zu done, %zu remaining)\n",
                n, next, sizeof...(Prev), sizeof...(Rest));
        n++;
        return LuaMethodHelper<T, FT, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
    }

    template <typename Next, typename... Rest>
    static typename std::enable_if<std::is_same<Next, LuaRef>::value, int>::type
    get(lua_State *L, T *o, int n, Prev... prev)
    {
            lua_pushvalue(L, n); // make copy on top of stack
            LuaRef next;
            next.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
            LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d ref%d (%zu done, %zu remaining)\n",
                    n, next.ref, sizeof...(Prev), sizeof...(Rest));
            n++;
            return LuaMethodHelper<T, FT, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
    }

    template <typename Next, typename... Rest>
    static typename std::enable_if<std::is_same<Next, LuaVariant>::value, int>::type
    get(lua_State *L, T *o, int n, Prev... prev)
    {
            LuaVariant next;
            next.Lua_Get(L,n);
            LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d variant (%zu done, %zu remaining)\n",
                    n, sizeof...(Prev), sizeof...(Rest));
            n++;
            return LuaMethodHelper<T, FT, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
    }

#ifndef NO_LUAMETHOD_JSON
    template <typename Next, typename... Rest>
    static typename std::enable_if<std::is_same<Next, json>::value, int>::type
    get(lua_State *L, T *o, int n, Prev... prev)
    {
        json next = lua_to_json(L, n);
        LUAMETHOD_DEBUG_printf("LuaMethod fetched: #%d json %s (%zu done, %zu remaining)\n",
                n, next.dump().c_str(), sizeof...(Prev), sizeof...(Rest));
        n++;
        return LuaMethodHelper<T, FT, F, Prev..., Next>::template run<Rest...>(L, o, n, prev..., next);
    }
#endif

    template <typename... Rest>
    static typename std::enable_if<sizeof...(Rest) != 0, int>::type
    run(lua_State *L, T *o, int n, Prev... prev)
    {
        // more args to be retrieved
        return LuaMethodHelper<T, FT, F, Prev...>::template get<Rest...>(L,o,n, prev...);
    }

    template <typename... Rest>
    static typename std::enable_if<sizeof...(Rest) == 0 && std::is_same<LuaGlue::return_type_t<decltype(F)>, void>::value, int>::type
    run(lua_State *L, T *o, int n, Prev... prev)
    {
        // result is void
        (o->*F)(prev...);
        LUAMETHOD_DEBUG_printf("LuaMethod return void\n");
        return 0;
    }

#ifndef NO_LUAMETHOD_JSON
    template <typename... Rest>
    static typename std::enable_if<sizeof...(Rest) == 0 && std::is_same<LuaGlue::return_type_t<decltype(F)>, json>::value, int>::type
    run(lua_State *L, T *o, int n, Prev... prev)
    {
        // result is json
        auto res = (o->*F)(prev...);
        json_to_lua(L, res);
        LUAMETHOD_DEBUG_printf("LuaMethod pushed json: %s\n", res.dump().c_str());
        return 1;
    }
#endif

    template <typename... Rest>
    static typename std::enable_if<sizeof...(Rest) == 0 && !std::is_same<LuaGlue::return_type_t<decltype(F)>, void>::value
#ifndef NO_LUAMETHOD_JSON
            && !std::is_same<LuaGlue::return_type_t<decltype(F)>, json>::value
#endif
            , int>::type
    run(lua_State *L, T *o, int n, Prev... prev)
    {
        // result is non-void non-json
        auto res = (o->*F)(prev...);
        Lua(L).Push(res);
        LUAMETHOD_DEBUG_printf("LuaMethod pushed result\n");
        return 1;
    }
};

// struct to provide static code that pulls T object and Args arguments from
// lua stack and then calls method F on that
// NOTE: in the binary, this will just generate one simple, unnamed function
//       per use of this template

template <class T, class FT, FT F, typename... Args>
struct LuaMethod {
    template <typename First, typename... Rest>
    constexpr static bool argsIsVoid() {
        return std::is_same<First, void>::value;
    }

    template <std::size_t N = sizeof...(Args)>
    constexpr static typename std::enable_if<N == 0, bool>::type
    hasArgs() {
        return false;
    }

    template <std::size_t N = sizeof...(Args)>
    constexpr static typename std::enable_if<N != 0, bool>::type
    hasArgs() {
        return !argsIsVoid<Args...>(); // used to not try to fetch void for function<X(void)>
    }

    template <class R = int, std::size_t N = sizeof...(Args)>
    static typename std::enable_if<!hasArgs(), R>::type // alternatively we could specialize run on Args... = <void>
    Func(lua_State *L) {
        T* o = T::luaL_checkthis(L, 1);
        if (!o)
            return 0;
        return LuaMethodHelper<T, FT, F>::template run<>(L, o, 2); // no args
    }

    template <class R = int, std::size_t N = sizeof...(Args)>
    static typename std::enable_if<hasArgs(), R>::type
    Func(lua_State *L) {
        T* o = T::luaL_checkthis(L, 1);
        if (!o)
            return 0;
        return LuaMethodHelper<T, FT, F>::template run<Args...>(L, o, 2); // with args
    }

    constexpr static size_t ArgCount = !hasArgs() ? 0 : sizeof...(Args);
};
