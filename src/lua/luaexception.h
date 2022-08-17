#ifndef __LUAEXCEPTION__
#define __LUAEXCEPTION__

#include <exception>

class LuaInterface;

class LuaException : public std::exception
{
public:
    LuaException(const std::string& error, int traceLevel = -1);
    virtual ~LuaException() throw() { }

    void generateLuaErrorMessage(const std::string& error, int traceLevel);

    virtual const char* what() const throw() { return m_what.c_str(); }

protected:
    LuaException() { }

    std::string m_what;
};

class LuaBadValueCastException : public LuaException
{
public:
    LuaBadValueCastException(const std::string& luaTypeName, const std::string& cppTypeName);
};

#endif

