#if defined(__WIIU__)
#include "renderer.h"
#include <coreinit/screen.h>

#include <gx2/clear.h>
#include <gx2/context.h>
#include <gx2/display.h>
#include <gx2/draw.h>
#include <gx2/enum.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/state.h>
#include <gx2/surface.h>
#include <gx2/swap.h>
#include <gx2/utils.h>
#include <gx2/event.h>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <malloc.h>
#include <cstring>

float g_robotX = 0.0f;
float g_robotY = 0.0f;
float g_robotHeadingRad = 0.0f;

static void* s_commandBuffer = nullptr;
static GX2ColorBuffer s_tvColorBuffer;
static GX2DepthBuffer s_tvDepthBuffer;
static void* s_tvColorBufferScanout = nullptr;
static void* s_tvDepthBufferScanout = nullptr;

static float s_wiiuProjection[16];
static float s_wiiuView[16];

void MtxIdentity(float* m) {
    m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f; m[3] = 0.0f;
    m[4] = 0.0f; m[5] = 1.0f; m[6] = 0.0f; m[7] = 0.0f;
    m[8] = 0.0f; m[9] = 0.0f; m[10] = 1.0f; m[11] = 0.0f;
    m[12] = 0.0f; m[13] = 0.0f; m[14] = 0.0f; m[15] = 1.0f;
}

void MtxPerspective(float* m, float fovyDeg, float aspect, float nearZ, float farZ) {
    float radians = (fovyDeg / 2.0f) * (3.14159265f / 180.0f);
    float cotangent = std::cos(radians) / std::sin(radians);
    
    std::fill_n(m, 16, 0.0f);
    m[0] = cotangent / aspect;
    m[5] = cotangent;
    m[10] = farZ / (nearZ - farZ);
    m[11] = -1.0f;
    m[14] = (nearZ * farZ) / (nearZ - farZ);
}

void InitGraphicsBackend() {
    OSScreenInit();
    
    s_commandBuffer = memalign(GX2_COMMAND_BUFFER_ALIGNMENT, 0x40000);
    
    uint32_t initAttributes[] = {
        GX2_INIT_CMD_BUF_BASE, (uintptr_t)s_commandBuffer,
        GX2_INIT_CMD_BUF_POOL_SIZE, 0x40000,
        GX2_INIT_END
    };
    GX2Init(initAttributes);

    // Color surface layout configuration
    s_tvColorBuffer.surface.width = 1280;
    s_tvColorBuffer.surface.height = 720;
    s_tvColorBuffer.surface.format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
    s_tvColorBuffer.surface.depth = 1;
    s_tvColorBuffer.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    s_tvColorBuffer.surface.mipLevels = 1;
    s_tvColorBuffer.surface.use = GX2_SURFACE_USE_COLOR_BUFFER;

    GX2CalcSurfaceSizeAndAlignment(&s_tvColorBuffer.surface);
    s_tvColorBufferScanout = memalign(s_tvColorBuffer.surface.alignment, s_tvColorBuffer.surface.imageSize);
    s_tvColorBuffer.surface.image = s_tvColorBufferScanout;

    // Depth surface layout configuration
    s_tvDepthBuffer.surface.width = 1280;
    s_tvDepthBuffer.surface.height = 720;
    s_tvDepthBuffer.surface.format = GX2_SURFACE_FORMAT_FLOAT_D24_S8;
    s_tvDepthBuffer.surface.depth = 1;
    s_tvDepthBuffer.surface.dim = GX2_SURFACE_DIM_TEXTURE_2D;
    s_tvDepthBuffer.surface.mipLevels = 1;
    s_tvDepthBuffer.surface.use = GX2_SURFACE_USE_DEPTH_BUFFER;

    GX2CalcSurfaceSizeAndAlignment(&s_tvDepthBuffer.surface);
    s_tvDepthBufferScanout = memalign(s_tvDepthBuffer.surface.alignment, s_tvDepthBuffer.surface.imageSize);
    s_tvDepthBuffer.surface.image = s_tvDepthBufferScanout;

    GX2InitColorBufferRegs(&s_tvColorBuffer);
    GX2InitDepthBufferRegs(&s_tvDepthBuffer);

    MtxPerspective(s_wiiuProjection, 45.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
}

void* StartRenderFrame() {
    GX2SetContextState(nullptr);
    GX2SetColorBuffer(&s_tvColorBuffer, GX2_RENDER_TARGET_0);
    GX2SetDepthBuffer(&s_tvDepthBuffer);
    GX2SetViewport(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f);
    GX2SetScissor(0, 0, 1280, 720);

    GX2ClearColor(&s_tvColorBuffer, 0.15f, 0.15f, 0.15f, 1.0f);
    
    GX2ClearDepthStencilEx(&s_tvDepthBuffer, 1.0f, 0, GX2_CLEAR_FLAGS_DEPTH);
    
    return nullptr;
}

void RenderFRCField() {}

void RenderRobotChassis(const RobotMesh& mesh, float fwd, float strafe, float rcw, void* context) {
    float dt = 0.016f;
    float linearSpeedMultiplier = 8.0f;  
    float angularSpeedMultiplier = 5.0f; 

    g_robotX += strafe * linearSpeedMultiplier * dt;
    g_robotY += fwd * linearSpeedMultiplier * dt;
    g_robotHeadingRad += rcw * angularSpeedMultiplier * dt;

    float modelMtx[16];
    MtxIdentity(modelMtx);
    
    float cosH = std::cos(-g_robotHeadingRad);
    float sinH = std::sin(-g_robotHeadingRad);
    modelMtx[0] = cosH;
    modelMtx[2] = -sinH;
    modelMtx[8] = sinH;
    modelMtx[10] = cosH;

    modelMtx[12] = g_robotX;
    modelMtx[13] = 0.4f;
    modelMtx[14] = -g_robotY;

    float s = 5.0f;
    modelMtx[0] *= s; modelMtx[5] *= s; modelMtx[10] *= s;

    #if !defined(__WII__)
    if (mesh.vertices.empty()) return;

    GX2SetAttribBuffer(0, mesh.vertices.size() * sizeof(Vertex), sizeof(Vertex), mesh.vertices.data());
    
    GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLES, mesh.vertices.size(), 0, 1); 
    #endif
}

void EndRenderFrame() {
    // Copy out the backbuffer contents to the TV hardware output scan target
    GX2CopyColorBufferToScanBuffer(&s_tvColorBuffer, GX2_SCAN_TARGET_TV);
    
    // Flip the display buffers
    GX2SwapScanBuffers();
    
    // Flush the command queue out to the GPU execution rings
    GX2Flush();
    
    GX2DrawDone();
}

void DeinitGraphicsBackend() {
    GX2Shutdown();
    
    if (s_tvColorBufferScanout) free(s_tvColorBufferScanout);
    if (s_tvDepthBufferScanout) free(s_tvDepthBufferScanout);
    if (s_commandBuffer) free(s_commandBuffer);
}
#endif