#include "graphics/shader_manager.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>


ShaderManager::~ShaderManager() {
    destroyAll();
}

void ShaderManager::destroyAll() {
    for (auto& [name, p] : programs_) {
        if (p.id != 0) {
            glDeleteProgram(p.id);
            p.id = 0;
        }
    }
    programs_.clear();
}

GLuint ShaderManager::getOrLoad(const std::string& name,
                                const std::string& vertex_path,
                                const std::string& fragment_path) {
    auto it = programs_.find(name);
    if (it != programs_.end()) return it->second.id;

    Program p;
    p.vertex_path = vertex_path;
    p.fragment_path = fragment_path;

    reloadProgram(name, p);
    programs_.emplace(name, p);
    return p.id;
}


void ShaderManager::reloadProgram(const std::string& name, Program& p) {
    const std::string vs_src = readTextFile(p.vertex_path);
    const std::string fs_src = readTextFile(p.fragment_path);

    GLuint vs = 0, fs = 0, prog = 0;
    try {
        vs = compileShader(GL_VERTEX_SHADER,   vs_src, name + " (VS)");
        fs = compileShader(GL_FRAGMENT_SHADER, fs_src, name + " (FS)");
        prog = linkProgram(vs, fs, name);

        // Swap in program atomically-ish
        if (p.id != 0) glDeleteProgram(p.id);
        p.id = prog;

        glDeleteShader(vs);
        glDeleteShader(fs);
    } catch (...) {
        if (prog) glDeleteProgram(prog);
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        throw;
    }
}

std::string ShaderManager::readTextFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint ShaderManager::compileShader(GLenum stage, const std::string& source, const std::string& debug_name) {
    GLuint sh = glCreateShader(stage);
    const char* src = source.c_str();
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);

    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(sh, len, nullptr, log.data());
        glDeleteShader(sh);
        throw std::runtime_error("Shader compile failed: " + debug_name + "\n" + log);
    }
    return sh;
}

GLuint ShaderManager::linkProgram(GLuint vs, GLuint fs, const std::string& debug_name) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(p, len, nullptr, log.data());
        glDeleteProgram(p);
        throw std::runtime_error("Program link failed: " + debug_name + "\n" + log);
    }
    return p;
}

