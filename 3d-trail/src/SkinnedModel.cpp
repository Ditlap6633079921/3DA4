#include "SkinnedModel.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/material.h>
#include <algorithm>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <string>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/gl.h>
#include <stb_image.h>

#include "Paths.h"

namespace {

glm::mat4 AiToGlm(const aiMatrix4x4& m)
{
    glm::mat4 g;
    g[0][0] = m.a1;
    g[1][0] = m.a2;
    g[2][0] = m.a3;
    g[3][0] = m.a4;
    g[0][1] = m.b1;
    g[1][1] = m.b2;
    g[2][1] = m.b3;
    g[3][1] = m.b4;
    g[0][2] = m.c1;
    g[1][2] = m.c2;
    g[2][2] = m.c3;
    g[3][2] = m.c4;
    g[0][3] = m.d1;
    g[1][3] = m.d2;
    g[2][3] = m.d3;
    g[3][3] = m.d4;
    return g;
}

void ReleaseAIScene(const aiScene* p)
{
    if (p)
        aiReleaseImport(p);
}

static void AddBoneWeight(SkinnedModel::SkinnedVertex& v, int boneId, float weight)
{
    for (int i = 0; i < 4; ++i)
    {
        if (v.weights[i] <= 0.0f)
        {
            v.boneIds[i] = boneId;
            v.weights[i] = weight;
            return;
        }
    }
}

unsigned FindPositionKey(float t, const aiNodeAnim* anim)
{
    for (unsigned i = 0; i < anim->mNumPositionKeys - 1; ++i)
    {
        if (t < static_cast<float>(anim->mPositionKeys[i + 1].mTime))
            return i;
    }
    return anim->mNumPositionKeys > 0 ? anim->mNumPositionKeys - 1 : 0;
}

unsigned FindRotationKey(float t, const aiNodeAnim* anim)
{
    for (unsigned i = 0; i < anim->mNumRotationKeys - 1; ++i)
    {
        if (t < static_cast<float>(anim->mRotationKeys[i + 1].mTime))
            return i;
    }
    return anim->mNumRotationKeys > 0 ? anim->mNumRotationKeys - 1 : 0;
}

unsigned FindScalingKey(float t, const aiNodeAnim* anim)
{
    for (unsigned i = 0; i < anim->mNumScalingKeys - 1; ++i)
    {
        if (t < static_cast<float>(anim->mScalingKeys[i + 1].mTime))
            return i;
    }
    return anim->mNumScalingKeys > 0 ? anim->mNumScalingKeys - 1 : 0;
}

glm::vec3 InterpolatePosition(float t, const aiNodeAnim* anim)
{
    if (anim->mNumPositionKeys == 0)
        return glm::vec3(0);
    if (anim->mNumPositionKeys == 1)
    {
        const aiVector3D& v = anim->mPositionKeys[0].mValue;
        return glm::vec3(v.x, v.y, v.z);
    }
    const unsigned i = FindPositionKey(t, anim);
    const unsigned next = std::min(i + 1, anim->mNumPositionKeys - 1);
    const float t0 = static_cast<float>(anim->mPositionKeys[i].mTime);
    const float t1 = static_cast<float>(anim->mPositionKeys[next].mTime);
    const float f = (t1 - t0) > 1e-6f ? (t - t0) / (t1 - t0) : 0.0f;
    const aiVector3D& a = anim->mPositionKeys[i].mValue;
    const aiVector3D& b = anim->mPositionKeys[next].mValue;
    const glm::vec3 va(a.x, a.y, a.z);
    const glm::vec3 vb(b.x, b.y, b.z);
    return glm::mix(va, vb, f);
}

glm::quat InterpolateRotation(float t, const aiNodeAnim* anim)
{
    if (anim->mNumRotationKeys == 0)
        return glm::quat(1, 0, 0, 0);
    if (anim->mNumRotationKeys == 1)
    {
        const aiQuaternion& q = anim->mRotationKeys[0].mValue;
        return glm::normalize(glm::quat(q.w, q.x, q.y, q.z));
    }
    const unsigned i = FindRotationKey(t, anim);
    const unsigned next = std::min(i + 1, anim->mNumRotationKeys - 1);
    const float t0 = static_cast<float>(anim->mRotationKeys[i].mTime);
    const float t1 = static_cast<float>(anim->mRotationKeys[next].mTime);
    const float f = (t1 - t0) > 1e-6f ? (t - t0) / (t1 - t0) : 0.0f;
    const aiQuaternion& qa = anim->mRotationKeys[i].mValue;
    const aiQuaternion& qb = anim->mRotationKeys[next].mValue;
    glm::quat q0(qa.w, qa.x, qa.y, qa.z);
    glm::quat q1(qb.w, qb.x, qb.y, qb.z);
    return glm::normalize(glm::slerp(q0, q1, f));
}

glm::vec3 InterpolateScale(float t, const aiNodeAnim* anim)
{
    if (anim->mNumScalingKeys == 0)
        return glm::vec3(1);
    if (anim->mNumScalingKeys == 1)
    {
        const aiVector3D& v = anim->mScalingKeys[0].mValue;
        return glm::vec3(v.x, v.y, v.z);
    }
    const unsigned i = FindScalingKey(t, anim);
    const unsigned next = std::min(i + 1, anim->mNumScalingKeys - 1);
    const float t0 = static_cast<float>(anim->mScalingKeys[i].mTime);
    const float t1 = static_cast<float>(anim->mScalingKeys[next].mTime);
    const float f = (t1 - t0) > 1e-6f ? (t - t0) / (t1 - t0) : 0.0f;
    const aiVector3D& a = anim->mScalingKeys[i].mValue;
    const aiVector3D& b = anim->mScalingKeys[next].mValue;
    return glm::mix(glm::vec3(a.x, a.y, a.z), glm::vec3(b.x, b.y, b.z), f);
}

static bool IsHipsOrPelvisNode(const std::string& name)
{
    if (name == "Hips" || name == "pelvis" || name == "Pelvis")
        return true;
    const size_t n = name.size();
    // Mixamo Collada: joint id "mixamorig_Hips"; Assimp keeps underscore (not ":Hips").
    if (n >= 5)
    {
        if (name.compare(n - 5, 5, "_Hips") == 0 || name.compare(n - 5, 5, ":Hips") == 0)
            return true;
    }
    return false;
}

} // namespace

