#pragma once

#include <array>

struct GLFWwindow;

class Input
{
public:
    void Update(GLFWwindow* window);

    bool Down(int glfwKey) const;
    bool Pressed(int glfwKey) const;

private:
    static constexpr int KeyCount = 512;
    std::array<unsigned char, KeyCount> curr{};
    std::array<unsigned char, KeyCount> prev{};
};
