#include <iostream>
#include "robot_physics.h"
#include "gfx/renderer.h"

// --- Platform Core Header Inclusions ---
#if defined(__DESKTOP_PC__)
    #include <GLFW/glfw3.h>
    // Global tracking handle pointer for our window execution context
#elif defined(__3DS__)
    #include <3ds.h>
#elif defined(__WII__)
    #include <gccore.h>
    #include <wiiuse/wpad.h> 
#elif defined(__WIIU__)
    #include <coreinit/thread.h>
    #include <coreinit/time.h>
    #include <vpad/input.h>
#endif

#if defined(__WII__)
    float g_wiiFwdInput = 0.0f;
    float g_wiiStrafeInput = 0.0f;
    float g_wiiRotInput = 0.0f;
#endif
// --- Abstract Global Input States ---
struct DriverStationInput {
    float drive_x = 0.0f;       // Swerve Translation X (-1.0 to 1.0)
    float drive_y = 0.0f;       // Swerve Translation Y (-1.0 to 1.0)
    float steer_rot = 0.0f;     // Swerve Angular Rotation (-1.0 to 1.0)
    bool autonomous_mode = false;
};



bool g_running = true;

// Expose the global simulated gyro orientation variable updated by the active renderer

int main(int argc, char** argv) {
    DriverStationInput player_controls;
    

    RobotDimensions my_robot_config = {
        1.2f,  // Example Track Width
        1.0f,  // Example Wheel Base
        0.25f  // Example Wheel Radius
    };

    // Load the robot's geometry mesh from the assets folder
    RobotMesh robot_mesh = LoadRobotMesh("model.obj"); 
    // 1. HARDWARE LAYER & ENGINE GRAPHICS SETUP
    // =========================================================================
    InitGraphicsBackend();

    // Secondary subsystem parameters configurations if needed outside renderer
#if defined(__3DS__)
    consoleInit(GFX_BOTTOM, NULL);
    std::cout << "\x1b[1;1H-- BREWSIM DRIVER STATION DATA --" << std::endl;
#elif defined(__WII__)
    PAD_Init(); 
    WPAD_Init(); 
#elif defined(__WIIU__)
    VPADInit();
#endif

    // =========================================================================
    // 2. MASTER RUNTIME LOOP EXECUTION
    // =========================================================================
    while (g_running) {
        
        // ---------------------------------------------------------------------
        // STEP A: INPUT POLLING & TRANSLATION
        // ---------------------------------------------------------------------
#if defined(__DESKTOP_PC__)
        if (g_pcWindow && glfwWindowShouldClose(g_pcWindow)) { g_running = false; }
        glfwPollEvents();
        if (g_pcWindow) {
            // WASD handles Field-Relative translation vectors
            player_controls.drive_x = (glfwGetKey(g_pcWindow, GLFW_KEY_D) == GLFW_PRESS) ? 1.0f : ((glfwGetKey(g_pcWindow, GLFW_KEY_A) == GLFW_PRESS) ? -1.0f : 0.0f);
            player_controls.drive_y = (glfwGetKey(g_pcWindow, GLFW_KEY_W) == GLFW_PRESS) ? 1.0f : ((glfwGetKey(g_pcWindow, GLFW_KEY_S) == GLFW_PRESS) ? -1.0f : 0.0f);
            // Arrow keys handle Spin Rotation vectors
            player_controls.steer_rot = (glfwGetKey(g_pcWindow, GLFW_KEY_RIGHT) == GLFW_PRESS) ? 1.0f : ((glfwGetKey(g_pcWindow, GLFW_KEY_LEFT) == GLFW_PRESS) ? -1.0f : 0.0f);
        }

#elif defined(__3DS__)
        hidScanInput();
        uint32_t kDown = hidKeysDown();
        if (kDown & KEY_START) { g_running = false; } 
        
        circlePosition cStick;
        hidCircleRead(&cStick);
        player_controls.drive_x = static_cast<float>(cStick.dx) / 150.0f;
        player_controls.drive_y = static_cast<float>(cStick.dy) / 150.0f;

#elif defined(__WII__)
        WPAD_ScanPads();
        uint32_t wDown = WPAD_ButtonsDown(0);
        uint32_t wHeld = WPAD_ButtonsHeld(0);
        
        if (wDown & WPAD_BUTTON_HOME) { g_running = false; } 

        // Clear previous tracking states
        g_wiiFwdInput = 0.0f;
        g_wiiStrafeInput = 0.0f;
        g_wiiRotInput = 0.0f;

        // Map Wii Remote buttons: 
        // 1 & 2 buttons handle spinning left and right
        if (wHeld & WPAD_BUTTON_1) g_wiiRotInput = -1.0f;
        if (wHeld & WPAD_BUTTON_2) g_wiiRotInput = 1.0f;

        WPADData* wpad_data = WPAD_Data(0);
        // Check if an Expansion Nunchuk is attached to drive the translations
        if (wpad_data->err == WPAD_ERR_NONE && wpad_data->exp.type == WPAD_EXP_NUNCHUK) {
            // Read and normalize Nunchuk stick vectors
            g_wiiStrafeInput = (static_cast<float>(wpad_data->exp.nunchuk.js.pos.x) - wpad_data->exp.nunchuk.js.center.x) / 45.0f;
            
            g_wiiFwdInput    = -((static_cast<float>(wpad_data->exp.nunchuk.js.pos.y) - wpad_data->exp.nunchuk.js.center.y) / 45.0f);
        } else {
            // Fallback: Invert the sideways Wiimote D-pad buttons to match
            if (wHeld & WPAD_BUTTON_UP)    g_wiiFwdInput = 1.0f;
            if (wHeld & WPAD_BUTTON_DOWN)  g_wiiFwdInput = -1.0f;
            if (wHeld & WPAD_BUTTON_LEFT)  g_wiiStrafeInput = -1.0f;
            if (wHeld & WPAD_BUTTON_RIGHT) g_wiiStrafeInput = 1.0f;
        }

        // Synchronize our universal tracking parameters back to the kinematics calculations
        player_controls.drive_x = g_wiiStrafeInput;
        player_controls.drive_y = g_wiiFwdInput;
        player_controls.steer_rot = g_wiiRotInput;


#elif defined(__WIIU__)
        VPADStatus vpad_status;
        VPADReadStatus vpad_error;
        VPADRead(VPAD_CHAN_0, &vpad_status, 1, &vpad_error);

        if (vpad_error == VPAD_READ_SUCCESS) {
            if (vpad_status.hold & VPAD_BUTTON_HOME) { g_running = false; } 
            
            player_controls.drive_x = vpad_status.leftStick.x;
            player_controls.drive_y = vpad_status.leftStick.y;
            player_controls.steer_rot = vpad_status.rightStick.x; 
        }
#endif

        // ---------------------------------------------------------------------
        // STEP B: PROCESS CORE PHYSICS CALCULATIONS
        // ---------------------------------------------------------------------
<<<<<<< HEAD
         SwerveDriveStates current_robot_swerve = CalculateSwerveKinematics(
            player_controls.drive_y, player_controls.drive_x, player_controls.steer_rot, g_robotHeadingRad, my_robot_config
        );
=======
        SwerveDriveStates current_robot_swerve = CalculateSwerveKinematics(
            player_controls.drive_y, player_controls.drive_x, player_controls.steer_rot, g_robotHeadingRad
        );

>>>>>>> 84c788d7a1d9c615dcf783b7a91086053a114957
        // ---------------------------------------------------------------------
        // STEP C: HARDWARE ACCELERATED 3D RENDER PIPELINE MATRIX
        // ---------------------------------------------------------------------
        StartRenderFrame();
        RenderFRCField();
<<<<<<< HEAD
        RenderRobotChassis(robot_mesh, current_robot_swerve); 
=======
        RenderRobotChassis(current_robot_swerve); 
>>>>>>> 84c788d7a1d9c615dcf783b7a91086053a114957
        EndRenderFrame();
    }

    // =========================================================================
    // 3. CLEAN SHUTDOWN DE-ALLOCATIONS
    // =========================================================================
    DeinitGraphicsBackend();
    return 0;
}
