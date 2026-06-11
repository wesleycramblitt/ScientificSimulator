#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "graphics/mesh.hpp"

namespace exd {
namespace assets {

class FileImporter {
    public:
        static graphics::Mesh loadMeshWithAssimp(const std::string& path); 
};

} // namespace assets
} // namespace exd

