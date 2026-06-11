#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

#include "assets/file_importer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "graphics/vertex.hpp"
#include "graphics/mesh.hpp"

namespace exd {
namespace assets {

static void ProcessNodeMerge(
    const aiNode* node,
    const aiScene* scene,
    std::vector<graphics::Vertex>& outVertices,
    std::vector<uint32_t>& outIndices
) {
    // For each mesh referenced by this node:
    for (unsigned i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh* m = scene->mMeshes[node->mMeshes[i]];

        // Assimp returns indices local to this aiMesh, so we offset them when merging.
        const uint32_t baseVertex = static_cast<uint32_t>(outVertices.size());

        // Vertices
        outVertices.reserve(outVertices.size() + m->mNumVertices);
        for (unsigned v = 0; v < m->mNumVertices; ++v) {
            graphics::Vertex vert{};
            vert.position = math::Vec3{
                m->mVertices[v].x,
                m->mVertices[v].y,
                m->mVertices[v].z
            };

            if (m->HasNormals()) {
                vert.normal = math::Vec3{
                    m->mNormals[v].x,
                    m->mNormals[v].y,
                    m->mNormals[v].z
                };
            } else {
                // If we didn't request generated normals, keep your default.
                vert.normal = math::Vec3{0.0f, 0.0f, 1.0f};
            }

            outVertices.push_back(vert);
        }

        // Faces -> indices (after aiProcess_Triangulate, every face should be 3 indices)
        // But we still guard for safety.
        for (unsigned f = 0; f < m->mNumFaces; ++f) {
            const aiFace& face = m->mFaces[f];
            if (face.mNumIndices < 3) continue;

            // If triangulated, this is exactly 3. If not, you can fan-triangulate.
            if (face.mNumIndices == 3) {
                outIndices.push_back(baseVertex + face.mIndices[0]);
                outIndices.push_back(baseVertex + face.mIndices[1]);
                outIndices.push_back(baseVertex + face.mIndices[2]);
            } else {
                // Fan triangulation (rare if Triangulate flag is set, but safe)
                for (unsigned k = 1; k + 1 < face.mNumIndices; ++k) {
                    outIndices.push_back(baseVertex + face.mIndices[0]);
                    outIndices.push_back(baseVertex + face.mIndices[k]);
                    outIndices.push_back(baseVertex + face.mIndices[k + 1]);
                }
            }
        }
    }

    // Recurse children
    for (unsigned c = 0; c < node->mNumChildren; ++c) {
        ProcessNodeMerge(node->mChildren[c], scene, outVertices, outIndices);
    }
}

graphics::Mesh FileImporter::loadMeshWithAssimp(const std::string& path) {
    Assimp::Importer importer;

    // Good defaults for your Mesh:
    // - Triangulate ensures index triples
    // - GenSmoothNormals ensures normals exist (unless you want hard edges: GenNormals)
    // - JoinIdenticalVertices helps reduce duplicates
    // - ImproveCacheLocality can help post-transform cache
    const unsigned flags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality;

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        throw std::runtime_error(std::string("Assimp load failed: ") + importer.GetErrorString());
    }

    graphics::Mesh mesh;
    mesh.topology = graphics::TRIANGLES;

    ProcessNodeMerge(scene->mRootNode, scene, mesh.vertices, mesh.indices);

    // Optional sanity checks
    if (mesh.indices.size() % 3 != 0) {
        std::cerr << "Warning: index count not multiple of 3 ("
                  << mesh.indices.size() << "), check topology/triangulation.\n";
    }

    // Guard: if you ever expect very large models, you must ensure vertex count fits uint32_t.
    if (mesh.vertices.size() > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("Mesh too large for uint32 indices.");
    }

    return mesh;
}

} // namespace assets
} // namespace exd
