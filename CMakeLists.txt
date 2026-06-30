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
    
    # === EMBEDDING PROCESS FOR WII ===
    find_program(XXD_PROGRAM xxd)
    if(XXD_PROGRAM)
        message(STATUS "Found xxd: Automating binary asset baking...")
        
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/modelwii_obj.h
            COMMAND ${XXD_PROGRAM} -i assets/modelwii.obj > ${CMAKE_CURRENT_BINARY_DIR}/modelwii_obj.h
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/modelwii.obj
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Baking OBJ into executable memory..."
        )
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/modelwii_mtl.h
            COMMAND ${XXD_PROGRAM} -i assets/modelwii.mtl > ${CMAKE_CURRENT_BINARY_DIR}/modelwii_mtl.h
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/modelwii.mtl
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Baking MTL into executable memory..."
        )
        
        # Define arrays as sources so they are compiled into this specific platform build target
        set(WII_EMBEDDED_HEADERS
            ${CMAKE_CURRENT_BINARY_DIR}/modelwii_obj.h
            ${CMAKE_CURRENT_BINARY_DIR}/modelwii_mtl.h
        )
    else()
        message(FATAL_ERROR "xxd command line utility not found! Cannot bake embedded assets.")
    endif()

    add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/wii_gx.cpp ${WII_EMBEDDED_HEADERS})
    target_link_libraries(${PROJECT_NAME} PRIVATE fat wiiuse bte ogc m)
    
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_definitions(${PROJECT_NAME} PRIVATE WII_EMBEDDED_ASSETS=1)
    
    ogc_create_dol(${PROJECT_NAME})

elseif(NINTENDO_3DS)
    message(STATUS "Configuring Target: Nintendo 3DS")
    add_definitions(-D__3DS__)
    
    list(APPEND CMAKE_MODULE_PATH $ENV{DEVKITPRO}/cmake/Platform)
    include(Nintendo3DS)
    
    include_directories($ENV{DEVKITPRO}/libctru/include)
    
    add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/3ds_citro3d.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE citro3d ctru m)
    ctr_create_3dsx(${PROJECT_NAME})

else()
    message(STATUS "Configuring Target: Native Linux Desktop PC")
    add_definitions(-D__DESKTOP_PC__)
    
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
    
    add_executable(${PROJECT_NAME} ${UNIVERSAL_SOURCES} src/gfx/pc_opengl.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL)
endif()

# --- 3. Global Configurations Applied Post-Target Creation ---
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
)

# Automatically copy the assets directory to the build output folder on every build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Copying assets folder to build destination..."
)