#ifndef __LUAINTERFACE__
#define __LUAINTERFACE__

#include <lua.hpp>

class LuaInterface;

typedef std::function<int(LuaInterface*)> LuaCppFunction;
typedef std::unique_ptr<LuaCppFunction> LuaCppFunctionPtr;

class LuaInterface
{
public:
	LuaInterface();

	bool init();
	void close();

	bool loadFile(const std::string& path);

	static bool reserveScript() {
		return ++scriptIndex < 16;
	}

	static void resetScript() {
		assert(scriptIndex >= 0);
		scriptIndex--;
	}

	int protectedCall(int nargs, int nresutls);
	void reportError(const char* function, const std::string& error_desc, bool stack_trace = false);

	void getOrCreateTable(const std::string& name);

	void pushObject(const LuaObjectPtr& obj);

	void registerClass(const std::string& className, const std::string& baseClass);

	template<typename R = void, typename... T>
	R callGlobalField(const std::string& global, const std::string& field, const T&... args);

	static void pushCppFunction(lua_State* L, const LuaCppFunction& func, const std::string& name);

	lua_State* getLuaState() const {
		return m_luaState;
	}

protected:
	void registerFunctions();
	void registerGlobalClass(const std::string& className);

	//functions
	static int luaErrorHandler(lua_State* L);
	static int luaCppFunctionCallback(lua_State* L);
	static int luaCollectCppFunction(lua_State* L);

	/// Metamethod that will retrieve fields values (that include functions) from the object when using '.' or ':'
	static int luaObjectGetEvent(LuaInterface* lua);
	/// Metamethod that is called when setting a field of the object by using the keyword '='
	static int luaObjectSetEvent(LuaInterface* lua);
	/// Metamethod that will check equality of objects by using the keyword '=='
	static int luaObjectEqualEvent(LuaInterface* lua);
	/// Metamethod that is called every two lua garbage collections
	/// for any LuaObject that have no references left in lua environment
	/// anymore, thus this creates the possibility of holding an object
	/// existence by lua until it got no references left
	static int luaObjectCollectEvent(lua_State* L);

	lua_State* m_luaState = nullptr;
	int m_totalFuncRefs = 0;
	int m_totalObjRefs = 0;
	int m_totalLuaObjectsWihtFields = 0;

	std::unordered_map<const std::type_info*, int> m_metatableMap;

	friend class LuaObject;

private:
	static int scriptIndex;

	std::string getStackTrace(const std::string& error_desc, int level);

	friend class LuaException;
};

extern LuaInterface g_lua;

#include <includes/tools.h>
#include "luaexception.h"
#include "luavaluecasts.h"

template<typename R, typename ...T>
R LuaInterface::callGlobalField(const std::string& global, const std::string& field, const T& ...args) {
	const std::string& name = global + '.' + field;

	lua_getglobal(m_luaState, global.c_str());
	if (!lua_istable(m_luaState, -1)) {
		reportError(name.c_str(), "attempt to index a " + typename_lua(m_luaState) + " value (global 'g_modules')");
		lua_pop(m_luaState, 1);
		return R();
	}

	lua_getfield(m_luaState, -1, field.c_str());
	lua_remove(m_luaState, -2);
	if (!lua_isfunction(m_luaState, -1)) {
		reportError(name.c_str(), "attempt to call a " + typename_lua(m_luaState) + " value (global 'g_modules.onLoad')");
		lua_pop(m_luaState, 1);
		return R();
	}

	int numArgs = polymorphic_push(m_luaState, args...);

	int previous_stack = lua_gettop(m_luaState);

	int ret = protectedCall(numArgs, LUA_MULTRET);
	if (ret != 0) {
		reportError(name.c_str(), polymorphic_pop<std::string>(m_luaState));
		return R();
	}

	int rets = (lua_gettop(m_luaState) + numArgs + 1) - previous_stack;
	while (rets != LUA_MULTRET) {
		if (rets < LUA_MULTRET) {
			lua_pushnil(m_luaState);
			rets++;
		}
		else {
			lua_pop(m_luaState, 1);
			rets--;
		}
	}

	if (rets > 0)
		return polymorphic_pop<R>(m_luaState);

	return R();
}

#endif
