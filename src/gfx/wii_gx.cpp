#if defined(__WII__)
#include "renderer.h"
#include <ogc/gu.h>
#include <cmath>
#include <cstring>
#include <gccore.h>
#include <malloc.h> // Required for safe memory allocations
extern "C" {
void SYS_Report(const char *format, ...);
}
#define FIFO_SIZE (256 * 1024)
static void *frameBuffer = NULL;
static GXRModeObj *rmode = NULL;
static Mtx s_activeCameraViewMtx;
// Synchronized physics tracking hooks
float g_robotX = 0.0f;
float g_robotY = 0.0f;
float g_robotHeadingRad = 0.0f;

void InitGraphicsBackend() {
  SYS_Report("--- BREWSIM WII INITIALIZATION START ---\n");
  VIDEO_Init();
  rmode = VIDEO_GetPreferredMode(NULL);

  // Allocate the real display frame buffer from safe memory locations
  frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

  VIDEO_Configure(rmode);
  VIDEO_SetNextFramebuffer(frameBuffer);
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();
  VIDEO_WaitVSync();

  // FIXED: Allocate the Command FIFO stream block out of the heap with 32-byte
  // alignment This stops it from stepping on video display frame memory!
  void *gp_fifo = memalign(32, FIFO_SIZE);
  memset(gp_fifo, 0, FIFO_SIZE);
  GX_Init(gp_fifo, FIFO_SIZE);

  // Set clear background color to dark arena gray
  GXColor background = {0x26, 0x26, 0x26, 0xFF};
  GX_SetCopyClear(background, GX_MAX_Z24);

  // Viewport hardware mapping configurations
  GX_SetViewport(0, 0, rmode->fbWidth, rmode->xfbHeight, 0, 1);
  GX_SetScissor(0, 0, rmode->fbWidth, rmode->xfbHeight);
  GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->xfbHeight);
  GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
  GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);

  GX_SetFieldMode(rmode->xfbMode,
                  ((rmode->viHeight == rmode->viTVMode) ? GX_FALSE : GX_TRUE));

  GX_ClearVtxDesc();
  GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
  GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

  // Set format matching standard structure mapping specifications
  // Position: 3 Floats (X,Y,Z) -> Expects GX_Position3f32
  GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
  
  // Color: 4 Unsigned Bytes (R,G,B,A) -> Expects GX_Color4u8
  GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

  // Set up basic Material Color Pass shader pipelines
  GX_SetNumChans(1);
  GX_SetChanCtrl(GX_COLOR0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL,
                 GX_DF_NONE, GX_AF_NONE);
  GX_SetNumTexGens(0);
  GX_SetNumTevStages(1);
  GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
  GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);

  // Setup active 3D camera projection bounds
  Mtx44 perspective;
  guPerspective(perspective, 60.0f,
                (float)rmode->fbWidth / (float)rmode->xfbHeight, 0.1f, 1000.0f);
  GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);


  GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
  GX_SetCullMode(GX_CULL_NONE); // Culls interior geometry out of view loops
}

Mtx* StartRenderFrame() {
    #if defined(__WII__)
        // ... (Keep your existing clear screen and viewport settings here)

        // Locate where your camera look-at logic is calculated:
        guVector cam_pos = { 0.0f, 35.0f, 45.0f };
        guVector cam_up  = { 0.0f, 1.0f, 0.0f };
        guVector cam_look = { 0.0f, 0.0f, 0.0f };
        guLookAt(s_activeCameraViewMtx, &cam_pos, &cam_up, &cam_look);
        
        // Return a pointer to this frame's camera alignment matrix
        return &s_activeCameraViewMtx;
    #endif
}

