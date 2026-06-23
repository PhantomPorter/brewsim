#if defined(__DESKTOP_PC__)
#include "renderer.h"
#include <GLFW/glfw3.h>
#include <cmath>

extern GLFWwindow* g_pcWindow;

// Global tracking variables exposed to main loops
float g_robotX = 0.0f;
float g_robotY = 0.0f;
float g_robotHeadingRad = 0.0f; // Actively tracks gyro heading orientation

void InitGraphicsBackend() {
    if (!glfwInit()) return;
    g_pcWindow = glfwCreateWindow(1280, 720, "brewsim - MoSim-Style Field-Oriented Sim", NULL, NULL);
    if (!g_pcWindow) { glfwTerminate(); return; }
    glfwMakeContextCurrent(g_pcWindow);
    glfwSwapInterval(1);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
}

void StartRenderFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float fov = 60.0f * M_PI / 180.0f;
    float h = 1.0f / tan(fov / 2.0f);
    float aspect = 1280.0f / 720.0f;
    
    float m[16] = {
        h / aspect, 0, 0, 0,
        0, h, 0, 0,
        0, 0, -(100.1f) / (99.9f), -1,
        0, 0, -(2.0f * 100.0f * 0.1f) / (99.9f), 0
    };
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
        glVertex3f(-fieldSize, 0.0f, i); glVertex3f(fieldSize, 0.0f, i);
        glVertex3f(i, 0.0f, -fieldSize); glVertex3f(i, 0.0f, fieldSize);
    }
    glEnd();
}

void RenderRobotChassis(const SwerveDriveStates& swerve) {
    float dt = 0.016f;
    float speedMultiplier = 6.0f;
    float rotationMultiplier = 4.0f;

    float fwdInput = 0.0f;
    float strafeInput = 0.0f;
    float rotInput = 0.0f;

    if (g_pcWindow) {
        if (glfwGetKey(g_pcWindow, GLFW_KEY_W) == GLFW_PRESS) fwdInput = 1.0f;
        if (glfwGetKey(g_pcWindow, GLFW_KEY_S) == GLFW_PRESS) fwdInput = -1.0f;
        if (glfwGetKey(g_pcWindow, GLFW_KEY_D) == GLFW_PRESS) strafeInput = 1.0f;
        if (glfwGetKey(g_pcWindow, GLFW_KEY_A) == GLFW_PRESS) strafeInput = -1.0f;
        if (glfwGetKey(g_pcWindow, GLFW_KEY_RIGHT) == GLFW_PRESS) rotInput = 1.0f;
        if (glfwGetKey(g_pcWindow, GLFW_KEY_LEFT) == GLFW_PRESS) rotInput = -1.0f;
    }

    // 1. Accumulate turning orientation natively from the yaw axis
    g_robotHeadingRad += rotInput * rotationMultiplier * dt;

    
    g_robotX += strafeInput * speedMultiplier * dt;
    g_robotY += fwdInput * speedMultiplier * dt;

    // --- Draw the Robot ---
    glPushMatrix();
    // Render translation coordinates along the field grid structure
    glTranslatef(g_robotX, 0.4f, -g_robotY); 
    
    // Rotate the 3D block visually around the Y-axis to represent heading adjustments
    glRotatef(-g_robotHeadingRad * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);

    float sizeX = ROBOT_TRACKWIDTH;
    float sizeY = 0.3f;
    float sizeZ = ROBOT_WHEELBASE;

    glBegin(GL_QUADS);
        // Front Face (Alliance Blue)
        glColor3f(0.0f, 0.4f, 1.0f);
        glVertex3f(-sizeX, -sizeY,  sizeZ); glVertex3f( sizeX, -sizeY,  sizeZ);
        glVertex3f( sizeX,  sizeY,  sizeZ); glVertex3f(-sizeX,  sizeY,  sizeZ);

        // Back Face (Shadow color)
        glColor3f(0.0f, 0.2f, 0.6f);
        glVertex3f(-sizeX, -sizeY, -sizeZ); glVertex3f(-sizeX,  sizeY, -sizeZ);
        glVertex3f( sizeX,  sizeY, -sizeZ); glVertex3f( sizeX, -sizeY, -sizeZ);

        // Top Face (Deck Frame Plate)
        glColor3f(0.7f, 0.7f, 0.7f);
        glVertex3f(-sizeX,  sizeY,  sizeZ); glVertex3f( sizeX,  sizeY,  sizeZ);
        glVertex3f( sizeX,  sizeY, -sizeZ); glVertex3f(-sizeX,  sizeY, -sizeZ);

        // Side Panels
        glColor3f(0.5f, 0.5f, 0.5f);
        glVertex3f(-sizeX, -sizeY, -sizeZ); glVertex3f(-sizeX, -sizeY,  sizeZ);
        glVertex3f(-sizeX,  sizeY,  sizeZ); glVertex3f(-sizeX,  sizeY, -sizeZ);
        glVertex3f( sizeX, -sizeY, -sizeZ); glVertex3f( sizeX,  sizeY, -sizeZ);
        glVertex3f( sizeX,  sizeY,  sizeZ); glVertex3f( sizeX, -sizeY,  sizeZ);
    glEnd();

    // Yellow Direction Arrow Pointer
    glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.8f, 0.0f); 
        glVertex3f(-0.2f, sizeY + 0.01f, sizeZ - 0.2f);
        glVertex3f( 0.2f, sizeY + 0.01f, sizeZ - 0.2f);
        glVertex3f( 0.0f, sizeY + 0.01f, sizeZ + 0.3f); 
    glEnd();

    glPopMatrix();
}

void EndRenderFrame() { if (g_pcWindow) glfwSwapBuffers(g_pcWindow); }
void DeinitGraphicsBackend() { if (g_pcWindow) glfwDestroyWindow(g_pcWindow); glfwTerminate(); }
#endif
