#if defined(__WII__)
    #include <gccore.h>
    #include <malloc.h>
    extern "C" { void SYS_Report(const char* format, ...); }
#endif

#include "renderer.h" 
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

#if defined(WII_EMBEDDED_ASSETS)
    #include "modelwii_obj.h"
    #include "modelwii_mtl.h"
#endif

struct MaterialPalette {
    char name[32];
    float r, g, b;
};


#define MAX_TEMP_POSITIONS 65000
static float* s_tempPositions = nullptr;
static uint32_t s_tempPositionCount = 0;

bool GetLineFromMemory(const unsigned char* buffer, uint32_t bufferLen, uint32_t& currentOffset, char* outLine, uint32_t maxLineLen) {
    if (currentOffset >= bufferLen) return false;

    uint32_t lineIdx = 0;
    while (currentOffset < bufferLen && lineIdx < (maxLineLen - 1)) {
        char c = static_cast<char>(buffer[currentOffset++]);
        if (c == '\n') break;
        if (c != '\r') {
            outLine[lineIdx++] = c;
        }
    }
    outLine[lineIdx] = '\0';
    return true;
}

RobotMesh LoadRobotMesh(const std::string& filepath) {
    RobotMesh mesh;
    std::vector<MaterialPalette> materials;
    materials.reserve(16);

    char lineBuffer[256];
    uint32_t memOffset = 0;

#if defined(__WII__)
    mesh.vertices = nullptr;
    mesh.vertexCount = 0;
#endif

    if (!s_tempPositions) {
        s_tempPositions = new float[MAX_TEMP_POSITIONS * 3];
    }
    s_tempPositionCount = 0;

    // =========================================================================
    // 1. Parse Material Palette (.mtl)
    // =========================================================================
#if defined(WII_EMBEDDED_ASSETS)
    memOffset = 0;
    MaterialPalette currentMat = {"", 1.0f, 1.0f, 1.0f};
    bool hasMat = false;

    while (GetLineFromMemory(assets_modelwii_mtl, assets_modelwii_mtl_len, memOffset, lineBuffer, sizeof(lineBuffer))) {
        if (lineBuffer[0] == '\0' || lineBuffer[0] == '#') continue;
        if (std::strncmp(lineBuffer, "newmtl ", 7) == 0) {
            if (hasMat) materials.push_back(currentMat);
            std::memset(&currentMat, 0, sizeof(MaterialPalette));
            std::sscanf(lineBuffer, "newmtl %31s", currentMat.name);
            hasMat = true;
        } else if (lineBuffer[0] == 'K' && lineBuffer[1] == 'd') {
            std::sscanf(lineBuffer, "Kd %f %f %f", &currentMat.r, &currentMat.g, &currentMat.b);
        }
    }
    if (hasMat) materials.push_back(currentMat);
#else
    std::string mtlPath = filepath;
    if (mtlPath.substr(mtlPath.find_last_of(".") + 1) == "obj") {
        mtlPath = mtlPath.substr(0, mtlPath.find_last_of(".")) + ".mtl";
    }
    std::ifstream mtl_file(mtlPath);
    if (mtl_file.is_open()) {
        std::string mtlLine;
        MaterialPalette currentMat = {"", 1.0f, 1.0f, 1.0f};
        bool hasMat = false;
        while (std::getline(mtl_file, mtlLine)) {
            if (mtlLine.empty() || mtlLine[0] == '#') continue;
            if (mtlLine.rfind("newmtl ", 0) == 0) {
                if (hasMat) materials.push_back(currentMat);
                std::memset(&currentMat, 0, sizeof(MaterialPalette));
                std::sscanf(mtlLine.c_str(), "newmtl %31s", currentMat.name);
                hasMat = true;
            } else if (mtlLine[0] == 'K' && mtlLine[1] == 'd') {
                std::sscanf(mtlLine.c_str(), "Kd %f %f %f", &currentMat.r, &currentMat.g, &currentMat.b);
            }
        }
        if (hasMat) materials.push_back(currentMat);
        mtl_file.close();
    }
#endif

    // =========================================================================
    // 2. Pre-flight Counting Pass (Wii Hardware Buffer Pre-allocation)
    // =========================================================================
    uint32_t totalWiiFacesNeeded = 0; // FIX: u32 -> uint32_t
#if defined(__WII__)
    memOffset = 0;
    while (GetLineFromMemory(assets_modelwii_obj, assets_modelwii_obj_len, memOffset, lineBuffer, sizeof(lineBuffer))) {
        if (lineBuffer[0] == 'f' && lineBuffer[1] == ' ') {
            totalWiiFacesNeeded++;
        }
    }
    uint32_t calculatedVertices = totalWiiFacesNeeded * 3;
    
    mesh.vertices = (Vertex*)memalign(32, calculatedVertices * sizeof(Vertex));
    std::memset(mesh.vertices, 0, calculatedVertices * sizeof(Vertex));
#endif

    // =========================================================================
    // 3. Pass A: Load Vertex Positions
    // =========================================================================
#if !defined(WII_EMBEDDED_ASSETS)
    std::ifstream obj_file_v(filepath);
    if (!obj_file_v.is_open()) return mesh;
    std::string diskLine;
    while (std::getline(obj_file_v, diskLine)) {
        std::strncpy(lineBuffer, diskLine.c_str(), sizeof(lineBuffer));
#else
    memOffset = 0;
    while (GetLineFromMemory(assets_modelwii_obj, assets_modelwii_obj_len, memOffset, lineBuffer, sizeof(lineBuffer))) {
#endif
        if (lineBuffer[0] == 'v' && lineBuffer[1] == ' ') {
            float x, y, z;
            if (std::sscanf(lineBuffer, "v %f %f %f", &x, &y, &z) == 3) {
#if defined(__WII__)
                if (s_tempPositionCount < MAX_TEMP_POSITIONS) {
                    u32 idx = s_tempPositionCount * 3;
                    s_tempPositions[idx]     = x;
                    s_tempPositions[idx + 1] = y;
                    s_tempPositions[idx + 2] = z;
                    s_tempPositionCount++;
                }
#else
                Vertex v{};
                v.x = x; v.y = y; v.z = z;
                v.r = 1.0f; v.g = 1.0f; v.b = 1.0f;
                mesh.vertices.push_back(v);
#endif
            }
        }
    }
#if !defined(WII_EMBEDDED_ASSETS)
    obj_file_v.close();
#endif

    // =========================================================================
    // 4. Pass B: Build Connected Elements Sequence
    // =========================================================================
#if !defined(WII_EMBEDDED_ASSETS)
    std::ifstream obj_file_f(filepath);
    if (!obj_file_f.is_open()) return mesh;
#endif

    float activeR = 1.0f, activeG = 1.0f, activeB = 1.0f;
    memOffset = 0;

#if !defined(WII_EMBEDDED_ASSETS)
    while (std::getline(obj_file_f, diskLine)) {
        std::strncpy(lineBuffer, diskLine.c_str(), sizeof(lineBuffer));
#else
    while (GetLineFromMemory(assets_modelwii_obj, assets_modelwii_obj_len, memOffset, lineBuffer, sizeof(lineBuffer))) {
#endif
        if (lineBuffer[0] == '\0' || lineBuffer[0] == '#') continue;

        if (lineBuffer[0] == 'u' && std::strncmp(lineBuffer, "usemtl ", 7) == 0) {
            char targetMatName[32] = {0};
            if (std::sscanf(lineBuffer, "usemtl %31s", targetMatName) == 1) {
                for (const auto& mat : materials) {
                    if (std::strcmp(mat.name, targetMatName) == 0) {
                        activeR = mat.r; activeG = mat.g; activeB = mat.b;
                        break;
                    }
                }
            }
            continue;
        }

        if (lineBuffer[0] == 'f' && lineBuffer[1] == ' ') {
            int v1, v2, v3;
            bool parsed = false;

            if (std::strchr(lineBuffer, '/') != nullptr) {
                int dummy;
                if (std::sscanf(lineBuffer, "f %d/%d/%d %d/%d/%d %d/%d/%d", &v1, &dummy, &dummy, &v2, &dummy, &dummy, &v3, &dummy, &dummy) == 9 ||
                    std::sscanf(lineBuffer, "f %d//%d %d//%d %d//%d", &v1, &dummy, &v2, &dummy, &v3, &dummy) == 6 ||
                    std::sscanf(lineBuffer, "f %d/%d %d/%d %d/%d", &v1, &dummy, &v2, &dummy, &v3, &dummy) == 6) {
                    parsed = true;
                }
            } else {
                if (std::sscanf(lineBuffer, "f %d %d %d", &v1, &v2, &v3) == 3) {
                    parsed = true;
                }
            }

            if (parsed) {
                v1--; v2--; v3--;
                
#if defined(__WII__)
                if (v1 < (int)s_tempPositionCount && v2 < (int)s_tempPositionCount && v3 < (int)s_tempPositionCount) {
                    u32 writeIndex = mesh.vertexCount;

                    mesh.vertices[writeIndex].x = s_tempPositions[v1 * 3];
                    mesh.vertices[writeIndex].y = s_tempPositions[v1 * 3 + 1];
                    mesh.vertices[writeIndex].z = s_tempPositions[v1 * 3 + 2];
                    mesh.vertices[writeIndex].r = activeR; mesh.vertices[writeIndex].g = activeG; mesh.vertices[writeIndex].b = activeB;

                    mesh.vertices[writeIndex + 1].x = s_tempPositions[v2 * 3];
                    mesh.vertices[writeIndex + 1].y = s_tempPositions[v2 * 3 + 1];
                    mesh.vertices[writeIndex + 1].z = s_tempPositions[v2 * 3 + 2];
                    mesh.vertices[writeIndex + 1].r = activeR; mesh.vertices[writeIndex + 1].g = activeG; mesh.vertices[writeIndex + 1].b = activeB;

                    mesh.vertices[writeIndex + 2].x = s_tempPositions[v3 * 3];
                    mesh.vertices[writeIndex + 2].y = s_tempPositions[v3 * 3 + 1];
                    mesh.vertices[writeIndex + 2].z = s_tempPositions[v3 * 3 + 2];
                    mesh.vertices[writeIndex + 2].r = activeR; mesh.vertices[writeIndex + 2].g = activeG; mesh.vertices[writeIndex + 2].b = activeB;

                    mesh.vertexCount += 3;
                }
#else
                if (v1 < (int)mesh.vertices.size() && v2 < (int)mesh.vertices.size() && v3 < (int)mesh.vertices.size()) {
                    mesh.vertices[v1].r = activeR; mesh.vertices[v1].g = activeG; mesh.vertices[v1].b = activeB;
                    mesh.vertices[v2].r = activeR; mesh.vertices[v2].g = activeG; mesh.vertices[v2].b = activeB;
                    mesh.vertices[v3].r = activeR; mesh.vertices[v3].g = activeG; mesh.vertices[v3].b = activeB;

                    mesh.indices.push_back(static_cast<unsigned int>(v1));
                    mesh.indices.push_back(static_cast<unsigned int>(v2));
                    mesh.indices.push_back(static_cast<unsigned int>(v3));
                }
#endif
            }
        }
    }

#if !defined(WII_EMBEDDED_ASSETS)
    obj_file_f.close();
#endif

#if defined(__WII__)
    SYS_Report("SUCCESSFULLY LOADED UNTRUNCATED MESH: %d Total Vertices Allocated dynamically.\n", mesh.vertexCount);
#endif

    return mesh;
}