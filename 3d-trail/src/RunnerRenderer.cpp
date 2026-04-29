#include "RunnerRenderer.h"

#include "Paths.h"
#include "RunnerGame.h"

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/gl.h>

namespace {

bool FileExists(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    return f.good();
}

std::string FirstExisting(const std::vector<std::string>& candidates)
{
    for (const std::string& p : candidates)
    {
        if (FileExists(p))
            return p;
    }
    return {};
}

} // namespace

GLuint RunnerRenderer::Compile(GLenum type, const char* src)
{
    const GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        GLsizei len = 0;
        glGetShaderInfoLog(s, sizeof(log), &len, log);
        std::fprintf(stderr, "Shader compile failed: %s\n", log);
    }
    return s;
}

GLuint RunnerRenderer::Link(GLuint vs, GLuint fs)
{
    const GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        GLsizei len = 0;
        glGetProgramInfoLog(p, sizeof(log), &len, log);
        std::fprintf(stderr, "Program link failed: %s\n", log);
    }
    return p;
}

bool RunnerRenderer::LoadShaders()
{
    const std::string exe = GetExecutableDir();
    const std::string cVsNear = PathJoin(exe, "assets/shaders/color.vert");
    const std::string cFsNear = PathJoin(exe, "assets/shaders/color.frag");
    const std::string sVsNear = PathJoin(exe, "assets/shaders/skinned.vert");
    const std::string sFsNear = PathJoin(exe, "assets/shaders/skinned.frag");

    std::string cv, cf, sv, sf;
    if (!ReadTextFile(cVsNear, cv) || !ReadTextFile(cFsNear, cf))
    {
        std::fprintf(stderr, "Missing color shaders (tried %s)\n", cVsNear.c_str());
        return false;
    }
    if (!ReadTextFile(sVsNear, sv) || !ReadTextFile(sFsNear, sf))
    {
        std::fprintf(stderr, "Missing skinned shaders (tried %s)\n", sVsNear.c_str());
        return false;
    }

    const char* cvP = cv.c_str();
    const char* cfP = cf.c_str();
    GLuint cvs = Compile(GL_VERTEX_SHADER, cvP);
    GLuint cfs = Compile(GL_FRAGMENT_SHADER, cfP);
    colorProgram_ = Link(cvs, cfs);
    glDeleteShader(cvs);
    glDeleteShader(cfs);
    if (colorProgram_ == 0)
        return false;
    uColorMvp_ = glGetUniformLocation(colorProgram_, "uMvp");
    uColorRgb_ = glGetUniformLocation(colorProgram_, "uColor");

    const char* svP = sv.c_str();
    const char* sfP = sf.c_str();
    GLuint svs = Compile(GL_VERTEX_SHADER, svP);
    GLuint sfs = Compile(GL_FRAGMENT_SHADER, sfP);
    skinnedProgram_ = Link(svs, sfs);
    glDeleteShader(svs);
    glDeleteShader(sfs);
    if (skinnedProgram_ == 0)
        return false;
    uSkinModel_ = glGetUniformLocation(skinnedProgram_, "uModel");
    uSkinViewProj_ = glGetUniformLocation(skinnedProgram_, "uViewProj");
    uSkinBones_ = glGetUniformLocation(skinnedProgram_, "uBones");
    uSkinRgb_ = glGetUniformLocation(skinnedProgram_, "uColor");
    uSkinUseTex_ = glGetUniformLocation(skinnedProgram_, "uUseTexture");
    uSkinDiffuse_ = glGetUniformLocation(skinnedProgram_, "uDiffuse");
    return true;
}

