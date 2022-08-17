#include <includes/includes.h>

#include "luainterface.h"

#include "luavaluecasts.h"

std::string typename_lua(lua_State* L, int index)
{
	return std::string(lua_typename(L, lua_type(L, index)));
}

bool pull_lua_value(lua_State* L, int& ref, int index)
{
	ref = static_cast<int>(lua_tointeger(L, index));
	if (ref == 0 && !lua_isnumber(L, index) && !lua_isnil(L, index))
		return false;
	return true;
}

bool pull_lua_value(lua_State* L, uint32_t& ref, int index)
{
	ref = static_cast<uint32_t>(lua_tonumber(L, index));
	if (ref == 0 && !lua_isnumber(L, index) && !lua_isnil(L, index))
		return false;
	return true;
}

bool pull_lua_value(lua_State* L, std::string& ref, int index)
{
	size_t len;
	const char* c_str = lua_tolstring(L, index, &len);
	if (c_str && len > 0) {
		ref = std::string(c_str, len);
		return true;
	}
	return false;
}

bool pull_lua_value(lua_State* L, bool& ref, int index)
{
	ref = lua_toboolean(L, index) != 0;
	return true;
}

int push_lua_value(lua_State* L, int value)
{
	lua_pushinteger(L, value);
	return 1;
}

int push_lua_value(lua_State* L, double value)
{
	lua_pushnumber(L, value);
	return 1;
}

int push_lua_value(lua_State* L, long value) { return push_lua_value(L, (double)value); }
int push_lua_value(lua_State* L, unsigned long value) { return push_lua_value(L, (double)value); }

int push_lua_value(lua_State* L, int8_t value) { return push_lua_value(L, (int)value); }
int push_lua_value(lua_State* L, uint8_t value) { return push_lua_value(L, (int)value); }
int push_lua_value(lua_State* L, int16_t value) { return push_lua_value(L, (int)value); }
int push_lua_value(lua_State* L, uint16_t value) { return push_lua_value(L, (int)value); }
int push_lua_value(lua_State* L, uint32_t value) { return push_lua_value(L, (double)value); }
int push_lua_value(lua_State* L, int64_t value) { return push_lua_value(L, (double)value); }
int push_lua_value(lua_State* L, uint64_t value) { return push_lua_value(L, (double)value); }

int push_lua_value(lua_State* L, const std::string& value)
{
	lua_pushlstring(L, value.c_str(), value.length());
	return 1;
}

int push_lua_value(lua_State* L, const LuaObjectPtr& t)
{
	if (t) {
		g_lua.pushObject(t);
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}

int polymorphic_push(lua_State*)
{
	return 0;
}
