#if defined(__WII__)
#include "../robot_physics.h"
#include "renderer.h"
#include <ogc/gu.h>
#include <cmath>
#include <cstring>
#include <gccore.h>
#include <malloc.h>

extern "C" {
void SYS_Report(const char *format, ...);
}

#define FIFO_SIZE (256 * 1024)
static void *frameBuffer = NULL;
static GXRModeObj *rmode = NULL;
static Mtx s_activeCameraViewMtx;

void InitGraphicsBackend() {
  SYS_Report("--- BREWSIM WII INITIALIZATION START ---\n");
  VIDEO_Init();
  rmode = VIDEO_GetPreferredMode(NULL);

  frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

  VIDEO_Configure(rmode);
  VIDEO_SetNextFramebuffer(frameBuffer);
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();
  VIDEO_WaitVSync();

  void *gp_fifo = memalign(32, FIFO_SIZE);
  memset(gp_fifo, 0, FIFO_SIZE);
  GX_Init(gp_fifo, FIFO_SIZE);

  GXColor background = {0x26, 0x26, 0x26, 0xFF};
  GX_SetCopyClear(background, GX_MAX_Z24);

  GX_SetViewport(0, 0, rmode->fbWidth, rmode->xfbHeight, 0, 1);
  GX_SetScissor(0, 0, rmode->fbWidth, rmode->xfbHeight);
  GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->xfbHeight);
  GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
  GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);

  GX_SetFieldMode(rmode->xfbMode, ((rmode->viHeight == rmode->viTVMode) ? GX_FALSE : GX_TRUE));

  GX_ClearVtxDesc();
  GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
  GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

  GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
  GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

  GX_SetNumChans(1);
  GX_SetChanCtrl(GX_COLOR0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
  GX_SetNumTexGens(0);
  GX_SetNumTevStages(1);
  GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
  GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);

  Mtx44 perspective;
  guPerspective(perspective, 60.0f, (float)rmode->fbWidth / (float)rmode->xfbHeight, 0.1f, 1000.0f);
  GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

  GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
  GX_SetCullMode(GX_CULL_NONE);
}

Mtx* StartRenderFrame() {
  GX_SetViewport(0, 0, rmode->fbWidth, rmode->xfbHeight, 0, 1);

  guVector cam_pos = { 0.0f, 20.0f, 35.0f };
  guVector cam_up  = { 0.0f, 1.0f, 0.0f };
  guVector cam_look = { 0.0f, 0.0f, 0.0f };
  guLookAt(s_activeCameraViewMtx, &cam_pos, &cam_up, &cam_look);
  
  return &s_activeCameraViewMtx;
}

void RenderFRCField() {
  GX_LoadPosMtxImm(s_activeCameraViewMtx, GX_PNMTX0);

  GX_Begin(GX_LINES, GX_VTXFMT0, 8);
  GX_Position3f32(-15.0f, 0.0f, -15.0f); GX_Color4u8(76, 76, 76, 255);
  GX_Position3f32(15.0f, 0.0f, -15.0f);  GX_Color4u8(76, 76, 76, 255);

  GX_Position3f32(15.0f, 0.0f, -15.0f);  GX_Color4u8(76, 76, 76, 255);
  GX_Position3f32(15.0f, 0.0f, 15.0f);   GX_Color4u8(76, 76, 76, 255);

  GX_Position3f32(15.0f, 0.0f, 15.0f);   GX_Color4u8(76, 76, 76, 255);
  GX_Position3f32(-15.0f, 0.0f, 15.0f);  GX_Color4u8(76, 76, 76, 255);

  GX_Position3f32(-15.0f, 0.0f, 15.0f);  GX_Color4u8(76, 76, 76, 255);
  GX_Position3f32(-15.0f, 0.0f, -15.0f); GX_Color4u8(76, 76, 76, 255);
  GX_End();
}