bool RunnerRenderer::CreateCubeMesh()
{
    const float v[] = {
        -0.5f, -0.5f, 0.5f,  0.5f, -0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f,  0.5f, 0.5f, -0.5f,
         0.5f, 0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f,  0.5f, -0.5f, -0.5f,  0.5f, 0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.5f,  0.5f, -0.5f, 0.5f,
        -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f,  0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
         0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f};

    glGenVertexArrays(1, &cubeVao_);
    glGenBuffers(1, &cubeVbo_);
    glBindVertexArray(cubeVao_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    return true;
}

void RunnerRenderer::DestroyCubeMesh()
{
    if (cubeVbo_)
        glDeleteBuffers(1, &cubeVbo_);
    if (cubeVao_)
        glDeleteVertexArrays(1, &cubeVao_);
    cubeVbo_ = 0;
    cubeVao_ = 0;
}

bool RunnerRenderer::ResolveResourcePaths(std::string& outSkin,
                                          std::string& outIdle,
                                          std::string& outRun,
                                          std::string& outFail) const
{
    const std::string exe    = GetExecutableDir();
    const std::string rExe   = PathJoin(exe, "resource");
    const std::string rCwd   = "resource";
    const std::string rUp    = PathJoin(exe, "../resource");

    auto pickSkin = [&]()
    {
        return FirstExisting({
            PathJoin(rExe, "skin/Ch46_nonPBR.fbx"),
            PathJoin(rCwd, "skin/Ch46_nonPBR.fbx"),
            PathJoin(rUp, "skin/Ch46_nonPBR.fbx"),
        });
    };
    auto pickRun = [&]()
    {
        return FirstExisting({
            PathJoin(rExe, "animation_with_skin/Drunk Run Forward/Drunk Run Forward.dae"),
            PathJoin(rCwd, "animation_with_skin/Drunk Run Forward/Drunk Run Forward.dae"),
            PathJoin(rUp, "animation_with_skin/Drunk Run Forward/Drunk Run Forward.dae"),
            PathJoin(rExe, "animation_with_skin/Drunk Run Forward/Drunk Run Forward.fbx"),
            PathJoin(rCwd, "animation_with_skin/Drunk Run Forward/Drunk Run Forward.fbx"),
            PathJoin(rUp, "animation_with_skin/Drunk Run Forward/Drunk Run Forward.fbx"),
            PathJoin(rExe, "animation/Female Walk .fbx"),
            PathJoin(rExe, "animation/Female Walk.fbx"),
            PathJoin(rCwd, "animation/Female Walk .fbx"),
            PathJoin(rUp, "animation/Female Walk .fbx"),
        });
    };
    auto pickIdle = [&]()
    {
        return FirstExisting({
            PathJoin(rExe, "animation_with_skin/Sitting Idle/Sitting Idle.dae"),
            PathJoin(rCwd, "animation_with_skin/Sitting Idle/Sitting Idle.dae"),
            PathJoin(rUp, "animation_with_skin/Sitting Idle/Sitting Idle.dae"),
            PathJoin(rExe, "animation_with_skin/Sitting Idle/Sitting Idle.fbx"),
            PathJoin(rCwd, "animation_with_skin/Sitting Idle/Sitting Idle.fbx"),
            PathJoin(rUp, "animation_with_skin/Sitting Idle/Sitting Idle.fbx"),
            PathJoin(rExe, "animation_with_skin/Idle/Idle.dae"),
            PathJoin(rExe, "animation_with_skin/Idle/Idle.fbx"),
            PathJoin(rCwd, "animation_with_skin/Idle/Idle.dae"),
            PathJoin(rUp, "animation_with_skin/Idle/Idle.dae"),
            PathJoin(rExe, "animation/Female Dynamic Pose.fbx"),
            PathJoin(rCwd, "animation/Female Dynamic Pose.fbx"),
            PathJoin(rUp, "animation/Female Dynamic Pose.fbx"),
        });
    };
    auto pickFail = [&]()
    {
        return FirstExisting({
            PathJoin(rExe, "animation_with_skin/Sitting Idle/Sitting Idle.dae"),
            PathJoin(rCwd, "animation_with_skin/Sitting Idle/Sitting Idle.dae"),
            PathJoin(rUp, "animation_with_skin/Sitting Idle/Sitting Idle.dae"),
            PathJoin(rExe, "animation_with_skin/Sitting Idle/Sitting Idle.fbx"),
            PathJoin(rExe, "animation_with_skin/Fail/Fail.dae"),
            PathJoin(rExe, "animation_with_skin/Fail/Fail.fbx"),
            PathJoin(rCwd, "animation_with_skin/Fail/Fail.dae"),
            PathJoin(rUp, "animation_with_skin/Fail/Fail.dae"),
            PathJoin(rExe, "animation/Sitting Laughing.fbx"),
            PathJoin(rCwd, "animation/Sitting Laughing.fbx"),
            PathJoin(rUp, "animation/Sitting Laughing.fbx"),
        });
    };

    outSkin = pickSkin();
    outIdle = pickIdle();
    outRun = pickRun();
    outFail = pickFail();

    if (outRun.empty())
    {
        std::fprintf(stderr,
                     "Could not find run animation. Add one under resource/animation_with_skin/ "
                     "(e.g. …/Drunk Run Forward/Drunk Run Forward.dae) or resource/animation/*.fbx\n");
        return false;
    }
    if (outSkin.empty())
        std::fprintf(stderr, "Note: skin FBX not found; using walk FBX as character mesh\n");
    return true;
}

void RunnerRenderer::TryLoadCharacter(const std::string& skinPath,
                                      const std::string& idlePath,
                                      const std::string& runPath,
                                      const std::string& failPath)
{
    character_.Shutdown();
    useCharacter_ = false;
    hasRunClip_ = hasIdleClip_ = hasFailClip_ = false;

    if (!runPath.empty() && character_.LoadMesh(runPath))
    {
        useCharacter_ = true;
        hasRunClip_ = character_.AddEmbeddedClip("run", 0);
        if (!hasRunClip_)
            std::fprintf(stderr,
                         "3d-trail: \"%s\" has no embedded animation. On Mixamo choose Download, FBX, "
                         "and set Skin to \"With Skin\" so the walk and skeleton match.\n",
                         runPath.c_str());
        if (!idlePath.empty())
            hasIdleClip_ = character_.AddClip("idle", idlePath);
        if (!failPath.empty())
            hasFailClip_ = character_.AddClip("fail", failPath);
        std::fprintf(stderr,
                     "3d-trail: using \"%s\" as mesh (same file as run). embedded_run=%d idle=%d fail=%d\n",
                     runPath.c_str(), (int)hasRunClip_, (int)hasIdleClip_, (int)hasFailClip_);
        std::fprintf(stderr,
                     "Tip: For idle/fail, pick the SAME Mixamo character as the walk, or paste those "
                     "FBX files from \"With Skin\" downloads only.\n");
        return;
    }

    if (!skinPath.empty() && character_.LoadMesh(skinPath))
    {
        useCharacter_ = true;
        if (!runPath.empty())
            hasRunClip_ = character_.AddClip("run", runPath);
        if (!hasRunClip_)
            hasRunClip_ = character_.AddEmbeddedClip("run", 0);
        if (!idlePath.empty())
            hasIdleClip_ = character_.AddClip("idle", idlePath);
        if (!failPath.empty())
            hasFailClip_ = character_.AddClip("fail", failPath);
        std::fprintf(stderr,
                     "3d-trail: skin fallback \"%s\". run_clip=%d idle=%d fail=%d\n",
                     skinPath.c_str(), (int)hasRunClip_, (int)hasIdleClip_, (int)hasFailClip_);
        std::fprintf(stderr,
                     "If the body is twisted or collapsed, your walk FBX is for a different character "
                     "than \"%s\". Use ONE avatar on Mixamo; download every animation With Skin, "
                     "or only \"Without Skin\" after binding all clips to that avatar.\n",
                     skinPath.c_str());
        return;
    }

    std::fprintf(stderr, "3d-trail: failed to load any character model (FBX/DAE); using proxy cube.\n");
}

bool RunnerRenderer::Init()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    if (!LoadShaders())
        return false;
    if (!CreateCubeMesh())
        return false;

    boneMats_.assign(static_cast<size_t>(SkinnedModel::kMaxBones), glm::mat4(1.0f));

    std::string skin, idle, run, fail;
    if (ResolveResourcePaths(skin, idle, run, fail))
    {
        TryLoadCharacter(skin, idle, run, fail);
    }

    return true;
}

