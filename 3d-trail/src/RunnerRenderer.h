#pragma once

#include <glad/gl.h>

#include <glm/glm.hpp>

#include <vector>

#include "SkinnedModel.h"

class RunnerGame;

class RunnerRenderer
{
public:
    bool Init();
    void Shutdown();

    void OnFramebufferSize(int width, int height);

    void Render(const RunnerGame& game);

private:
    bool LoadShaders();
    bool CreateCubeMesh();
    void DestroyCubeMesh();
    bool ResolveResourcePaths(std::string& outSkin,
                              std::string& outIdle,
                              std::string& outRun,
                              std::string& outFail) const;
    void TryLoadCharacter(const std::string& skinPath,
                          const std::string& idlePath,
                          const std::string& runPath,
                          const std::string& failPath);

    static GLuint Compile(GLenum type, const char* src);
    static GLuint Link(GLuint vs, GLuint fs);

    void DrawCube(const glm::mat4& viewProj, const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& color);

    unsigned colorProgram_ = 0;
    int uColorMvp_ = -1;
    int uColorRgb_ = -1;

    unsigned skinnedProgram_ = 0;
    int uSkinModel_ = -1;
    int uSkinViewProj_ = -1;
    int uSkinBones_ = -1;
    int uSkinRgb_ = -1;
    int uSkinUseTex_ = -1;
    int uSkinDiffuse_ = -1;

    unsigned cubeVao_ = 0;
    unsigned cubeVbo_ = 0;

    int fbW_ = 1280;
    int fbH_ = 720;

    SkinnedModel character_;
    bool useCharacter_ = false;
    bool hasRunClip_ = false;
    bool hasIdleClip_ = false;
    bool hasFailClip_ = false;

    std::vector<glm::mat4> boneMats_;
};
