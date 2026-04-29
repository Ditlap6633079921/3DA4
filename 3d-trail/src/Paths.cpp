#include "Paths.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <limits.h>
#include <mach-o/dyld.h>
#include <unistd.h>
#endif

std::string PathJoin(const std::string& a, const std::string& b)
{
    if (a.empty())
        return b;
    if (!a.empty() && (a.back() == '/' || a.back() == '\\'))
        return a + b;
    return a + "/" + b;
}

std::string GetExecutableDir()
{
#if defined(_WIN32)
    wchar_t wbuf[MAX_PATH];
    const DWORD n = GetModuleFileNameW(nullptr, wbuf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return ".";
    std::wstring w(wbuf);
    const size_t slash = w.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
        return ".";
    w.resize(slash);
    std::string out;
    out.reserve(w.size() * 2);
    for (wchar_t c : w)
        out.push_back(static_cast<char>(c));
    return out;
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buf;
    buf.resize(size + 1);
    if (_NSGetExecutablePath(buf.data(), &size) != 0)
        return ".";
    buf.resize(std::strlen(buf.c_str()));
    char resolved[PATH_MAX];
    if (realpath(buf.c_str(), resolved))
    {
        std::string full(resolved);
        const size_t slash = full.find_last_of('/');
        return (slash == std::string::npos) ? "." : full.substr(0, slash);
    }
    const size_t slash = buf.find_last_of('/');
    return (slash == std::string::npos) ? "." : buf.substr(0, slash);
#else
    return ".";
#endif
}

bool ReadTextFile(const std::string& path, std::string& out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}
