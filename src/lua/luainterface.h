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

	static int protectedCall(lua_State* L, int nargs, int nresutls);
	static int luaErrorHandler(lua_State* L);

	static std::string getString(lua_State* L, int index);

	void getOrCreateTable(const std::string& name);

	void pushObject(const LuaObjectPtr& obj);

	void registerClass(const std::string& className, const std::string& baseClass);

	// Function
	static int luaCppFunctionCallback(lua_State* L);
	static int luaCollectCppFunction(lua_State* L);
	static void pushCppFunction(lua_State* L, const LuaCppFunction& func, const std::string& name);

	lua_State* getLuaState() const {
		return m_luaState;
	}

protected:
	void registerGlobalClass(const std::string& className);

	lua_State* m_luaState = nullptr;
	int m_totalFuncRefs = 0;
	int m_totalObjRefs = 0;
	int m_totalLuaObjectsWihtFields = 0;

	std::unordered_map<const std::type_info*, int> m_metatableMap;

	friend class LuaObject;

private:
	static int scriptIndex;

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
};

extern LuaInterface g_lua;

#endif

