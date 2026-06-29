// src/gfx/pc_opengl.cpp (THIS IS THE FINAL CODE FOR THE DRAWING)

#if defined(__DESKTOP_PC__)
#include "../robot_physics.h"
#include "renderer.h"
#include <GLFW/glfw3.h>
#include <cmath>

// Global tracking variables exposed to main loops
extern GLFWwindow *g_pcWindow = nullptr;

// Global state variables (defined here, fulfilling the extern declaration in
// renderer.h)
float g_robotX = 0.0f;
float g_robotY = 0.0f;
float g_robotHeadingRad = 0.0f;

void InitGraphicsBackend() {
  if (!glfwInit())
    return;
  g_pcWindow = glfwCreateWindow(
      1280, 720, "brewsim - MoSim-Style Field-Oriented Sim", NULL, NULL);
  if (!g_pcWindow) {
    glfwTerminate();
    return;
  }
  glfwMakeContextCurrent(g_pcWindow);
  glfwSwapInterval(1);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
}

void StartRenderFrame() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // --- Camera Simplification ---
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  // Keep the projection settings, but make the world bigger
  float fov = 60.0f * M_PI / 180.0f;
  float h = 1.0f / tan(fov / 2.0f);
  float aspect = 1280.0f / 720.0f;

  float m[16] = {h / aspect, 0, 0, 0, 0, h, 0, 0,
                 // Increased the Z bounds to cover a larger field of view
                 0, 0, -(200.1f) / (199.9f), -1, 0, 0,
                 -(4.0f * 100.0f * 0.1f) / (199.9f), 0};
  glLoadMatrixf(m);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  // Static camera position acting as the Driver Station glass viewpoint
  glTranslatef(0.0f, -5.0f, -14.0f);
  glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
}

void RenderFRCField() {
  glBegin(GL_LINES);
  glColor3f(0.25f, 0.25f, 0.25f);
  float fieldSize = 15.0f;
  for (float i = -fieldSize; i <= fieldSize; i += 1.0f) {
    glVertex3f(-fieldSize, 0.0f, i);
    glVertex3f(fieldSize, 0.0f, i);
    glVertex3f(i, 0.0f, -fieldSize);
    glVertex3f(i, 0.0f, fieldSize);
  }
  glEnd();
}

// FINAL FUNCTION: Renders the imported OBJ mesh geometry!
void RenderRobotChassis(const RobotMesh &mesh,
                        const SwerveDriveStates &swerve) {

  // --- 1. Calculate Pose (Physics Integration) ---
  // Use the kinematics output to update the robot's position and heading.
  float dt = 0.016f;

  // Integrate translational movement (X, Y) based on the current calculated
  // state This assumes the kinematics function outputs a normalized speed (0 to
  // 1).
  g_robotX += swerve.front_left.speed * dt;
  g_robotY += swerve.front_left.speed * dt;

  // Integrate rotational movement
  // We multiply by a factor to tune the speed of rotation in the visualization.
  g_robotHeadingRad += swerve.front_left.angle_rad * dt * 10.0f;

  // --- 2. Rendering the Mesh ---

  glPushMatrix();

  // Apply the calculated position and rotation to the current OpenGL matrix
  glTranslatef(g_robotX, 0.4f, -g_robotY);
  glRotatef(-g_robotHeadingRad * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
  glScalef(5.0f, 5.0f, 5.0f);                          
  // Draw the mesh using the stored indices
  // Inside src/gfx/pc_opengl.cpp -> RenderRobotChassis()
    glBegin(GL_TRIANGLES);
    for (unsigned int i = 0; i < mesh.indices.size(); i += 3) {
        const Vertex& v1 = mesh.vertices[mesh.indices[i]];
        const Vertex& v2 = mesh.vertices[mesh.indices[i+1]];
        const Vertex& v3 = mesh.vertices[mesh.indices[i+2]];

        glColor3f(v1.r, v1.g, v1.b); glVertex3f(v1.x, v1.y, v1.z);
        glColor3f(v2.r, v2.g, v2.b); glVertex3f(v2.x, v2.y, v2.z);
        glColor3f(v3.r, v3.g, v3.b); glVertex3f(v3.x, v3.y, v3.z);
    }
    glEnd();

  glPopMatrix();
}

void EndRenderFrame() {
  if (g_pcWindow)
    glfwSwapBuffers(g_pcWindow);
}
void DeinitGraphicsBackend() {
  if (g_pcWindow)
    glfwDestroyWindow(g_pcWindow);
  glfwTerminate();
}
#endif