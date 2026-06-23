#if defined(__WII__)
#include "renderer.h"
#include <gccore.h>
#include <cstring>
#include <cmath>

#define FIFO_SIZE (256 * 1024)
static void* frameBuffer = NULL;
static GXRModeObj* rmode = NULL;

// Global tracking variables mirrored from your main simulation calculations
float g_robotX = 0.0f;
float g_robotY = 0.0f;
float g_robotHeadingRad = 0.0f; 

void InitGraphicsBackend() {
    VIDEO_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    // Allocate and initialize the hardware GX FIFO pipeline command stream
    void* gp_fifo = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    memset(gp_fifo, 0, FIFO_SIZE);
    GX_Init(gp_fifo, FIFO_SIZE);

    // Set clear background color to dark arena gray
    GXColor background = {0x26, 0x26, 0x26, 0xFF};
    GX_SetCopyClear(background, GX_MAX_Z24);

    // Viewport hardware mapping rules
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->xfbHeight, 0, 1);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->xfbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->xfbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    
    GX_SetFieldMode(rmode->xfbMode, ((rmode->viHeight == rmode->viTVMode) ? GX_FALSE : GX_TRUE));

    // Clear and define active hardware Vertex Attributes (Position & Color)
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    // Set up basic Material Color Pass shader pipelines
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    GX_SetNumTexGens(0);
    GX_SetNumTevStages(1);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);

    Mtx44 perspective;
    guPerspective(perspective, 60.0f, (float)rmode->fbWidth / (float)rmode->xfbHeight, 0.1f, 1000.0f);
    GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);
}

void StartRenderFrame() {
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->xfbHeight, 0, 1);
    GX_InvalidateTexAll();
}

void RenderFRCField() {
    Mtx view;
    guVector cam = {0.0f, 5.0f, 14.0f}; 
    guVector up = {0.0f, 1.0f, 0.0f};
    guVector look = {0.0f, 0.0f, 0.0f};
    guLookAt(view, &cam, &up, &look);
    
    GX_LoadPosMtxImm(view, GX_PNMTX0);

    GX_Begin(GX_LINES, GX_VTXFMT0, 4);
        // Left barrier line
        GX_Position3f32(-10.0f, 0.0f, -10.0f); GX_Color4u8(0x40, 0x40, 0x40, 0xFF);
        GX_Position3f32(-10.0f, 0.0f,  10.0f); GX_Color4u8(0x40, 0x40, 0x40, 0xFF);
        // Right barrier line
        GX_Position3f32( 10.0f, 0.0f, -10.0f); GX_Color4u8(0x40, 0x40, 0x40, 0xFF);
        GX_Position3f32( 10.0f, 0.0f,  10.0f); GX_Color4u8(0x40, 0x40, 0x40, 0xFF);
    GX_End();
}

