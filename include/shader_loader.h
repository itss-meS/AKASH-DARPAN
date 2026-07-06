#pragma once
// ============================================================
//  AKASH-DARPAN  —  shader_loader.h
//  Load, compile and link GLSL shaders from disk at runtime.
//  Supports hot-reload: call reload() to recompile on the fly.
// ============================================================
#include <string>
#include <unordered_map>
#include <cstdio>
#include <glad/glad.h>

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram() { if (id_) glDeleteProgram(id_); }

    // Load from two files; returns false and prints error on failure
    bool load(const std::string& vertPath, const std::string& fragPath) {
        vertPath_ = vertPath;
        fragPath_ = fragPath;
        return compile();
    }

    // Hot-reload from same paths
    bool reload() { return compile(); }

    void use() const { glUseProgram(id_); }

    // Uniform setters
    void setInt  (const char* name, int v)              const { glUniform1i (loc(name), v); }
    void setFloat(const char* name, float v)             const { glUniform1f (loc(name), v); }
    void setVec3 (const char* name, float x,float y,float z) const { glUniform3f(loc(name),x,y,z); }
    void setMat4 (const char* name, const float* m)     const { glUniformMatrix4fv(loc(name),1,GL_FALSE,m); }

    unsigned int id() const { return id_; }
    bool valid()      const { return id_ != 0; }

private:
    unsigned int id_   = 0;
    std::string  vertPath_, fragPath_;

    GLint loc(const char* name) const { return glGetUniformLocation(id_, name); }

    static std::string readFile(const std::string& path) {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            fprintf(stderr, "[Shader] Cannot open: %s\n", path.c_str());
            return "";
        }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        std::string out(sz, '\0');
        fread(out.data(), 1, sz, f);
        fclose(f);
        return out;
    }

    static unsigned int compileStage(GLenum type, const std::string& src,
                                      const std::string& path) {
        const char* c = src.c_str();
        unsigned int s = glCreateShader(type);
        glShaderSource(s, 1, &c, nullptr);
        glCompileShader(s);
        int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(s, 1024, nullptr, log);
            fprintf(stderr, "[Shader] Compile error in %s:\n%s\n",
                    path.c_str(), log);
            glDeleteShader(s);
            return 0;
        }
        return s;
    }

    bool compile() {
        std::string vSrc = readFile(vertPath_);
        std::string fSrc = readFile(fragPath_);
        if (vSrc.empty() || fSrc.empty()) return false;

        unsigned int vs = compileStage(GL_VERTEX_SHADER,   vSrc, vertPath_);
        unsigned int fs = compileStage(GL_FRAGMENT_SHADER, fSrc, fragPath_);
        if (!vs || !fs) { glDeleteShader(vs); glDeleteShader(fs); return false; }

        unsigned int prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);
        glDeleteShader(vs);
        glDeleteShader(fs);

        int ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetProgramInfoLog(prog, 1024, nullptr, log);
            fprintf(stderr, "[Shader] Link error (%s + %s):\n%s\n",
                    vertPath_.c_str(), fragPath_.c_str(), log);
            glDeleteProgram(prog);
            return false;
        }

        if (id_) glDeleteProgram(id_);
        id_ = prog;
        printf("[Shader] Loaded: %s + %s\n",
               vertPath_.c_str(), fragPath_.c_str());
        return true;
    }
};

// ── Shader cache (singleton) ───────────────────────────────
class ShaderCache {
public:
    static ShaderCache& instance() {
        static ShaderCache s;
        return s;
    }

    // Load or retrieve cached program
    ShaderProgram* get(const std::string& name,
                        const std::string& vertPath,
                        const std::string& fragPath) {
        auto it = cache_.find(name);
        if (it != cache_.end()) return it->second.get();
        auto prog = std::make_unique<ShaderProgram>();
        if (!prog->load(vertPath, fragPath)) return nullptr;
        cache_[name] = std::move(prog);
        return cache_[name].get();
    }

    // Hot-reload all shaders (call on F5)
    void reloadAll() {
        for (auto& [name, prog] : cache_) {
            if (prog->reload())
                printf("[ShaderCache] Reloaded: %s\n", name.c_str());
        }
    }

    void clear() { cache_.clear(); }

private:
    ShaderCache() = default;
    std::unordered_map<std::string, std::unique_ptr<ShaderProgram>> cache_;
};
