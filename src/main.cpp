#include <iostream>

// --- Platform Core Header Inclusions ---
#if defined(__DESKTOP_PC__)
    #include <GLFW/glfw3.h>
#elif defined(__3DS__)
    #include <3ds.h>
#elif defined(__WII__)
    #include <gccore.h>
    #include <wiiuse/wpad.h> 
#elif defined(__WIIU__)
    #include <coreinit/thread.h>
    #include <coreinit/time.h>
    #include <vpad/input.h>
    #include <whb/log.h>
    #include <whb/log_console.h>
#endif

// --- Abstract Global Input States ---
struct DriverStationInput {
    float drive_x = 0.0f;       
    float drive_y = 0.0f;       
    float steer_rot = 0.0f;     
    bool autonomous_mode = false;
};

bool g_running = true;

int main(int argc, char** argv) {
    DriverStationInput player_controls;

    // =========================================================================
    // 1. HARDWARE LAYER INITIALIZATION
    // =========================================================================
#if defined(__DESKTOP_PC__)
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "FRC Sim - Desktop", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

#elif defined(__3DS__)
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    std::cout << "\x1b[1;1H-- FRC DRIVER STATION TERMINAL --" << std::endl;

#elif defined(__WII__)
    VIDEO_Init();
    PAD_Init(); 
    WPAD_Init(); 
    GXRModeObj* rmode = VIDEO_GetPreferredMode(NULL);
    void* xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * 2);
    std::cout << "BREWSIM INIT ON WII HARDWARE" << std::endl;

#elif defined(__WIIU__)
    VPADInit();
    WHBLogConsoleInit();
    WHBLogPrintf("========================================");
    WHBLogPrintf("  BREWSIM FRC SIMULATOR INITIALIZED!    ");
    WHBLogPrintf("========================================");
#endif

    // =========================================================================
    // 2. MAIN SIMULATION ENGINE LOOP
    // =========================================================================
    while (g_running) {
        
#if defined(__DESKTOP_PC__)
        if (glfwWindowShouldClose(window)) { g_running = false; }
        glfwPollEvents();
        player_controls.drive_x = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) ? 1.0f : ((glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) ? -1.0f : 0.0f);
        player_controls.drive_y = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) ? 1.0f : ((glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) ? -1.0f : 0.0f);

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
        if (wDown & WPAD_BUTTON_HOME) { g_running = false; } 
        WPADData* wpad_data = WPAD_Data(0);
        if (wpad_data->err == WPAD_ERR_NONE && wpad_data->exp.type == WPAD_EXP_NUNCHUK) {
            player_controls.drive_x = wpad_data->exp.nunchuk.js.pos.x;
            player_controls.drive_y = wpad_data->exp.nunchuk.js.pos.y;
        }

#elif defined(__WIIU__)
        VPADStatus vpad_status;
        VPADReadError vpad_error;
        VPADRead(VPAD_CHAN_0, &vpad_status, 1, &vpad_error);

        if (vpad_error == VPAD_READ_SUCCESS) {
            if (vpad_status.hold & VPAD_BUTTON_HOME) { g_running = false; } 
            
            player_controls.drive_x = vpad_status.leftStick.x;
            player_controls.drive_y = vpad_status.leftStick.y;
            player_controls.steer_rot = vpad_status.rightStick.x; 

            WHBLogPrintf("-- FRC DRIVER STATION DATA --");
            WHBLogPrintf("Drive X Vector: %.2f", player_controls.drive_x);
            WHBLogPrintf("Drive Y Vector: %.2f", player_controls.drive_y);
            WHBLogPrintf("Steer Rotation: %.2f", player_controls.steer_rot);
        }
#endif

        // ---------------------------------------------------------------------
        // STEP B: DRAW COMPONENT RENDER PIPELINES
        // ---------------------------------------------------------------------
#if defined(__DESKTOP_PC__)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwSwapBuffers(window);
#elif defined(__3DS__)
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
#elif defined(__WII__)
        VIDEO_WaitVSync();
#elif defined(__WIIU__)
        // Force the text logging canvas to draw itself onto Cemu's viewport
        WHBLogConsoleDraw();
#endif
    }

    // =========================================================================
    // 3. CLEAN SHUTDOWN DE-ALLOCATIONS
    // =========================================================================
#if defined(__DESKTOP_PC__)
    glfwDestroyWindow(window);
    glfwTerminate();
#elif defined(__3DS__)
    gfxExit();
#elif defined(__WIIU__)
    // Clean up logging framework out of memory
    WHBLogConsoleFree();
#endif

    return 0;
}
