#include <includes/includes.h>
#include <includes/tools.h>

#include "luaobject.h"

#include "luainterface.h"

int LuaObject::lua_rawSet(LuaInterface* lua)
{
    lua_getFieldsTable(lua);
    lua_insert(lua->getLuaState(), -3); // move the fields table to the top
    lua_rawset(lua->getLuaState(), -3);
    lua_pop(lua->getLuaState(), 1);
    return 0;
}

int LuaObject::lua_rawGet(LuaInterface* lua)
{
    lua_State* L = lua->getLuaState();
    if (m_fieldsTableRef != -1) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, m_fieldsTableRef);
        lua_insert(L, -2);
        lua_rawget(L, -1);
        lua_remove(L, -2);
    }
    else {
        lua_pop(L, 1);
        lua_pushnil(L);
    }
    return 1;
}

int LuaObject::lua_getMetatable(LuaInterface* lua)
{
    const std::type_info& tinfo = typeid(*this);
    auto it = g_lua.m_metatableMap.find(&tinfo);

    int metatableRef;
    if (it == g_lua.m_metatableMap.end()) {
        auto name = getCppClassName() + "_mt";
        lua_getglobal(g_lua.getLuaState(), name.c_str());
        metatableRef = luaL_ref(g_lua.getLuaState(), LUA_REGISTRYINDEX);
        g_lua.m_metatableMap[&tinfo] = metatableRef;
    }
    else
        metatableRef = it->second;

    lua_rawgeti(lua->getLuaState(), LUA_REGISTRYINDEX, metatableRef);
    return 0;
}

int LuaObject::lua_getFieldsTable(LuaInterface* lua)
{
    // create fields table on the fly
    if (m_fieldsTableRef == -1) {
        lua_newtable(g_lua.getLuaState());
        m_fieldsTableRef = luaL_ref(g_lua.getLuaState(), LUA_REGISTRYINDEX); // save a reference for it
        g_lua.m_totalLuaObjectsWihtFields++;
    }

    lua_rawgeti(lua->getLuaState(), LUA_REGISTRYINDEX, m_fieldsTableRef);
    return 1;
}

std::string LuaObject::getCppClassName()
{
#ifdef _MSC_VER
    return demangle_name(typeid(*this).name() + 6);
#else
    return demangle_name(typeid(*this).name());
#endif
}
