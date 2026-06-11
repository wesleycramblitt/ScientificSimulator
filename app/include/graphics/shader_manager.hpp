#pragma once
#include <string>
#include <unordered_map>
#include <glad/gl.h>

namespace exd {
namespace graphics {

class ShaderManager {
public:
    struct Program {
        GLuint id = 0;
        std::string vertex_path;
        std::string fragment_path;
        uint64_t last_write_stamp = 0; // optional hot reload
    };

    ~ShaderManager();

    // Load/cached by a logical name, e.g. "mesh_pbr" or "mesh_basic"
    GLuint getOrLoad(const std::string& name,
                     const std::string& vertex_path,
                     const std::string& fragment_path);

    // Convenience
    void destroyAll();

private:
    std::unordered_map<std::string, Program> programs_;

    static std::string readTextFile(const std::string& path);
    static GLuint compileShader(GLenum stage, const std::string& source, const std::string& debug_name);
    static GLuint linkProgram(GLuint vs, GLuint fs, const std::string& debug_name);

    static uint64_t getLastWriteStamp(const std::string& path);
    void reloadProgram(const std::string& name, Program& p);
};

} // namespace graphics
} // namespace exd
