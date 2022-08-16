#ifndef __LUAOBJECT__
#define __LUAOBJECT__

class LuaInterface;

class LuaObject : public std::enable_shared_from_this<LuaObject> {
public:
	LuaObject() = default;
	virtual ~LuaObject() {}

	int lua_rawSet(LuaInterface* lua);
	int lua_rawGet(LuaInterface* lua);

	int lua_getMetatable(LuaInterface* lua);

	int lua_getFieldsTable(LuaInterface* lua);

	std::string getCppClassName();

private:
	int m_fieldsTableRef = -1;
};

#endif
