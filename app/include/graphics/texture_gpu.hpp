#pragma once
#include <string>
#include <cstdint>
#include <glad/gl.h>
#include "common/macros.hpp"
#include "components/cubemap.hpp"
#include <stdexcept>
#include "stb_image.h"


struct TextureGPU {
    TextureGPU(CubeMap cubemap) {
        glGenTextures(1, &id);
        GL_CALL(glActiveTexture(GL_TEXTURE0));
        GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, id)); 
        
        for (size_t i{}; i < 6; ++i) {
           // load image, stbi_load?
           unsigned char* data = stbi_load(cubemap.faces[i].name.c_str(), 
                                           &cubemap.faces[i].width, 
                                           &cubemap.faces[i].height,          // error handling
                                           &cubemap.faces[i].channels,
                                           0);
           if (!data) {
                glDeleteTextures(1, &id);
                throw std::runtime_error("Failed to load cubemap face: " + cubemap.faces[i].name);
           }
           
           GLenum format = (cubemap.faces[i].channels == 4) ? GL_RGBA : GL_RGB;

           glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
               0,  //cubemap.faces[i].mipLevels,
               format,
               cubemap.faces[i].width,
               cubemap.faces[i].height,
               0,
               format,
               GL_UNSIGNED_BYTE,
               data);

           stbi_image_free(data);
        };

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
        GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

    };
    TextureGPU(const TextureGPU&) = delete;
    TextureGPU& operator=(const TextureGPU&) = delete;

    TextureGPU(TextureGPU&& other) noexcept : id(other.id) {
        other.id = 0;
    }
    TextureGPU& operator=(TextureGPU&& other) noexcept {
        if (this != &other) {
            if (id) glDeleteTextures(1, &id);
            id = other.id;
            other.id = 0;
        }
        return *this;
    }
    ~TextureGPU() {
        glDeleteTextures(1, &id);
        id = 0;
    };
    std::string name;
    GLuint id;

};
