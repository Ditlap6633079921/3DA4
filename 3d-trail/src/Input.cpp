#include "Input.h"

#include <GLFW/glfw3.h>

void Input::Update(GLFWwindow* window)
{
    prev = curr;
    for (int i = 0; i < KeyCount; ++i)
        curr[static_cast<size_t>(i)] = static_cast<unsigned char>(glfwGetKey(window, i));
}

bool Input::Down(int glfwKey) const
{
    if (glfwKey < 0 || glfwKey >= KeyCount)
        return false;
    return curr[static_cast<size_t>(glfwKey)] == GLFW_PRESS;
}

bool Input::Pressed(int glfwKey) const
{
    if (glfwKey < 0 || glfwKey >= KeyCount)
        return false;
    return curr[static_cast<size_t>(glfwKey)] == GLFW_PRESS
        && prev[static_cast<size_t>(glfwKey)] != GLFW_PRESS;
}
