cmake_minimum_required(VERSION 3.13)
project(FRC_Console_Sim CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Universal Source Files ---
set(UNIVERSAL_SOURCES
    src/main.cpp
    src/robot_physics.cpp
)

# --- 2. Toolchain Detection & Platform Logic ---
if(DEFINED ENV{DEVKITPRO})
    message(STATUS "devkitPro Toolchain Detected!")

    if(WIIU)
        message(STATUS "Configuring Target: Nintendo Wii U (PowerPC Multi-core)")
        add_definitions(-D__WIIU__)

        # Find the official devkitPro Wii U Toolchain (WUT) package wrapper
        find_package(wut REQUIRED)
        
        # Inject standard hardware engine headers for input and GX2 graphics
        include_directories(${WUT_INCLUDE_DIRS})
        
        add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/wiiu_gx2.cpp)
        
        # Link core system hooks (coreinit), input processing (vpad/pad), and audio
        target_link_libraries(${PROJECT_NAME} PRIVATE 
            wut::coreinit 
            wut::gx2 
            wut::vpad 
            wut::pad 
            m
        )
        
        # devkitPro macro to post-process the code into a Wii U homebrew executable (.rpx)
        wut_create_rpx(${PROJECT_NAME})

    elseif(WII)
        message(STATUS "Configuring Target: Nintendo Wii (PowerPC)")
        add_definitions(-D__WII__)
        
        include_directories($ENV{DEVKITPRO}/libogc/include)
        link_directories($ENV{DEVKITPRO}/libogc/lib/wii)
        
        add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/wii_gx.cpp)
        target_link_libraries(${PROJECT_NAME} PRIVATE wiiuse bte ogc m)
        
        wii_make_dol(${PROJECT_NAME})

    elseif(3DS)
        message(STATUS "Configuring Target: Nintendo 3DS (ARM)")
        add_definitions(-D__3DS__)
        
        include_directories($ENV{DEVKITPRO}/libctru/include)
        
        add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/3ds_citro3d.cpp)
        target_link_libraries(${PROJECT_NAME} PRIVATE citro3d ctru m)
        
        add_3dsx_target(${PROJECT_NAME})
    endif()

else()
    # --- 3. Desktop Native Fallback Config ---
    message(STATUS "Configuring Target: Desktop PC (OpenGL)")
    add_definitions(-D__DESKTOP_PC__)

    find_package(GLFW3 REQUIRED)
    find_package(OpenGL REQUIRED)

    add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/pc_opengl.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE GLFW::GLFW OpenGL::GL)
endif()