// Mixamo FBX often uses an extra node under root; a bad inverse collapses the mesh.
static glm::mat4 GlobalInverseForScene(const aiScene* scene)
{
    if (!scene || !scene->mRootNode)
        return glm::mat4(1.0f);

    glm::mat4 root = AiToGlm(scene->mRootNode->mTransformation);
    if (scene->mRootNode->mNumChildren == 1)
    {
        const glm::mat4 child = AiToGlm(scene->mRootNode->mChildren[0]->mTransformation);
        const glm::mat4 combined = root * child;
        const float d = glm::determinant(combined);
        if (std::fabs(d) > 1e-8f && std::fabs(d) < 1e9f)
            return glm::inverse(combined);
    }
    const float d = glm::determinant(root);
    if (std::fabs(d) > 1e-8f && std::fabs(d) < 1e9f)
        return glm::inverse(root);
    return glm::mat4(1.0f);
}

static std::string DirNameOf(const std::string& path)
{
    const size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos)
        return ".";
    return path.substr(0, slash);
}

GLuint SkinnedModel::LoadDiffuseTexture(const aiScene* scene, const aiMaterial* mat, const std::string& modelDir)
{
    if (!mat)
        return 0;
    aiString texStr;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texStr) != AI_SUCCESS
        && mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texStr) != AI_SUCCESS)
        return 0;

    const char* p = texStr.C_Str();
    if (!p || !p[0])
        return 0;

    stbi_set_flip_vertically_on_load(1);

    int w = 0, h = 0, ch = 0;
    unsigned char* pixels = nullptr;

    if (p[0] == '*' && scene)
    {
        const int idx = std::atoi(p + 1);
        if (idx < 0 || idx >= static_cast<int>(scene->mNumTextures))
            return 0;
        const aiTexture* t = scene->mTextures[idx];
        if (t->mHeight == 0)
            pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(t->pcData),
                                           static_cast<int>(t->mWidth),
                                           &w, &h, &ch, 4);
    }
    else
    {
        std::string full = PathJoin(modelDir, std::string(p));
        for (char& c : full)
        {
            if (c == '\\')
                c = '/';
        }
        pixels = stbi_load(full.c_str(), &w, &h, &ch, 4);
    }

    if (!pixels || w <= 0 || h <= 0)
    {
        if (pixels)
            stbi_image_free(pixels);
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

bool SkinnedModel::LoadMesh(const std::string& path)
{
    Shutdown();

    modelDir_ = DirNameOf(path);

    constexpr unsigned flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                               aiProcess_LimitBoneWeights | aiProcess_ValidateDataStructure;

    aiScene* raw = const_cast<aiScene*>(aiImportFile(path.c_str(), flags));
    if (!raw || raw->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !raw->mRootNode)
    {
        std::fprintf(stderr, "SkinnedModel: failed to load %s\n", path.c_str());
        if (raw)
            aiReleaseImport(raw);
        return false;
    }

    meshScene_ = std::shared_ptr<const aiScene>(raw, ReleaseAIScene);
    globalInverse_ = GlobalInverseForScene(raw);

    ProcessNode(raw, raw->mRootNode);
    if (meshes_.empty())
    {
        std::fprintf(stderr, "SkinnedModel: no meshes in %s\n", path.c_str());
        return false;
    }

    if (boneCount_ > kMaxBones)
        std::fprintf(stderr, "SkinnedModel: warning %d bones, clamping to %d in shader\n",
                     boneCount_, kMaxBones);

    return true;
}

bool SkinnedModel::AddClip(const std::string& name, const std::string& path)
{
    constexpr unsigned flags = aiProcess_Triangulate;
    aiScene* raw = const_cast<aiScene*>(aiImportFile(path.c_str(), flags));
    if (!raw || !raw->HasAnimations())
    {
        std::fprintf(stderr, "SkinnedModel: no animation in %s\n", path.c_str());
        if (raw)
            aiReleaseImport(raw);
        return false;
    }
    const aiAnimation* a = raw->mAnimations[0];
    const float tps = a->mTicksPerSecond != 0.0 ? static_cast<float>(a->mTicksPerSecond) : 25.0f;
    const float dur = static_cast<float>(a->mDuration) / tps;

    Clip c;
    c.anim = a;
    c.durationSec = dur;
    c.owner = std::shared_ptr<const aiScene>(raw, ReleaseAIScene);
    clips_[name] = std::move(c);
    return true;
}

bool SkinnedModel::AddEmbeddedClip(const std::string& name, unsigned animIndex)
{
    if (!meshScene_ || animIndex >= meshScene_->mNumAnimations)
        return false;
    const aiAnimation* a = meshScene_->mAnimations[animIndex];
    const float tps = a->mTicksPerSecond != 0.0 ? static_cast<float>(a->mTicksPerSecond) : 25.0f;
    Clip c;
    c.anim = a;
    c.durationSec = static_cast<float>(a->mDuration) / tps;
    c.owner.reset();
    clips_[name] = std::move(c);
    return true;
}

float SkinnedModel::ClipDurationSec(const std::string& clipName) const
{
    auto it = clips_.find(clipName);
    if (it == clips_.end())
        return 0.0f;
    return it->second.durationSec;
}

const aiNodeAnim* SkinnedModel::FindNodeAnim(const aiAnimation* anim, const std::string& nodeName) const
{
    if (!anim)
        return nullptr;
    for (unsigned i = 0; i < anim->mNumChannels; ++i)
    {
        if (std::string(anim->mChannels[i]->mNodeName.C_Str()) == nodeName)
            return anim->mChannels[i];
    }
    return nullptr;
}

void SkinnedModel::ReadHierarchy(float timeTicks,
                                 const aiAnimation* anim,
                                 const aiNode* node,
                                 const glm::mat4& parent,
                                 std::vector<glm::mat4>& out,
                                 bool freezeRootMotion)
{
    const std::string name(node->mName.C_Str());
    const aiNodeAnim* nodeAnim = FindNodeAnim(anim, name);

    glm::mat4 nodeTransform = AiToGlm(node->mTransformation);
    if (nodeAnim)
    {
        glm::vec3 pos = InterpolatePosition(timeTicks, nodeAnim);
        const glm::quat rot = InterpolateRotation(timeTicks, nodeAnim);
        const glm::vec3 scl = InterpolateScale(timeTicks, nodeAnim);
        // Root-motion runs: Hips translation moves the model each frame; at loop wrap (fmod)
        // the position snaps -> “teleport”. Keep bind-pose translation for hips = in-place run.
        if (freezeRootMotion && IsHipsOrPelvisNode(name))
        {
            const glm::mat4 bindLocal = AiToGlm(node->mTransformation);
            pos = glm::vec3(bindLocal[3][0], bindLocal[3][1], bindLocal[3][2]);
        }
        const glm::mat4 t = glm::translate(glm::mat4(1.0f), pos);
        const glm::mat4 r = glm::mat4_cast(rot);
        const glm::mat4 s = glm::scale(glm::mat4(1.0f), scl);
        nodeTransform = t * r * s;
    }

    const glm::mat4 global = parent * nodeTransform;

    auto bit = bones_.find(name);
    if (bit != bones_.end())
    {
        const int bid = bit->second.id;
        if (bid >= 0 && bid < kMaxBones)
            out[static_cast<size_t>(bid)] =
                globalInverse_ * global * bit->second.offset;
    }

    for (unsigned i = 0; i < node->mNumChildren; ++i)
        ReadHierarchy(timeTicks, anim, node->mChildren[i], global, out, freezeRootMotion);
}

void SkinnedModel::ComputeBoneMatrices(const std::string& clipName, float timeSec, std::vector<glm::mat4>& out)
{
    out.assign(static_cast<size_t>(kMaxBones), glm::mat4(1.0f));
    if (!meshScene_)
        return;

    auto it = clips_.find(clipName);
    if (it == clips_.end())
    {
        ReadHierarchy(0.0f, nullptr, meshScene_->mRootNode, glm::mat4(1.0f), out, false);
        return;
    }

    const aiAnimation* anim = it->second.anim;
    const float tps =
        anim->mTicksPerSecond != 0.0 ? static_cast<float>(anim->mTicksPerSecond) : 25.0f;
    const float dur = anim->mDuration;
    float ticks = timeSec * tps;
    if (dur > 1e-6)
        ticks = std::fmod(ticks, dur);

    const bool freezeRootMotion = (clipName == "run");
    ReadHierarchy(ticks, anim, meshScene_->mRootNode, glm::mat4(1.0f), out, freezeRootMotion);
}

int SkinnedModel::AcquireBoneId(const std::string& name, const glm::mat4& offset)
{
    auto it = bones_.find(name);
    if (it != bones_.end())
        return it->second.id;

    if (boneCount_ >= kMaxBones)
        return 0;

    const int id = boneCount_++;
    bones_[name] = BoneInfo{id, offset};
    return id;
}

void SkinnedModel::ProcessMesh(const aiScene* scene, aiMesh* mesh)
{
    std::vector<SkinnedVertex> verts(mesh->mNumVertices);
    for (unsigned i = 0; i < mesh->mNumVertices; ++i)
    {
        verts[i].boneIds = glm::ivec4(0, 0, 0, 0);
        verts[i].weights = glm::vec4(0, 0, 0, 0);
        verts[i].uv = glm::vec2(0, 0);
        const aiVector3D& p = mesh->mVertices[i];
        verts[i].pos = glm::vec3(p.x, p.y, p.z);
        if (mesh->HasNormals())
        {
            const aiVector3D& n = mesh->mNormals[i];
            verts[i].norm = glm::vec3(n.x, n.y, n.z);
        }
        if (mesh->mTextureCoords[0])
        {
            const aiVector3D& uv = mesh->mTextureCoords[0][i];
            verts[i].uv = glm::vec2(uv.x, uv.y);
        }
    }

    for (unsigned b = 0; b < mesh->mNumBones; ++b)
    {
        aiBone* bone = mesh->mBones[b];
        const std::string bname(bone->mName.C_Str());
        const glm::mat4 offset = AiToGlm(bone->mOffsetMatrix);
        const int boneId = AcquireBoneId(bname, offset);

        for (unsigned w = 0; w < bone->mNumWeights; ++w)
        {
            const unsigned vid = bone->mWeights[w].mVertexId;
            const float wt = bone->mWeights[w].mWeight;
            if (vid < verts.size())
                AddBoneWeight(verts[vid], boneId, wt);
        }
    }

    for (auto& v : verts)
    {
        float sum = v.weights[0] + v.weights[1] + v.weights[2] + v.weights[3];
        if (sum > 1e-6f)
        {
            v.weights /= sum;
        }
        else
        {
            v.boneIds = glm::ivec4(0, 0, 0, 0);
            v.weights = glm::vec4(1, 0, 0, 0);
        }
    }

    std::vector<unsigned> indices;
    for (unsigned f = 0; f < mesh->mNumFaces; ++f)
    {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    MeshGl gl{};
    gl.indexCount = static_cast<unsigned>(indices.size());
    if (mesh->mMaterialIndex < scene->mNumMaterials)
    {
        const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        gl.diffuseTex = LoadDiffuseTexture(scene, mat, modelDir_);
        gl.hasDiffuse = gl.diffuseTex != 0;
    }

    glGenVertexArrays(1, &gl.vao);
    glGenBuffers(1, &gl.vbo);
    glGenBuffers(1, &gl.ebo);

    glBindVertexArray(gl.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(SkinnedVertex)),
                 verts.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned)),
                 indices.data(),
                 GL_STATIC_DRAW);

    const GLsizei stride = static_cast<GLsizei>(sizeof(SkinnedVertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertex, norm));
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 4, GL_INT, stride, (void*)offsetof(SkinnedVertex, boneIds));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertex, weights));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertex, uv));

    glBindVertexArray(0);
    meshes_.push_back(gl);
}

