#ifndef RENDERER_H
#define RENDERER_H

#include "../robot_physics.h"

void InitGraphicsBackend();
void StartRenderFrame();
void RenderFRCField();
void RenderRobotChassis(const SwerveDriveStates& swerve);
void EndRenderFrame();
void DeinitGraphicsBackend();

#endif 
