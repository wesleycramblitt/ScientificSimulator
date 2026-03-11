#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "graphics/mesh.hpp"


class FileImporter {
    public:
        static Mesh loadMeshWithAssimp(const std::string& path); 
};

