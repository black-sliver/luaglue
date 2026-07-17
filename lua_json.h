#ifndef LUAJSON_H
#define LUAJSON_H

#include "lua_include.h"
#include "luacompat.h"
#include <nlohmann/json.hpp>


#if defined __cpp_exceptions || defined __EXCEPTIONS || defined _CPPUNWIND
#   define HAS_EXCEPTIONS
#   define LUA_TO_JSON_MAX_DEPTH_REACHED() \
        throw std::runtime_error("Max depth reached");
#   define LUA_TO_JSON_STACK_OVERFLOW() \
        throw std::runtime_error("Stack overflow");
#   define JSON_TO_LUA_MAX_DEPTH_REACHED() \
        throw std::runtime_error("Max depth reached");
#   define JSON_TO_LUA_STACK_OVERFLOW() \
        throw std::runtime_error("Stack overflow");
#else
#   define LUA_TO_JSON_MAX_DEPTH_REACHED() { \
        fprintf(stderr, "Warning: Max depth reached in lua_to_json. Returning null.\n"); \
        return nullptr; \
    }
#   define LUA_TO_JSON_STACK_OVERFLOW() { \
        fprintf(stderr, "Warning: Lua stack overflow in lua_to_json. Returning null.\n"); \
        return nullptr; \
    }
#   define JSON_TO_LUA_MAX_DEPTH_REACHED() { \
        lua_pushnil(L); \
        fprintf(stderr, "Warning: Max depth reached in json_to_lua. Returning nil.\n"); \
        return; \
    }
#   define JSON_TO_LUA_STACK_OVERFLOW() { \
        lua_pushnil(L); \
        fprintf(stderr, "Warning: Lua stack overflow in json_to_lua. Returning nil.\n"); \
        return; \
    }
#endif


using nlohmann::json;


class LuaJson_EmptyArray {
    // Lua does not differentiate between array and object and lua_to_json will
    // create an empty object for an empty table. This can be used to create a
    // pseudo constant that will generate an empty array.

public:
    static void Lua_Register(lua_State *L) { // create "Class" in Lua
        // create metatable for this class
        luaL_newmetatable(L, Lua_Name);
        lua_pop(L,1);
    }

    void Lua_Push(lua_State *L) const { // pushes instance to Lua stack
        // create userdata=pointer for this instance
        const void **ud = (const void**) lua_newuserdata(L, sizeof(void**));
        *ud = nullptr;
        // set metatable
        luaL_setmetatable(L, Lua_Name);
    }

    static bool Lua_is(lua_State *L, int narg) {
        if (luaL_testudata(L, narg, Lua_Name))
            return true;
        return false;
    }

    // Lua interface implementation
#if !defined _MSC_VER || _MSC_VER >= 1911
    static constexpr const char Lua_Name[] = "LuaJson_EmptyArray";
#else
    static const char Lua_Name[]; // = "LuaJson_EmptyArray"; // assign this in implementation
#endif
};

static json lua_to_json(lua_State* L, const int n = -1, const int maxDepth = 1000)
{
    json j;
    auto type = lua_type(L,n);
    switch (type) {
        case LUA_TNUMBER:
            if (lua_isinteger(L,n))
                return lua_tointeger(L,n);
            return lua_tonumber(L,n);
        case LUA_TSTRING:
            return lua_tostring(L,n);
        case LUA_TBOOLEAN:
            return (bool)lua_toboolean(L,n);
        case LUA_TTABLE:
        {
            if (maxDepth == 1)
                LUA_TO_JSON_MAX_DEPTH_REACHED();
            if (!lua_checkstack(L, 2))
                LUA_TO_JSON_STACK_OVERFLOW();
            lua_pushnil(L); // first key
            while (lua_next(L, (n<0) ? (n-1) : n)) {
                // key now at -2, value at -1
                if (lua_isinteger(L,-2)) {
                    if (j.is_null()) j = json::array();
                    if (j.is_object()) {
                        // NOTE: key will be a string when converting mixed back; this has to be handled in lua
                        int ikey = lua_tointeger(L,-2);
                        std::string key = std::to_string(ikey);
#ifdef HAS_EXCEPTIONS
                        try {
                            j[key] = lua_to_json(L);
                        } catch (...) {
                            lua_pop(L, 2);
                            throw;
                        }
#else
                        j[key] = lua_to_json(L);
#endif
                    } else {
                        int key = lua_tointeger(L,-2) - 1; // nlohmann::json arrays are zero-based
                        if (key>=0) {
#ifdef HAS_EXCEPTIONS
                            try {
                                j[key] = lua_to_json(L, -1, maxDepth - 1);
                            } catch (...) {
                                lua_pop(L, 2);
                                throw;
                            }
#else
                            j[key] = lua_to_json(L, -1, maxDepth - 1);
#endif
                        } else {
                            fprintf(stderr, "Warning: Invalid array index: %d\n", key);
                        }
                    }
                } else if (lua_isstring(L,-2)) {
                    if (j.is_null()) j = json::object();
                    if (j.is_array()) {
                        // convert array to object for mixed table
                        json arr = j;
                        j = json::object();
                        int i = 1;
                        for (auto it: arr) {
                            std::string key = std::to_string(i);
                            j[key] = it;
                            i++;
                        }
                    }
                    const char* key = lua_tostring(L,-2);
#ifdef HAS_EXCEPTIONS
                    try {
                        j[key] = lua_to_json(L, -1, maxDepth - 1);
                    } catch (...) {
                        lua_pop(L, 2);
                        throw;
                    }
#else
                    j[key] = lua_to_json(L, -1, maxDepth - 1);
#endif
                } else {
                    fprintf(stderr, "Warning: unhandled table key type %d\n", lua_type(L,-2));
                }
                // pop value, keep key (used in lua_next())
                lua_pop(L,1);
            }
            if (j.is_null()) j = json::object(); // return empty object for empty table
            break;
        }
        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA:
            if (LuaJson_EmptyArray::Lua_is(L, n)) {
                j = json::array(); // empty array
                break;
            }
            // fall through
        case LUA_TNIL:
        case LUA_TNONE:
        default:
            break; // return NULL
    }
    return j;
}

