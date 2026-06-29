cmake_minimum_required(VERSION 3.13)
project(brewsim CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Universal Source Files ---
set(UNIVERSAL_SOURCES
    src/main.cpp
    src/robot_physics.cpp
    src/gfx/renderer.cpp
)

# --- 2. Target Platform Selector ---
if(WIIU)
    message(STATUS "Configuring Target: Nintendo Wii U")
    add_definitions(-D__WIIU__)
    
    include("$ENV{DEVKITPRO}/wut/share/wut.cmake" REQUIRED)
    
    # Compile with the Wii U GX2 graphics backend file
    add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/wiiu_gx2.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE wut m)
    wut_create_rpx(${PROJECT_NAME})

elseif(GEKKO OR NINTENDO_WII)
    message(STATUS "Configuring Target: Nintendo Wii")
    add_definitions(-D__WII__)
    
    list(APPEND CMAKE_MODULE_PATH $ENV{DEVKITPRO}/cmake/Platform)
    include(NintendoWii)
    
    include_directories($ENV{DEVKITPRO}/libogc/include)
    link_directories($ENV{DEVKITPRO}/libogc/lib/wii)
    
    # Compile with the Wii GX graphics backend file
    add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/wii_gx.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE wiiuse bte ogc m)
    ogc_create_dol(${PROJECT_NAME})

elseif(NINTENDO_3DS)
    message(STATUS "Configuring Target: Nintendo 3DS")
    add_definitions(-D__3DS__)
    
    list(APPEND CMAKE_MODULE_PATH $ENV{DEVKITPRO}/cmake/Platform)
    include(Nintendo3DS)
    
    include_directories($ENV{DEVKITPRO}/libctru/include)
    
    # Compile with the 3DS Citro3D graphics backend file
    add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/3ds_citro3d.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE citro3d ctru m)
    ctr_create_3dsx(${PROJECT_NAME})

else()
    message(STATUS "Configuring Target: Native Linux Desktop PC")
    add_definitions(-D__DESKTOP_PC__)
    
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
    
    # Compile with the Desktop OpenGL graphics backend file
    add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/pc_opengl.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL)
endif()