void RenderRobotChassis(const SwerveDriveStates& swerve) {
    float dt = 0.016f;
    float speedMultiplier = 6.0f;
    float rotationMultiplier = 4.0f;

    extern float g_wiiFwdInput;
    extern float g_wiiStrafeInput;
    extern float g_wiiRotInput;

    g_robotHeadingRad += g_wiiRotInput * rotationMultiplier * dt;
    g_robotX += g_wiiStrafeInput * speedMultiplier * dt;
    g_robotY += g_wiiFwdInput * speedMultiplier * dt;

    Mtx model, modelView, translation, rotation;
    Mtx cameraView;
    
    guVector cam = {0.0f, 5.0f, 14.0f}; 
    guVector up = {0.0f, 1.0f, 0.0f};
    guVector look = {0.0f, 0.0f, 0.0f};
    guLookAt(cameraView, &cam, &up, &look);

    guMtxTrans(translation, g_robotX, 0.4f, g_robotY);
    guMtxRotRad(rotation, 'Y', g_robotHeadingRad);
    
    guMtxConcat(translation, rotation, model);
    guMtxConcat(cameraView, model, modelView);
    
    GX_LoadPosMtxImm(modelView, GX_PNMTX0);

    float sizeX = ROBOT_TRACKWIDTH;
    float sizeY = 0.3f;
    float sizeZ = ROBOT_WHEELBASE;

    GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
        // Front Face (FRC Alliance Blue)
        GX_Position3f32(-sizeX, -sizeY,  sizeZ); GX_Color4u8(0x00, 0x66, 0xFF, 0xFF);
        GX_Position3f32( sizeX, -sizeY,  sizeZ); GX_Color4u8(0x00, 0x66, 0xFF, 0xFF);
        GX_Position3f32( sizeX,  sizeY,  sizeZ); GX_Color4u8(0x00, 0x66, 0xFF, 0xFF);
        GX_Position3f32(-sizeX,  sizeY,  sizeZ); GX_Color4u8(0x00, 0x66, 0xFF, 0xFF);

        // Back Face (Dark shadow Blue)
        GX_Position3f32(-sizeX, -sizeY, -sizeZ); GX_Color4u8(0x00, 0x33, 0x99, 0xFF);
        GX_Position3f32(-sizeX,  sizeY, -sizeZ); GX_Color4u8(0x00, 0x33, 0x99, 0xFF);
        GX_Position3f32( sizeX,  sizeY, -sizeZ); GX_Color4u8(0x00, 0x33, 0x99, 0xFF);
        GX_Position3f32( sizeX, -sizeY, -sizeZ); GX_Color4u8(0x00, 0x33, 0x99, 0xFF);

        // Top Face (Silver frame color)
        GX_Position3f32(-sizeX,  sizeY,  sizeZ); GX_Color4u8(0xB3, 0xB3, 0xB3, 0xFF);
        GX_Position3f32( sizeX,  sizeY,  sizeZ); GX_Color4u8(0xB3, 0xB3, 0xB3, 0xFF);
        GX_Position3f32( sizeX,  sizeY, -sizeZ); GX_Color4u8(0xB3, 0xB3, 0xB3, 0xFF);
        GX_Position3f32(-sizeX,  sizeY, -sizeZ); GX_Color4u8(0xB3, 0xB3, 0xB3, 0xFF);

        // Right side profiles
        GX_Position3f32( sizeX, -sizeY, -sizeZ); GX_Color4u8(0x80, 0x80, 0x80, 0xFF);
        GX_Position3f32( sizeX,  sizeY, -sizeZ); GX_Color4u8(0x80, 0x80, 0x80, 0xFF);
        GX_Position3f32( sizeX,  sizeY,  sizeZ); GX_Color4u8(0x80, 0x80, 0x80, 0xFF);
        GX_Position3f32( sizeX, -sizeY,  sizeZ); GX_Color4u8(0x80, 0x80, 0x80, 0xFF);

        // Left side profiles
        GX_Position3f32(-sizeX, -sizeY, -sizeZ); GX_Color4u8(0x80, 0x80, 0x80, 0xFF);
        GX_Position3f32(-sizeX, -sizeY,  sizeZ); GX_Color4u8(0x80, 0x80, 0x80, 0xFF);
        GX_Position3f32(-sizeX,  sizeY,  sizeZ); GX_Color4u8(0x80, 0x80, 0x80, 0xFF);
        GX_Position3f32(-sizeX,  sizeY, -sizeZ); GX_Color4u8(0x80, 0x80, 0x80, 0xFF);

        // Bottom profile
        GX_Position3f32(-sizeX, -sizeY,  sizeZ); GX_Color4u8(0x1A, 0x1A, 0x1A, 0xFF);
        GX_Position3f32(-sizeX, -sizeY, -sizeZ); GX_Color4u8(0x1A, 0x1A, 0x1A, 0xFF);
        GX_Position3f32( sizeX, -sizeY, -sizeZ); GX_Color4u8(0x1A, 0x1A, 0x1A, 0xFF);
        GX_Position3f32( sizeX, -sizeY,  sizeZ); GX_Color4u8(0x1A, 0x1A, 0x1A, 0xFF);
    GX_End();
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
