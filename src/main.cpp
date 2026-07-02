#include "gfx/renderer.h"
#include "robot_physics.h"
#include <iostream>

// --- Platform Core Header Inclusions ---
#if defined(__DESKTOP_PC__)
#include <GLFW/glfw3.h>
extern GLFWwindow *g_pcWindow;
#elif defined(__3DS__)
#include <3ds.h>
#elif defined(__WII__)
#include <fat.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
extern "C" {
void SYS_Report(const char *format, ...);
}
#elif defined(__WIIU__)
#include <vpad/input.h>
#endif

#if defined(__WII__)
float g_wiiFwdInput = 0.0f;
float g_wiiStrafeInput = 0.0f;
float g_wiiRotInput = 0.0f;
#endif

struct DriverStationInput {
  float drive_x = 0.0f;   
  float drive_y = 0.0f;   
  float steer_rot = 0.0f; 
  bool autonomous_mode = false;
};

bool g_running = true;

int main(int argc, char **argv) {
  InitGraphicsBackend();
  DriverStationInput player_controls;

 RobotDimensions my_robot_config = { 1.2f * 5.0f, 1.0f * 5.0f, 0.25f * 5.0f }; 
  
  // Track structural vectors and orientation across frames
  RobotPhysicsState robot_state;
  robot_state.position = Vector3(0.0f, 0.0f, 0.0f);
  robot_state.velocity = Vector3(0.0f, 0.0f, 0.0f);
  robot_state.heading_rad = 0.0f;
  robot_state.angular_velocity = 0.0f;

  // Track the rolling game piece simulation vectors
  BallPhysicsState game_ball;
  game_ball.position = Vector3(2.0f, 2.0f, 0.0f); // Spawns slightly in front of center field
  game_ball.velocity = Vector3(0.0f, 0.0f, 0.0f);

  // Establish a constant simulation step (60Hz target frame rate)
  const float dt = 1.0f / 60.0f; 

  std::string assetsRoot = ASSETS_PATH;
  std::string modelFullPath;

#if defined(__WII__)
  modelFullPath = "EMBEDDED";
#else
  modelFullPath = std::string(ASSETS_PATH) + "modelpchigh.obj";
#endif

  RobotMesh robot_mesh = LoadRobotMesh(modelFullPath);

#if defined(__WII__)
    if (robot_mesh.vertexCount > 0 && robot_mesh.vertices != nullptr) {
        DCStoreRange(robot_mesh.vertices, robot_mesh.vertexCount * sizeof(Vertex));
        __asm__ volatile("sc");
    }
#endif
#if defined(__3DS__)
  consoleInit(GFX_BOTTOM, NULL);
  std::cout << "\x1b[1;1H-- BREWSIM DRIVER STATION DATA --" << std::endl;
#elif defined(__WII__)
  PAD_Init();
  WPAD_Init();
#elif defined(__WIIU__)
  VPADInit();
#endif

  while (g_running) {
    // ---------------------------------------------------------------------
    // STEP A: INPUT POLLING & TRANSLATION
    // ---------------------------------------------------------------------
#if defined(__DESKTOP_PC__)
    if (g_pcWindow && glfwWindowShouldClose(g_pcWindow)) {
      g_running = false;
    }
    glfwPollEvents();
    if (g_pcWindow) {
      player_controls.drive_x = (glfwGetKey(g_pcWindow, GLFW_KEY_D) == GLFW_PRESS) ? 1.0f : ((glfwGetKey(g_pcWindow, GLFW_KEY_A) == GLFW_PRESS) ? -1.0f : 0.0f);
      player_controls.drive_y = (glfwGetKey(g_pcWindow, GLFW_KEY_W) == GLFW_PRESS) ? 1.0f : ((glfwGetKey(g_pcWindow, GLFW_KEY_S) == GLFW_PRESS) ? -1.0f : 0.0f);
      player_controls.steer_rot = (glfwGetKey(g_pcWindow, GLFW_KEY_RIGHT) == GLFW_PRESS) ? 1.0f : ((glfwGetKey(g_pcWindow, GLFW_KEY_LEFT) == GLFW_PRESS) ? -1.0f : 0.0f);
    }
#elif defined(__3DS__)
    hidScanInput();
    uint32_t kDown = hidKeysDown();
    if (kDown & KEY_START) g_running = false;
    circlePosition cStick;
    hidCircleRead(&cStick);
    player_controls.drive_x = static_cast<float>(cStick.dx) / 150.0f;
    player_controls.drive_y = static_cast<float>(cStick.dy) / 150.0f;
    
    uint32_t kHeld = hidKeysHeld();
    player_controls.steer_rot = (kHeld & KEY_DRIGHT) ? 1.0f : ((kHeld & KEY_DLEFT) ? -1.0f : 0.0f);
#elif defined(__WII__)
    WPAD_ScanPads();
    uint32_t wDown = WPAD_ButtonsDown(0);
    uint32_t wHeld = WPAD_ButtonsHeld(0);

    if (wDown & WPAD_BUTTON_HOME) {
      g_running = false;
    }

    g_wiiFwdInput = 0.0f;
    g_wiiStrafeInput = 0.0f;
    g_wiiRotInput = 0.0f;

    if (wHeld & WPAD_BUTTON_1) g_wiiRotInput = -1.0f;
    if (wHeld & WPAD_BUTTON_2) g_wiiRotInput = 1.0f;

    WPADData *wpad_data = WPAD_Data(0);
    if (wpad_data->err == WPAD_ERR_NONE && wpad_data->exp.type == WPAD_EXP_NUNCHUK) {
      g_wiiStrafeInput = (static_cast<float>(wpad_data->exp.nunchuk.js.pos.x) - wpad_data->exp.nunchuk.js.center.x) / 45.0f;
      g_wiiFwdInput = -((static_cast<float>(wpad_data->exp.nunchuk.js.pos.y) - wpad_data->exp.nunchuk.js.center.y) / 45.0f);
    } else {
      if (wHeld & WPAD_BUTTON_UP)    g_wiiFwdInput = 1.0f;
      if (wHeld & WPAD_BUTTON_DOWN)  g_wiiFwdInput = -1.0f;
      if (wHeld & WPAD_BUTTON_LEFT)  g_wiiStrafeInput = -1.0f;
      if (wHeld & WPAD_BUTTON_RIGHT) g_wiiStrafeInput = 1.0f;
    }

    player_controls.drive_x = g_wiiStrafeInput;
    player_controls.drive_y = g_wiiFwdInput;
    player_controls.steer_rot = g_wiiRotInput;
#elif defined(__WIIU__)
    VPADStatus vpad_status;
    VPADReadError vpad_error; 
    VPADRead(VPAD_CHAN_0, &vpad_status, 1, &vpad_error);

    if (vpad_error == WPAD_READ_SUCCESS) { 
      if (vpad_status.hold & VPAD_BUTTON_HOME) {
        g_running = false;
      }

      player_controls.drive_x = vpad_status.leftStick.x;
      player_controls.drive_y = vpad_status.leftStick.y;
      player_controls.steer_rot = vpad_status.rightStick.x;
    }
#endif

    // ---------------------------------------------------------------------
    // STEP B: KINEMATICS BACKGROUND MATH & INTEGRATION
    // ---------------------------------------------------------------------
    // Move the chassis, apply deadzones, and lock movement to the camera axis
    IntegrateRobotPhysics(robot_state, player_controls.drive_y, player_controls.drive_x, player_controls.steer_rot, my_robot_config, dt);

    // 2. Process ball interactions matching those absolute coordinates
    UpdateAndBounceBall(game_ball, robot_state, my_robot_config, dt);

    // 3. Process swerve module state outputs 
    // Pass raw joystick values so wheel angle indicators stay relative to the chassis frame
    SwerveDriveStates current_robot_swerve = CalculateSwerveKinematics(
        player_controls.drive_y, player_controls.drive_x,
        player_controls.steer_rot, robot_state, my_robot_config);

    // ---------------------------------------------------------------------
    // STEP C: HARDWARE ACCELERATED 3D RENDER PIPELINE
    // ---------------------------------------------------------------------
#if defined(__WII__)
    Mtx* currentFrameViewMtx = StartRenderFrame();
    RenderFRCField();
    RenderGameBall(game_ball);
    RenderRobotChassis(robot_mesh, robot_state, currentFrameViewMtx);
#else
    StartRenderFrame();
    RenderFRCField();
    RenderGameBall(game_ball);
    RenderRobotChassis(robot_mesh, robot_state);
#endif

    EndRenderFrame();
  }

  DeinitGraphicsBackend();
  return 0;
}