void RunnerRenderer::Shutdown()
{
    character_.Shutdown();
    DestroyCubeMesh();
    if (colorProgram_)
        glDeleteProgram(colorProgram_);
    if (skinnedProgram_)
        glDeleteProgram(skinnedProgram_);
    colorProgram_ = 0;
    skinnedProgram_ = 0;
    uColorMvp_ = -1;
    uColorRgb_ = -1;
    uSkinModel_ = -1;
    uSkinViewProj_ = -1;
    uSkinBones_ = -1;
    uSkinRgb_ = -1;
    uSkinUseTex_ = -1;
    uSkinDiffuse_ = -1;
}

void RunnerRenderer::OnFramebufferSize(int width, int height)
{
    fbW_ = (width > 0) ? width : 1;
    fbH_ = (height > 0) ? height : 1;
    glViewport(0, 0, fbW_, fbH_);
}

void RunnerRenderer::DrawCube(const glm::mat4& viewProj, const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& color)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, pos);
    model = glm::scale(model, scale);
    const glm::mat4 mvp = viewProj * model;
    glUseProgram(colorProgram_);
    glUniformMatrix4fv(uColorMvp_, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(uColorRgb_, 1, &color[0]);
    glBindVertexArray(cubeVao_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void RunnerRenderer::Render(const RunnerGame& game)
{
    glClearColor(0.45f, 0.72f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Side-on “Mario” style: run direction +X, jump +Y, lane depth Z.
    // IMPORTANT: ortho bounds must be symmetric around 0 — lookAt maps the target into
    // camera space near the origin, so ortho(0..viewW, 0..viewH) clips everything off-screen.
    const float aspect = static_cast<float>(fbW_) / static_cast<float>(fbH_);
    const float halfH = 6.5f;
    const float halfW = halfH * aspect;
    const glm::mat4 proj = glm::ortho(-halfW, halfW, -halfH, halfH, 0.5f, 200.0f);
    const glm::vec3 eye(-12.0f, 5.5f, 16.0f);
    const glm::vec3 at(4.5f, 2.0f, 0.0f);
    const glm::mat4 view = glm::lookAt(eye, at, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 viewProj = proj * view;

    const float scroll = game.ScrollDistance();

    // Backdrop: draw without depth so it always sits behind playfield.
    glDisable(GL_DEPTH_TEST);
    DrawCube(viewProj,
             glm::vec3(22.0f, 8.5f, -9.5f),
             glm::vec3(110.0f, 22.0f, 0.4f),
             glm::vec3(0.38f, 0.62f, 0.92f));
    DrawCube(viewProj,
             glm::vec3(22.0f, 5.2f, -9.2f),
             glm::vec3(110.0f, 2.8f, 0.35f),
             glm::vec3(0.75f, 0.88f, 0.95f));

    auto hash01 = [](int i, int salt) -> float
    {
        uint32_t x = static_cast<uint32_t>(i * 374761393 + salt * 668265263);
        x = (x ^ (x >> 13)) * 1274126177;
        return (x ^ (x >> 16)) * (1.0f / 4294967296.0f);
    };

    // Parallax skyline — two rows drift opposite to run direction (+X world is “coming at” player).
    const float farZ = -5.15f;
    const float nearZ = -4.78f;
    for (int layer : {0, 1})
    {
        const float z = layer == 0 ? farZ : nearZ;
        const float px = layer == 0 ? 0.09f : 0.2f;
        const glm::vec3 baseCol = layer == 0 ? glm::vec3(0.11f, 0.12f, 0.16f) : glm::vec3(0.16f, 0.18f, 0.22f);
        const float slice = 3.15f;
        const int count = 45;
        const float phase = std::fmod(scroll * px, slice);
        for (int i = -4; i < count; ++i)
        {
            const float x = -28.0f + static_cast<float>(i) * slice - phase;
            const float h = 1.15f + hash01(i, layer * 31) * 2.85f;
            const float w = 0.85f + hash01(i, 7 + layer) * 0.55f;
            const glm::vec3 pos(x, 0.22f + h * 0.5f, z);
            DrawCube(viewProj, pos, glm::vec3(w, h, 0.32f), baseCol);
        }
    }

    // Soft clouds (very slow parallax)
    const float cloudSlice = 8.0f;
    const float cloudPhase = std::fmod(scroll * 0.045f, cloudSlice);
    for (int c = -2; c < 12; ++c)
    {
        const float cx = -15.0f + static_cast<float>(c) * cloudSlice - cloudPhase;
        const float cy = 5.9f + 0.35f * std::sin(static_cast<float>(c) * 0.7f);
        const float s = 2.0f + 0.4f * hash01(c, 99);
        DrawCube(viewProj, glm::vec3(cx, cy, -8.9f), glm::vec3(s * 1.2f, s * 0.35f, 0.25f), glm::vec3(0.94f, 0.96f, 1.0f));
        DrawCube(viewProj, glm::vec3(cx + s * 0.55f, cy + 0.15f, -8.88f), glm::vec3(s * 0.75f, s * 0.28f, 0.2f), glm::vec3(0.9f, 0.94f, 1.0f));
    }

    glEnable(GL_DEPTH_TEST);

    constexpr float kLaneStep = 2.1f;
    const float laneZ = game.PlayerX();

    // Track scrolls with speed so motion reads even between obstacle spawns
    constexpr float kTileX = 2.35f;
    const float trackPhase = std::fmod(scroll, kTileX);

    for (int i = -3; i < 36; ++i)
    {
        const float x = -8.0f + static_cast<float>(i) * kTileX - trackPhase;
        DrawCube(viewProj,
                 glm::vec3(x, -0.15f, 0.0f),
                 glm::vec3(2.2f, 0.16f, 8.2f),
                 glm::vec3(0.38f, 0.39f, 0.41f));
        for (int lane : {-1, 0, 1})
            DrawCube(viewProj,
                     glm::vec3(x, 0.03f, static_cast<float>(lane) * kLaneStep),
                     glm::vec3(1.45f, 0.07f, 1.25f),
                     glm::vec3(0.18f, 0.72f, 0.32f));
    }

    // Moving lane center-line dashes (white streaks)
    const float dashPeriod = 1.8f;
    const float dashPhase = std::fmod(scroll * 1.15f, dashPeriod);
    for (int lane : {-1, 0, 1})
    {
        const float lz = static_cast<float>(lane) * kLaneStep;
        for (int d = -10; d < 40; ++d)
        {
            const float dx = -6.0f + static_cast<float>(d) * dashPeriod - dashPhase;
            DrawCube(viewProj,
                     glm::vec3(dx, 0.045f, lz),
                     glm::vec3(0.5f, 0.02f, 0.12f),
                     glm::vec3(0.92f, 0.92f, 0.88f));
        }
    }

    for (const Obstacle& o : game.Obstacles())
    {
        const float oz = static_cast<float>(o.lane) * kLaneStep;
        const glm::vec3 c(o.z, o.tall ? 0.72f : 0.38f, oz);
        const glm::vec3 s(0.75f, o.tall ? 1.35f : 0.65f, 0.75f);
        const glm::vec3 col = o.tall ? glm::vec3(0.92f, 0.32f, 0.1f) : glm::vec3(0.95f, 0.8f, 0.15f);
        DrawCube(viewProj, c, s, col);
    }

    const glm::vec3 playerPos(0.0f, game.PlayerY(), laneZ);
    glm::mat4 charModel(1.0f);
    charModel = glm::translate(charModel, playerPos + glm::vec3(0.15f, 0.1f, 0.0f));
    charModel = glm::rotate(charModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    charModel = glm::scale(charModel, glm::vec3(0.012f));

    if (useCharacter_)
    {
        std::string clip = "run";
        float t = game.AnimTime();
        if (game.IsGameOver())
        {
            if (hasFailClip_)
                clip = "fail";
            else if (hasIdleClip_)
                clip = "idle";
            t = game.FailAnimTime();
        }
        else if (!game.IsPlaying())
        {
            clip = hasIdleClip_ ? "idle" : (hasRunClip_ ? "run" : "idle");
        }
        else if (!hasRunClip_)
            clip = hasIdleClip_ ? "idle" : "run";

        character_.ComputeBoneMatrices(clip, t, boneMats_);

        glUseProgram(skinnedProgram_);
        glUniformMatrix4fv(uSkinModel_, 1, GL_FALSE, glm::value_ptr(charModel));
        glUniformMatrix4fv(uSkinViewProj_, 1, GL_FALSE, glm::value_ptr(viewProj));
        glUniformMatrix4fv(uSkinBones_, SkinnedModel::kMaxBones, GL_FALSE, glm::value_ptr(boneMats_[0]));
        glUniform3f(uSkinRgb_, 1.0f, 1.0f, 1.0f);
        character_.Draw(uSkinUseTex_, uSkinDiffuse_);
    }
    else
    {
        DrawCube(viewProj, playerPos + glm::vec3(0.0f, 0.95f, 0.0f), glm::vec3(0.65f, 1.75f, 0.45f), glm::vec3(0.2f, 0.45f, 0.95f));
    }
}