static void json_to_lua(lua_State* L, const json& j, const int maxDepth = 1000)
{
    if (!lua_checkstack(L, 1))
        JSON_TO_LUA_STACK_OVERFLOW();
    switch (j.type()) {
        case json::value_t::number_unsigned:
            if ((sizeof(lua_Integer) >= 8 && j.get<uint64_t>() <= INT64_MAX) || j.get<uint64_t>() <= INT32_MAX) {
                lua_pushinteger(L, j);
            } else {
                lua_pushnumber(L, j);
            }
            break;
        case json::value_t::number_integer:
            if (sizeof(lua_Integer) >= 8 || (j.get<int64_t>() <= INT32_MAX && j.get<int64_t>() >= INT32_MIN)) {
                lua_pushinteger(L, j);
            } else {
                lua_pushnumber(L, j);
            }
            break;
        case json::value_t::number_float:
            lua_pushnumber(L, j);
            break;
        case json::value_t::boolean:
            lua_pushboolean(L, j);
            break;
        case json::value_t::string:
            lua_pushstring(L, j.get<std::string>().c_str());
            break;
        case json::value_t::null:
            lua_pushnil(L);
            break;
        case json::value_t::object:
            if (maxDepth == 1)
                JSON_TO_LUA_MAX_DEPTH_REACHED();
            if (!lua_checkstack(L, 2)) {
                lua_pushnil(L);
                JSON_TO_LUA_STACK_OVERFLOW();
            }
            lua_newtable(L);
            for (auto it=j.begin(); it!=j.end(); ++it) {
#ifdef HAS_EXCEPTIONS
                try {
                    json_to_lua(L, it.value(), maxDepth - 1);
                } catch (...) {
                    lua_pop(L, 1);
                    throw;
                }
#else
                json_to_lua(L, it.value(), maxDepth - 1);
#endif
                lua_setfield(L, -2, it.key().c_str());
            }
            break;
        case json::value_t::array:
            if (maxDepth == 1)
                JSON_TO_LUA_MAX_DEPTH_REACHED();
            if (!lua_checkstack(L, 3))
                JSON_TO_LUA_STACK_OVERFLOW();
            lua_newtable(L);
            for (size_t i=0; i<j.size(); i++) {
                if (sizeof(i) < sizeof(lua_Integer)
                        || i < static_cast<size_t>(std::numeric_limits<lua_Integer>::max()))
                    lua_pushinteger(L, static_cast<lua_Integer>(i) + 1);
                else
                    lua_pushnumber(L, static_cast<lua_Number>(i) + 1);
#ifdef HAS_EXCEPTIONS
                try {
                    json_to_lua(L, j[i], maxDepth - 1);
                } catch (...) {
                    lua_pop(L, 2);
                    throw;
                }
#else
                json_to_lua(L, j[i], maxDepth - 1);
#endif
                lua_settable(L, -3);
            }
            break;
        default:
            // not implemented
            fprintf(stderr, "Warning: unhandled type %d %s in json_to_lua()\n",
                    (int)j.type(), j.type_name());
            lua_pushnil(L);
            break;
    }
}


#undef HAS_EXCEPTIONS
#undef LUA_TO_JSON_MAX_DEPTH_REACHED
#undef LUA_TO_JSON_STACK_OVERFLOW
#undef JSON_TO_LUA_STACK_OVERFLOW

#endif /* LUAJSON_H */

