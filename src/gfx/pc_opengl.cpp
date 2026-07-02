#if defined(__DESKTOP_PC__)
#include "../robot_physics.h"
#include "renderer.h"
#include <GLFW/glfw3.h>
#include <cmath>

// Global tracking variables exposed to main loops
GLFWwindow *g_pcWindow = nullptr;

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
  float fov = 60.0f * M_PI / 180.0f;
  float h = 1.0f / tan(fov / 2.0f);
  float aspect = 1280.0f / 720.0f;

  float m[16] = {h / aspect, 0, 0, 0, 0, h, 0, 0,
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

void RenderRobotChassis(const RobotMesh& mesh, const RobotPhysicsState& state) {
    glPushMatrix();
    
    // Position and spin strictly on physics parameters
    glTranslatef(state.position.x, 0.4f, -state.position.y); 
    glRotatef(-state.heading_rad * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);

    float assetScale = 5.0f; 
    glScalef(assetScale, assetScale, assetScale);

    glDisable(GL_LIGHTING); 

    glBegin(GL_TRIANGLES);
    for (unsigned int i = 0; i < mesh.indices.size(); i += 3) {
        if (i + 2 >= mesh.indices.size()) break;

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

void RenderGameBall(const BallPhysicsState& ball) {
    glPushMatrix();
    glTranslatef(ball.position.x, ball.radius, -ball.position.y); 
    
    // UPDATED: Bright Yellow primary channel colors
    glColor3f(1.0f, 1.0f, 0.0f); 
    float r = ball.radius;

    glBegin(GL_LINES);
    // Upper Pyramid Edges
    glVertex3f(0, r, 0);  glVertex3f(r, 0, 0);
    glVertex3f(0, r, 0);  glVertex3f(-r, 0, 0);
    glVertex3f(0, r, 0);  glVertex3f(0, 0, r);
    glVertex3f(0, r, 0);  glVertex3f(0, 0, -r);
    
    // Base Ring Perimeter
    glVertex3f(r, 0, 0);  glVertex3f(0, 0, r);
    glVertex3f(0, 0, r);  glVertex3f(-r, 0, 0);
    glVertex3f(-r, 0, 0); glVertex3f(0, 0, -r);
    glVertex3f(0, 0, -r); glVertex3f(r, 0, 0);

    // Lower Pyramid Edges
    glVertex3f(0, -r, 0); glVertex3f(r, 0, 0);
    glVertex3f(0, -r, 0); glVertex3f(-r, 0, 0);
    glVertex3f(0, -r, 0); glVertex3f(0, 0, r);
    glVertex3f(0, -r, 0); glVertex3f(0, 0, -r);
    glEnd();
    
    glPopMatrix();
}




void DeinitGraphicsBackend() {
  if (g_pcWindow)
    glfwDestroyWindow(g_pcWindow);
  glfwTerminate();
}
#endif
