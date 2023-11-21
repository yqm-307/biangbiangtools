#pragma once
#include "./Config.hpp"
#include <string>

namespace bbt::cxxlua
{

enum ERRCODE
{
    Default = 0, 
    Succes  = 1,
    Failed  = 2,
    // lua vm
    ErrParams = 3,  // 参数错误
    ErrSyntax = 4,  // 语法错误
    ErrMem    = 5,  // 内存分配错误
};

namespace detail
{

class LuaErr
{
public:
    LuaErr():
        m_traceback_err(std::nullopt), m_errcode(ERRCODE::Default) {}
    LuaErr(const char* cstr, bbt::cxxlua::ERRCODE code):
        m_traceback_err(cstr), m_errcode(code) {}
    LuaErr(LuaErr&& other):
        m_traceback_err(std::move(other.m_traceback_err)),
        m_errcode(m_errcode){}
    ~LuaErr() {}

    void Clear(){m_traceback_err.value().clear(); m_errcode = bbt::cxxlua::ERRCODE::Default;}

    void Reset(const char* cstr, bbt::cxxlua::ERRCODE code = Default)
    { m_traceback_err = std::string(cstr); m_errcode = code; }

    void SetStr(const std::string& str) 
    { m_traceback_err = str; }

    void SetCode(ERRCODE code)
    { m_errcode = code; }

    void Swap(LuaErr&& other)
    { 
        if(other.m_traceback_err != std::nullopt)
            m_traceback_err.emplace(other.m_traceback_err.value());
        m_errcode = other.m_errcode;
    }

    std::string What()
    { return m_traceback_err == std::nullopt ? "" : m_traceback_err.value(); }

    ERRCODE Code()
    { return m_errcode; }
private:
    std::optional<std::string> m_traceback_err;
    bbt::cxxlua::ERRCODE m_errcode;
};

}

}
