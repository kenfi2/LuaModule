#include <includes/includes.h>
#include <includes/tools.h>

#include "luainterface.h"

#include <thread/thread.h>

// include after include luainterface
#include "luavaluecasts.h"
#include "luabinder.h"

LuaInterface g_lua;

int LuaInterface::scriptIndex = 0;

LuaInterface::LuaInterface()
{
	init();
}

bool LuaInterface::init()
{
	m_luaState = luaL_newstate();
	if (!m_luaState)
		return false;

	luaL_openlibs(m_luaState);

	// register functions
	registerGlobalClass("g_dispatcher");
	luabinder::bindMethodFunction("g_dispatcher", "addTask", &Dispatcher::addTask, &g_dispatcher);
	luabinder::bindMethodFunction("g_dispatcher", "shutdown", &Dispatcher::shutdown, &g_dispatcher);

	registerGlobalClass("g_scheduler");
	luabinder::bindMethodFunction("g_scheduler", "addEvent", &Scheduler::addEvent, &g_scheduler);
	luabinder::bindMethodFunction("g_scheduler", "shutdown", &Scheduler::shutdown, &g_scheduler);

	luabinder::bindFunction("getFileTime", [](std::string file) { return getFileTime(file); });
	luabinder::bindFunction("getFiles", [](std::string path, std::string extension) { return getFiles(path, extension); });

	return true;
}

void LuaInterface::close()
{
	if (!m_luaState)
		return;

	lua_close(m_luaState);
	m_luaState = nullptr;
}

bool LuaInterface::loadFile(const std::string& path)
{
	int ret = luaL_loadfile(m_luaState, path.c_str());
	if (ret != 0)
		return false;

	if (!lua_isfunction(m_luaState, -1))
		return false;

	if (!reserveScript()) {
		return false;
	}

	ret = protectedCall(m_luaState, 0, 0);
	if (ret != 0) {
		std::cout << getString(m_luaState, -1) << std::endl;
		resetScript();
		return false;
	}

	resetScript();
	return true;
}

int LuaInterface::protectedCall(lua_State* L, int nargs, int nresults)
{
	int error_index = lua_gettop(L) - nargs;
	lua_pushcfunction(L, luaErrorHandler);
	lua_insert(L, error_index);

	int ret = lua_pcall(L, nargs, nresults, error_index);
	lua_remove(L, error_index);
	return ret;
}

int LuaInterface::luaErrorHandler(lua_State* L)
{
	const std::string& errorMessage = getString(L, -1);
	lua_pushlstring(L, errorMessage.c_str(), errorMessage.length());
	return 1;
}

std::string LuaInterface::getString(lua_State* L, int index)
{
	size_t len;
	const char* c_str = lua_tolstring(L, index, &len);
	if (!c_str || len == 0) {
		return std::string();
	}
	return std::string(c_str, len);
}

void LuaInterface::getOrCreateTable(const std::string& name)
{
	lua_getglobal(m_luaState, name.c_str());
	if (!lua_istable(m_luaState, -1)) {
		lua_pop(m_luaState, 1);
		lua_newtable(m_luaState);
		lua_pushvalue(m_luaState, -1);
		lua_setglobal(m_luaState, name.c_str());
	}
}

void LuaInterface::pushObject(const LuaObjectPtr& obj)
{
	new(lua_newuserdata(m_luaState, sizeof(LuaObjectPtr))) LuaObjectPtr(obj);
	m_totalObjRefs++;

	obj->lua_getMetatable(this);
	if (lua_isnil(m_luaState, -1)) {
		std::cout << "metatable for class " << obj->getCppClassName() << " not found, did you bind the C++ class?" << std::endl;
		lua_pop(m_luaState, 1);
		return;
	}

	lua_setmetatable(m_luaState, -2);
}

void LuaInterface::registerClass(const std::string& className, const std::string& baseClass)
{
	getOrCreateTable(className);
	const int clazz = lua_gettop(m_luaState);

	getOrCreateTable(className + "_mt");
	int clazz_mt = lua_gettop(m_luaState);

	// set metatable metamethods
	pushCppFunction(m_luaState, &LuaInterface::luaObjectGetEvent, "LuaObject:index");
	lua_setfield(m_luaState, clazz_mt, "__index");
	pushCppFunction(m_luaState, &LuaInterface::luaObjectSetEvent, "LuaObject:newindex");
	lua_setfield(m_luaState, clazz_mt, "__newindex");
	pushCppFunction(m_luaState, &LuaInterface::luaObjectEqualEvent, "LuaObject:eq");
	lua_setfield(m_luaState, clazz_mt, "__eq");
	lua_pushcclosure(m_luaState, LuaInterface::luaObjectCollectEvent, 0); // use C function for gc event, thus will avoid profiling GC calls
	lua_setfield(m_luaState, clazz_mt, "__gc");

	lua_pushvalue(m_luaState, clazz);
	lua_rawseti(m_luaState, clazz_mt, 1);

	if (!className.empty() && className != "LuaObject") {
		lua_pushvalue(m_luaState, clazz);
		lua_newtable(m_luaState);
		getOrCreateTable(baseClass);
		lua_setfield(m_luaState, -2, "__index");
		lua_setmetatable(m_luaState, -2);
		lua_pop(m_luaState, 1);
	}

	lua_pop(m_luaState, 2);
}

