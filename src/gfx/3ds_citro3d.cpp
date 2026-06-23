#if defined(__3DS__)
#include "renderer.h"
float g_robotHeadingRad = 0.0f; 
void InitGraphicsBackend() {}
void StartRenderFrame() {}
void RenderFRCField() {}
void RenderRobotChassis(const SwerveDriveStates& swerve) {}
void EndRenderFrame() {}
void DeinitGraphicsBackend() {}
#endif
