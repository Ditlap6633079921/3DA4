#pragma once

#include <glm/glm.hpp>

#include <assimp/scene.h>

#include <glad/gl.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SkinnedModel
{
public:
    static constexpr int kMaxBones = 64;

    bool LoadMesh(const std::string& path);

    bool AddClip(const std::string& name, const std::string& path);

    bool AddEmbeddedClip(const std::string& name, unsigned animIndex = 0);

    bool HasMesh() const { return !meshes_.empty(); }
    bool HasSkeleton() const { return boneCount_ > 0; }

    void ComputeBoneMatrices(const std::string& clipName, float timeSec, std::vector<glm::mat4>& out);

    float ClipDurationSec(const std::string& clipName) const;

    void Draw(GLint uniformUseTexture, GLint uniformDiffuse) const;

    void Shutdown();

    struct SkinnedVertex
    {
        glm::vec3 pos{0};
        glm::vec3 norm{0, 1, 0};
        glm::vec2 uv{0, 0};
        glm::ivec4 boneIds{0, 0, 0, 0};
        glm::vec4 weights{0, 0, 0, 0};
    };

private:
    struct MeshGl
    {
        unsigned vao = 0;
        unsigned vbo = 0;
        unsigned ebo = 0;
        unsigned indexCount = 0;
        unsigned diffuseTex = 0;
        bool hasDiffuse = false;
    };

    struct BoneInfo
    {
        int id = -1;
        glm::mat4 offset{1.0f};
    };

    struct Clip
    {
        const aiAnimation* anim = nullptr;
        float durationSec = 0.0f;
        std::shared_ptr<const aiScene> owner;
    };

    void ProcessMesh(const aiScene* scene, aiMesh* mesh);
    void ProcessNode(const aiScene* scene, aiNode* node);
    int AcquireBoneId(const std::string& name, const glm::mat4& offset);

    void ReadHierarchy(float timeTicks,
                       const aiAnimation* anim,
                       const aiNode* node,
                       const glm::mat4& parent,
                       std::vector<glm::mat4>& out,
                       bool freezeRootMotion);

    const aiNodeAnim* FindNodeAnim(const aiAnimation* anim, const std::string& nodeName) const;

    GLuint LoadDiffuseTexture(const aiScene* scene, const aiMaterial* mat, const std::string& modelDir);

    std::vector<MeshGl> meshes_;
    std::unordered_map<std::string, BoneInfo> bones_;
    int boneCount_ = 0;
    glm::mat4 globalInverse_{1.0f};

    std::shared_ptr<const aiScene> meshScene_;
    std::string modelDir_;

    std::unordered_map<std::string, Clip> clips_;
};