// Function
int LuaInterface::luaCppFunctionCallback(lua_State* L)
{
	LuaCppFunctionPtr* funcPtr = static_cast<LuaCppFunctionPtr*>(lua_touserdata(L, lua_upvalueindex(1)));
	assert(funcPtr);

	int numRets = 0;
	try {
		numRets = (*(funcPtr->get()))(&g_lua);
		assert(numRets == lua_gettop(L));
	}
	catch (const std::invalid_argument&) {
		luaL_error(L, "Invalid argument");
	}
	return numRets;
}

int LuaInterface::luaCollectCppFunction(lua_State* L)
{
	LuaCppFunctionPtr** funcPtr = static_cast<LuaCppFunctionPtr**>(lua_touserdata(L, -1));
	if (!funcPtr)
		return 0;

	assert(*funcPtr);
	(*funcPtr)->reset();
	g_lua.m_totalFuncRefs--;
	return 0;
}

void LuaInterface::pushCppFunction(lua_State* L, const LuaCppFunction& func, const std::string& name)
{
	std::string _name = name;
	if (_name.empty())
		_name = "??";

	// add function pointer to stack
	new(lua_newuserdata(L, sizeof(LuaCppFunctionPtr))) LuaCppFunctionPtr(new LuaCppFunction(func));
	g_lua.m_totalFuncRefs++;

	// setmetatable({}, { __gc = collect, __name = name })
	lua_newtable(L);
	lua_pushcclosure(L, &LuaInterface::luaCollectCppFunction, 0);
	lua_setfield(L, -2, "__gc");
	lua_pushlstring(L, _name.c_str(), _name.length());
	lua_setfield(L, -2, "__name");

	lua_setmetatable(L, -2);

	lua_pushcclosure(L, &LuaInterface::luaCppFunctionCallback, 1);
}

void LuaInterface::registerGlobalClass(const std::string& className)
{
	lua_getglobal(m_luaState, className.c_str());
	if (!lua_istable(m_luaState, -1)) {
		lua_pop(m_luaState, 1);
		lua_newtable(m_luaState);
		lua_pushvalue(m_luaState, -1);
		lua_setglobal(m_luaState, className.c_str());
	}
	lua_pop(m_luaState, 1);
}

int LuaInterface::luaObjectGetEvent(LuaInterface* lua)
{
	lua_State* L = lua->getLuaState();

	// stack: obj, key
	LuaObjectPtr* objPtr = static_cast<LuaObjectPtr*>(lua_touserdata(L, -2));
	assert(objPtr);
	std::string key = getString(L, -1);
	LuaObjectPtr obj = *objPtr;
	assert(obj);

	// if the field for this key exists, returns it
	obj->lua_rawGet(lua);
	if (!lua_isnil(L, -1)) {
		lua_remove(L, -2); // removes the obj
		// field value is on the stack
		return 1;
	}

	lua_pop(L, 1); // pops the nil field

	// pushes the method assigned by this key
	lua_getmetatable(L, -1); // pushes obj metatable
	lua_rawgeti(L, -1, 1); // push obj methods table
	lua_getfield(L, -1, key.c_str()); // pushes obj method
	lua_insert(L, -4); // move value to bottom
	lua_pop(L, 3); // pop obj methods, obj metatable, obj

	// the result value is on the stack
	return 1;
}

int LuaInterface::luaObjectSetEvent(LuaInterface* lua)
{
	lua_State* L = lua->getLuaState();

	// stack: obj, key, value
	LuaObjectPtr* objPtr = static_cast<LuaObjectPtr*>(lua_touserdata(L, -3));
	assert(objPtr);
	LuaObjectPtr obj = *objPtr;
	assert(obj);

	lua_remove(L, -3);

	obj->lua_rawSet(lua);
	return 0;
}

int LuaInterface::luaObjectEqualEvent(LuaInterface* lua)
{
	lua_State* L = lua->getLuaState();

	// stack: obj1, obj2
	bool ret = false;

	// check if obj1 == obj2
	if (lua_isuserdata(L, -1) && lua_isuserdata(L, -2)) {
		LuaObjectPtr* objPtr2 = static_cast<LuaObjectPtr*>(lua_touserdata(L, -1));
		LuaObjectPtr* objPtr1 = static_cast<LuaObjectPtr*>(lua_touserdata(L, -2));
		assert(objPtr1 && objPtr2);
		if (*objPtr1 == *objPtr2)
			ret = true;
	}

	lua_pop(L, 2);

	lua_pushboolean(L, ret != 0);
	return 1;
}

int LuaInterface::luaObjectCollectEvent(lua_State* L)
{
	// gets object pointer
	auto objPtr = static_cast<LuaObjectPtr*>(lua_touserdata(L, -1));
	assert(objPtr);

	g_lua.m_totalObjRefs--;

	// resets pointer to decrease object use count
	objPtr->reset();
	return 0;
}
