#ifndef __LUAVALUECASTS__
#define __LUAVALUECASTS__

template<typename T, typename... Args>
int polymorphic_push(lua_State* L, const T& v, const Args&... args);

class luafunction_holder {
public:
	luafunction_holder() = delete;
	luafunction_holder(const luafunction_holder&) = delete;
	luafunction_holder& operator=(const luafunction_holder&) = delete;

	luafunction_holder(int ref) : funcRef(ref) { }
	~luafunction_holder() { luaL_unref(g_lua.getLuaState(), LUA_REGISTRYINDEX, funcRef); }
	void push() const { lua_rawgeti(g_lua.getLuaState(), LUA_REGISTRYINDEX, funcRef); }
private:
	int funcRef;
};

bool pull_lua_value(lua_State* L, int& ref, int index = -1)
{
	ref = static_cast<int>(lua_tointeger(L, index));
	if (ref == 0 && !lua_isnumber(L, index) && !lua_isnil(L, index))
		return false;
	return true;
}

bool pull_lua_value(lua_State* L, uint32_t& ref, int index = -1)
{
	ref = static_cast<uint32_t>(lua_tonumber(L, index));
	if (ref == 0 && !lua_isnumber(L, index) && !lua_isnil(L, index))
		return false;
	return true;
}

bool pull_lua_value(lua_State* L, std::string& ref, int index = -1)
{
	ref = LuaInterface::getString(L, index);
	return true;
}

bool pull_lua_value(lua_State* L, bool& ref, int index = -1)
{
	ref = lua_toboolean(L, index) != 0;
	return true;
}

template<class T>
bool pull_lua_value(lua_State* L, std::shared_ptr<T>& t, int index = -1)
{
	auto userdata = static_cast<std::shared_ptr<T>*>(lua_touserdata(L, index));
	if (userdata && *userdata) {
		t = *userdata;
		return true;
	}
	return false;
}

template<typename... Args>
bool pull_lua_value(lua_State* L, std::function<void(Args...)>& func, int index = -1)
{
	if (!lua_isfunction(L, index)) {
		std::cout << "The paramater need be a function" << std::endl;
		return false;
	}

	lua_pushvalue(L, -1);
	int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);
	std::shared_ptr<luafunction_holder> funcHolder(new luafunction_holder(funcRef));
	func = [funcHolder](Args... args) {
		try {
			lua_State* L = g_lua.getLuaState();
			funcHolder->push();
			if (lua_isfunction(L, -1)) {
				int numArgs = polymorphic_push(L, args...);
				int rets = LuaInterface::protectedCall(L, numArgs, 0);
				lua_pop(L, numArgs);
			}
			else {
				throw std::invalid_argument("Attempt to call a expired function");
			}
		}
		catch (const std::exception&) {
			std::cout << "lua function callback failed" << std::endl;
		}
	};
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

template<typename T>
int push_lua_value(lua_State* L, const std::vector<T>& vector);

template<typename T>
T lua_pull_value(lua_State* L, int index = -1)
{
	T o;
	if (!pull_lua_value(L, o, index))
		throw std::invalid_argument("luabinder::lua_pull_value invalid value in stack " + index);
	return o;
}

template<typename T>
T polymorphic_pop(lua_State* L)
{
	T v = lua_pull_value<T>(L);
	lua_pop(L, 1);
	return v;
}

int polymorphic_push(lua_State*) { return 0; }

template<typename T, typename... Args>
int polymorphic_push(lua_State* L, const T& v, const Args&... args)
{
	int r = push_lua_value(L, v);
	return r + polymorphic_push(L, args...);
}

template<int N>
struct push_tuple_internal_luavalue {
	template<typename Tuple>
	static void call(lua_State* L, const Tuple& tuple) {
		push_internal_luavalue(L, std::get<N - 1>(tuple));
		lua_rawseti(L, -2, N);
		push_tuple_internal_luavalue<N - 1>::call(L, tuple);
	}
};

template<>
struct push_tuple_internal_luavalue<0> {
	template<typename Tuple>
	static void call(lua_State* L, const Tuple& tuple) { }
};

template<typename... Args>
int push_internal_luavalue(lua_State* L, const std::tuple<Args...>& tuple) {
	lua_newtable(L);
	push_tuple_internal_luavalue<sizeof...(Args)>::call(L, tuple);
	return 1;
}

template<typename T>
int push_internal_luavalue(lua_State* L, T v) {
	return push_lua_value(L, v);
}

template<typename T>
int push_lua_value(lua_State* L, const std::vector<T>& vector) {
	lua_createtable(L, static_cast<int>(vector.size()), 0);

	int i = 0;
	for (const T& v : vector) {
		push_internal_luavalue(L, v);
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

#endif
