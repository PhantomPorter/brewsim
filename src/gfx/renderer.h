#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Vertex {
    float x, y, z;
    float r, g, b;
};

struct RobotMesh {
#if defined(__WII__)
    Vertex* vertices;
    uint32_t vertexCount;
#else
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
#endif
};

void InitGraphicsBackend();
void DeinitGraphicsBackend();
void RenderFRCField();
void EndRenderFrame();
RobotMesh LoadRobotMesh(const std::string& filepath);

// --- STRICT CROSS-PLATFORM DISPATCH BLOCKS ---
#if defined(__WII__)
    #include <gccore.h>
    Mtx* StartRenderFrame();
    void RenderRobotChassis(const RobotMesh& mesh, float fwd, float strafe, float rcw, Mtx* viewMtx);

#elif defined(__WIIU__)
    void* StartRenderFrame(); 
    void RenderRobotChassis(const RobotMesh& mesh, float fwd, float strafe, float rcw, void* context = nullptr);

#else // PC Desktop & fallback targets
    void StartRenderFrame();
    void RenderRobotChassis(const RobotMesh& mesh, float fwd, float strafe, float rcw);
#endif