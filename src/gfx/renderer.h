#pragma once
#include <string>
#include <vector>
#include <cstdint> // FIX: Adds standard types like uint32_t

struct Vertex {
    float x, y, z;
    float r, g, b;
};

struct RobotMesh {
#if defined(__WII__)
    Vertex* vertices;
    uint32_t vertexCount; // FIX: Changed from u32 to uint32_t
#else
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
#endif
};

// --- Pipeline Control Core Functions ---
void InitGraphicsBackend();
void DeinitGraphicsBackend();

#if defined(__WII__)
    #include <gccore.h>
    Mtx* StartRenderFrame();
    void RenderRobotChassis(const RobotMesh& mesh, float fwd, float strafe, float rcw, Mtx* viewMtx);
#else
    void StartRenderFrame();
    void RenderRobotChassis(const RobotMesh& mesh, float fwd, float strafe, float rcw);
#endif

void RenderFRCField();
void EndRenderFrame();
RobotMesh LoadRobotMesh(const std::string& filepath);