void RenderFRCField() {
  Mtx view;
  // Move camera further back (Z = 35.0f) and higher up (Y = 20.0f) for a clean isometric overview
  guVector cam  = { 0.0f, 20.0f, 35.0f }; 
  guVector up   = { 0.0f, 1.0f,  0.0f };
  
  // Track the center of the arena floor
  guVector look = { 0.0f, 0.0f,  0.0f };
  guLookAt(view, &cam, &up, &look);

  GX_LoadPosMtxImm(view, GX_PNMTX0);

  // FIXED: Changed GX_Color3f32 to GX_Color4u8 to match GX_RGBA8 hardware declaration!
  // Order must match perfectly: Position call first, then Color call.
  GX_Begin(GX_LINES, GX_VTXFMT0, 8);
  
  GX_Position3f32(-15.0f, 0.0f, -15.0f);
  GX_Color4u8(76, 76, 76, 255);
  GX_Position3f32(15.0f, 0.0f, -15.0f);
  GX_Color4u8(76, 76, 76, 255);

  GX_Position3f32(15.0f, 0.0f, -15.0f);
  GX_Color4u8(76, 76, 76, 255);
  GX_Position3f32(15.0f, 0.0f, 15.0f);
  GX_Color4u8(76, 76, 76, 255);

  GX_Position3f32(15.0f, 0.0f, 15.0f);
  GX_Color4u8(76, 76, 76, 255);
  GX_Position3f32(-15.0f, 0.0f, 15.0f);
  GX_Color4u8(76, 76, 76, 255);

  GX_Position3f32(-15.0f, 0.0f, 15.0f);
  GX_Color4u8(76, 76, 76, 255);
  GX_Position3f32(-15.0f, 0.0f, -15.0f);
  GX_Color4u8(76, 76, 76, 255);
  
  GX_End();
}


void RenderRobotChassis(const RobotMesh& mesh, float fwd, float strafe, float rcw, Mtx* viewMtx) {
#if defined(__WII__)
    float dt = 0.016f;
    float linearSpeedMultiplier = 8.0f;  
    float angularSpeedMultiplier = 5.0f; 

    g_robotX += strafe * linearSpeedMultiplier * dt;
    g_robotY += fwd * linearSpeedMultiplier * dt;
    g_robotHeadingRad += rcw * angularSpeedMultiplier * dt; 

    Mtx modelMatrix, translationMatrix, rotationMatrix, scaleMatrix;
    
    guMtxTrans(translationMatrix, g_robotX, 0.4f, -g_robotY);
    guMtxRotRad(rotationMatrix, 'Y', -g_robotHeadingRad);
    
    float assetScale = 5.0f;
    guMtxScale(scaleMatrix, assetScale, assetScale, assetScale);
    
    guMtxConcat(rotationMatrix, scaleMatrix, modelMatrix);
    guMtxConcat(translationMatrix, modelMatrix, modelMatrix);
    
    Mtx modelViewMatrix;
    if (viewMtx != nullptr) {
        guMtxConcat(*viewMtx, modelMatrix, modelViewMatrix);
    } else {
        guMtxCopy(modelMatrix, modelViewMatrix);
    }
    
    GX_LoadPosMtxImm(modelViewMatrix, GX_PNMTX0);

    if (mesh.vertexCount == 0 || mesh.vertices == nullptr) return;
    
    // FIX: Using uint32_t for hardware tracking loop parity
    uint32_t safeVertexCount = (mesh.vertexCount / 3) * 3;

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, safeVertexCount);
    for (uint32_t i = 0; i < safeVertexCount; ++i) {
        const Vertex& v = mesh.vertices[i];
        
        u8 r = static_cast<u8>(v.r >= 1.0f ? 255 : (v.r <= 0.0f ? 0 : v.r * 255.0f));
        u8 g = static_cast<u8>(v.g >= 1.0f ? 255 : (v.g <= 0.0f ? 0 : v.g * 255.0f));
        u8 b = static_cast<u8>(v.b >= 1.0f ? 255 : (v.b <= 0.0f ? 0 : v.b * 255.0f));

        GX_Position3f32(v.x, v.y, v.z);
        GX_Color4u8(r, g, b, 255); 
    }
    GX_End();
#endif
}

void EndRenderFrame() {
  GX_DrawDone();
  GX_CopyDisp(frameBuffer, GX_TRUE);
  GX_Flush();
  VIDEO_SetNextFramebuffer(frameBuffer);
  VIDEO_Flush();
  VIDEO_WaitVSync();
}

void DeinitGraphicsBackend() {}
#endif