void SkinnedModel::ProcessNode(const aiScene* scene, aiNode* node)
{
    for (unsigned i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(scene, mesh);
    }
    for (unsigned i = 0; i < node->mNumChildren; ++i)
        ProcessNode(scene, node->mChildren[i]);
}

void SkinnedModel::Draw(GLint uniformUseTexture, GLint uniformDiffuse) const
{
    for (const MeshGl& m : meshes_)
    {
        if (uniformUseTexture >= 0)
        {
            if (m.hasDiffuse && uniformDiffuse >= 0)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m.diffuseTex);
                glUniform1i(uniformDiffuse, 0);
                glUniform1i(uniformUseTexture, 1);
            }
            else
                glUniform1i(uniformUseTexture, 0);
        }
        glBindVertexArray(m.vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m.indexCount), GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void SkinnedModel::Shutdown()
{
    for (MeshGl& m : meshes_)
    {
        if (m.diffuseTex)
            glDeleteTextures(1, &m.diffuseTex);
        if (m.ebo)
            glDeleteBuffers(1, &m.ebo);
        if (m.vbo)
            glDeleteBuffers(1, &m.vbo);
        if (m.vao)
            glDeleteVertexArrays(1, &m.vao);
        m = MeshGl{};
    }
    meshes_.clear();
    bones_.clear();
    boneCount_ = 0;
    meshScene_.reset();
    modelDir_.clear();
    clips_.clear();
}