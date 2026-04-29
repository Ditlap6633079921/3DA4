#include <cstdio>
#include <string>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "Input.h"
#include "RunnerGame.h"
#include "RunnerRenderer.h"

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* renderer = static_cast<RunnerRenderer*>(glfwGetWindowUserPointer(window));
    if (renderer)
        renderer->OnFramebufferSize(width, height);
}

int main()
{
    if (!glfwInit())
    {
        std::fprintf(stderr, "glfwInit failed\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "3D Trail — Subway-style runner", nullptr, nullptr);
    if (!window)
    {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
    {
        std::fprintf(stderr, "gladLoadGLLoader failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    RunnerRenderer renderer;
    glfwSetWindowUserPointer(window, &renderer);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    renderer.OnFramebufferSize(fbw, fbh);

    if (!renderer.Init())
    {
        std::fprintf(stderr, "Renderer::Init failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    RunnerGame game;
    Input input;

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        glfwPollEvents();
        input.Update(window);

        if (input.Pressed(GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        if (input.Pressed(GLFW_KEY_R))
            game.Reset();

        if (input.Pressed(GLFW_KEY_SPACE))
        {
            if (game.IsGameOver())
            {
                game.Reset();
                game.StartRun();
            }
            else if (!game.IsPlaying())
                game.StartRun();
            else
                game.OnJump();
        }

        if (game.IsPlaying() && !game.IsGameOver())
        {
            if (input.Pressed(GLFW_KEY_A) || input.Pressed(GLFW_KEY_LEFT))
                game.OnLaneLeft();
            if (input.Pressed(GLFW_KEY_D) || input.Pressed(GLFW_KEY_RIGHT))
                game.OnLaneRight();
        }

        game.Update(dt);

        {
            std::string title = "3D Trail  |  Score " + std::to_string(static_cast<int>(game.Score()));
            if (!game.IsPlaying())
                title += "  |  Side-scroller  Space start";
            else if (game.IsGameOver())
                title += "  |  Game over  Space restart  R reset";
            else
                title += "  |  Auto-run  A/D lanes  Space jump";
            glfwSetWindowTitle(window, title.c_str());
        }

        renderer.Render(game);
        glfwSwapBuffers(window);
    }

    renderer.Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