void RenderGameBall(const BallPhysicsState& ball) {
    Mtx model, modelView;
    guMtxTrans(model, ball.position.x, ball.radius, -ball.position.y);
    guMtxConcat(s_activeCameraViewMtx, model, modelView);
    GX_LoadPosMtxImm(modelView, GX_PNMTX0);

    float r = ball.radius;
    
    // Draw a bright yellow wireframe diamond shape on the Wii screen
    GX_Begin(GX_LINES, GX_VTXFMT0, 24);
    
    // Top Cone
    GX_Position3f32(0, r, 0);  GX_Color4u8(255, 255, 0, 255); GX_Position3f32(r, 0, 0);  GX_Color4u8(255, 255, 0, 255);
    GX_Position3f32(0, r, 0);  GX_Color4u8(255, 255, 0, 255); GX_Position3f32(-r, 0, 0); GX_Color4u8(255, 255, 0, 255);
    GX_Position3f32(0, r, 0);  GX_Color4u8(255, 255, 0, 255); GX_Position3f32(0, 0, r);  GX_Color4u8(255, 255, 0, 255);
    GX_Position3f32(0, r, 0);  GX_Color4u8(255, 255, 0, 255); GX_Position3f32(0, 0, -r); GX_Color4u8(255, 255, 0, 255);
    
    // Rim
    GX_Position3f32(r, 0, 0);  GX_Color4u8(255, 255, 0, 255); GX_Position3f32(0, 0, r);  GX_Color4u8(255, 255, 0, 255);
    GX_Position3f32(0, 0, r);  GX_Color4u8(255, 255, 0, 255); GX_Position3f32(-r, 0, 0); GX_Color4u8(255, 255, 0, 255);
    GX_Position3f32(-r, 0, 0); GX_Color4u8(255, 255, 0, 255); GX_Position3f32(0, 0, -r); GX_Color4u8(255, 255, 0, 255);
    GX_Position3f32(0, 0, -r); GX_Color4u8(255, 255, 0, 255); GX_Position3f32(r, 0, 0);  GX_Color4u8(255, 255, 0, 255);

    // Bottom Cone
    GX_Position3f32(0, -r, 0); GX_Color4u8(255, 255, 0, 255); GX_Position3f32(r, 0, 0);  GX_Color4u8(255, 255, 0, 255);
    GX_Position3f32(0, -r, 0); GX_Color4u8(255, 255, 0, 255); GX_Position3f32(-r, 0, 0); GX_Color4u8(255, 255, 0, 255);
    GX_Position3f32(0, -r, 0); GX_Color4u8(255, 255, 0, 255); GX_Position3f32(0, 0, r);  GX_Color4u8(255, 255, 0, 255);
    GX_Position3f32(0, -r, 0); GX_Color4u8(255, 255, 0, 255); GX_Position3f32(0, 0, -r); GX_Color4u8(255, 255, 0, 255);
    GX_End();
}

void RenderRobotChassis(const RobotMesh& mesh, const RobotPhysicsState& state, Mtx* cameraViewMtx) {
    Mtx model, modelView;
    Mtx trans, rot;

    guMtxTrans(trans, state.position.x, 0.4f, -state.position.y);
    float headingDegrees = -state.heading_rad * 180.0f / M_PI;
    guMtxRotDeg(rot, 'Y', headingDegrees);

    guMtxConcat(trans, rot, model);
    guMtxConcat(*cameraViewMtx, model, modelView);
    GX_LoadPosMtxImm(modelView, GX_PNMTX0);

    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, mesh.vertexCount);
    for (unsigned int i = 0; i < mesh.vertexCount; ++i) {
        const Vertex& v = mesh.vertices[i];
        // Applying the scale multiplier directly inside the GX buffer pipeline
        GX_Position3f32(v.x * 5.0f, v.y * 5.0f, v.z * 5.0f); 
        GX_Color4u8(static_cast<u8>(v.r * 255), static_cast<u8>(v.g * 255), static_cast<u8>(v.b * 255), 255);
    }
    GX_End();
}

void EndRenderFrame() {
  GX_DrawDone();
  GX_CopyDisp(frameBuffer, GX_TRUE);
  GX_Flush();
  VIDEO_WaitVSync();
}

void DeinitGraphicsBackend() {}
#endif
