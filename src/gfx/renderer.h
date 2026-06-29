#ifndef RENDERER_H
#define RENDERER_H

#include "../robot_physics.h"
// Include the necessary GLFW header here to declare g_pcWindow correctly
#include <GLFW/glfw3.h> 
#include <vector>
#include <string>

// ----------------------------------------------------------------------
// DECLARATIONS FOR GLOBAL STATE
// ----------------------------------------------------------------------
extern GLFWwindow* g_pcWindow; // Declaration for the window handle
extern float g_robotX;
extern float g_robotY;
extern float g_robotHeadingRad; 


struct Vertex {
    float x, y, z; // Position coordinates
    float r, g, b; // Color channels (0.0 to 1.0)
};

struct RobotMesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices; 
};

RobotMesh LoadRobotMesh(const std::string& filepath);

void InitGraphicsBackend();
void StartRenderFrame();
void RenderFRCField();
void RenderRobotChassis(const RobotMesh& mesh, const SwerveDriveStates& swerve);
void EndRenderFrame();
void DeinitGraphicsBackend();

#endif
