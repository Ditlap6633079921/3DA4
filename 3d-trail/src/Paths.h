#pragma once

#include <string>

std::string GetExecutableDir();
std::string PathJoin(const std::string& a, const std::string& b);
bool ReadTextFile(const std::string& path, std::string& out);
