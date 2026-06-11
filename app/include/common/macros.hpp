
#pragma once
#include <iostream>
#include <glad/gl.h>

namespace exd {
namespace common {

#ifdef DEBUG
    #define GL_CALL(stmt) do {                                  \
        while (glGetError() != GL_NO_ERROR) {}                  \
        stmt;                                                   \
        GLenum err = glGetError();                              \
        if (err != GL_NO_ERROR) {                               \
            std::cerr << "GL error " << err                     \
                      << " at " << __FILE__ << ":" << __LINE__  \
                      << " -> " << #stmt << "\n";               \
        }                                                       \
    } while(0)
#else
    #define GL_CALL(stmt) stmt
#endif

} // namespace common
} // namespace exd
