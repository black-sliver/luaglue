#include "../luatestbase.hpp"
#include "../macros.hpp"
#include "../../luainterface.h"
#include "../../luamethod.h"


// ReSharper disable CppMemberFunctionMayBeStatic
class LuaMethodTester : public LuaInterface<LuaMethodTester> {
    friend class LuaInterface;

    void RecursiveArgTester(json&) const
    {
    }

    json TooDeepResultTester() const
    {
        return json::parse(std::string(2000, '[') + std::string(2000, ']'));
    }

protected: // Lua interface implementation
    static constexpr char Lua_Name[] = "LuaMethodTester";
    static const MethodMap Lua_Methods;
};

const LuaInterface<LuaMethodTester>::MethodMap LuaMethodTester::Lua_Methods = {
    LUA_METHOD(LuaMethodTester, RecursiveArgTester, json),
    LUA_METHOD(LuaMethodTester, TooDeepResultTester, void),
};

class LuaMethodTest : public LuaTestBase {
protected:
    LuaMethodTester tester;

    LuaMethodTest()
    {
        LuaMethodTester::Lua_Register(L);
        tester.Lua_Push(L);
        lua_setglobal(L, "tester");
    }

    ~LuaMethodTest() override
    {
        lua_pushnil(L);
        lua_setglobal(L, "tester");
        lua_gc(L, LUA_GCCOLLECT, 0);
    }
};

TEST_F(LuaMethodTest, RecursiveArg) {
#ifndef USE_EXCEPTIONS
    ASSERT_TRUE(doString(R""""(
        x = {}
        x[1] = x
        tester:RecursiveArgTester(x)
        return true
    )""""));
#else
    EXPECT_TRUE(doString(R""""(
        x = {}
        x[1] = x
        local ok, err = pcall(function() tester:RecursiveArgTester(x) end)
        if ok then
            return false -- expected error
        elseif not err:match("Max depth reached$") then
            print("Unexpected error: " .. err)
            return false
        else
            return true -- ok
        end
    )""""));
#endif
    ASSERT_GE(lua_gettop(L), 1);
    EXPECT_TRUE(lua_toboolean(L, -1));
}

TEST_F(LuaMethodTest, TooDeepResult) {
#ifndef USE_EXCEPTIONS
    ASSERT_TRUE(doString(R""""(
        tester:TooDeepResultTester()
        return true
    )""""));
#else
    ASSERT_TRUE(doString(R""""(
        local ok, err = pcall(function() tester:TooDeepResultTester() end)
        if ok then
            return false -- expected error
        elseif not err:match("Max depth reached$") then
            print("Unexpected error: " .. err)
            return false
        else
            return true -- ok
        end
    )""""));
#endif
    ASSERT_GE(lua_gettop(L), 1);
    EXPECT_TRUE(lua_toboolean(L, -1));
}
