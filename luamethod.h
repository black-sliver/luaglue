#ifndef _LUAGLUE_LUAMETHOD_H
#define _LUAGLUE_LUAMETHOD_H

#if __cplusplus < 201700L && !defined LUA_METHOD_LONG_FORM
#define LUA_METHOD_LONG_FORM
#endif

#ifdef LUA_METHOD_LONG_FORM
#include "_luamethod.14.h" // c++14 implementation requires decltype template argument
#else
#include "_luamethod.17.h" // c++17 implementation uses auto template argument
#endif

#endif
