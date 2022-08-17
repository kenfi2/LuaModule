#include <includes/includes.h>

#include "luaexception.h"

#include "luainterface.h"

LuaException::LuaException(const std::string& error, int traceLevel)
{
    lua_pop(g_lua.getLuaState(), lua_gettop(g_lua.getLuaState())); // on every exception, clear lua stack
    generateLuaErrorMessage(error, traceLevel);
}

void LuaException::generateLuaErrorMessage(const std::string& error, int traceLevel)
{
    if (traceLevel >= 0)
        m_what = g_lua.getStackTrace(error, traceLevel);
    else
        m_what = error;
}

LuaBadValueCastException::LuaBadValueCastException(const std::string& luaTypeName, const std::string& cppTypeName)
{
    std::ostringstream error;
    error << "attempt to cast a '"<< luaTypeName << "' lua value to '" << cppTypeName << "'";
    generateLuaErrorMessage(error.str(), 0);
}

