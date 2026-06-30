#include "renderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

// Helper structure to hold RGB color channels
struct RGBColor {
    float r = 0.5f;
    float g = 0.5f;
    float b = 0.5f;
};

// Main function to load the mesh and materials synchronously
RobotMesh LoadRobotMesh(const std::string& filepath) {
    RobotMesh mesh;
    std::ifstream obj_file(filepath);

    if (!obj_file.is_open()) {
        std::cerr << "ERROR: Could not open OBJ file: " << filepath << std::endl;
        return mesh;
    }

    // --- PHASE 1: PARSE THE COMPANION .MTL FILE FIRST ---
    std::map<std::string, RGBColor> material_library;
    
    // Convert "path/to/model.obj" -> "path/to/model.mtl"
    std::string mtl_filepath = filepath;
    size_t ext_pos = mtl_filepath.find_last_of(".");
    if (ext_pos != std::string::npos) {
        mtl_filepath = mtl_filepath.substr(0, ext_pos) + ".mtl";
    }

    std::ifstream mtl_file(mtl_filepath);
    if (mtl_file.is_open()) {
        std::string mtl_line;
        std::string current_mtl_name = "";

        while (std::getline(mtl_file, mtl_line)) {
            std::stringstream ss(mtl_line);
            std::string prefix;
            ss >> prefix;

            if (prefix == "newmtl") {
                ss >> current_mtl_name;
            } 
            else if (prefix == "Kd" && !current_mtl_name.empty()) {
                float r, g, b;
                if (ss >> r >> g >> b) {
                    material_library[current_mtl_name] = {r, g, b};
                }
            }
        }
        mtl_file.close();
        std::cout << "SUCCESS: Loaded " << material_library.size() << " materials from " << mtl_filepath << std::endl;
    } else {
        std::cerr << "WARNING: Could not find or open companion material file: " << mtl_filepath << std::endl;
    }

    // --- PHASE 2: PARSE THE OBJ FILE & DYNAMICALLY ASSIGN COLORS ---
    std::string line;
    std::vector<Vertex> raw_positions;

    // Fallback tracking color state
    float currentR = 0.6f; 
    float currentG = 0.6f; 
    float currentB = 0.6f; 

    while (std::getline(obj_file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            float x, y, z;
            if (ss >> x >> y >> z) {
                // Collect clean vertex coordinates (We can use Option 2 from scaling here if needed!)
                float scaleFactor = 5.0f; // Increase size by 5x right on load
                raw_positions.push_back({x * scaleFactor, y * scaleFactor, z * scaleFactor, 1.0f, 1.0f, 1.0f});
            }
        } 
        else if (prefix == "usemtl") {
            std::string matName;
            ss >> matName;
            
            // Match the material name to our loaded map dictionary!
            auto it = material_library.find(matName);
            if (it != material_library.end()) {
                currentR = it->second.r;
                currentG = it->second.g;
                currentB = it->second.b;
            } else {
                // If it fails to find the material key, fall back to neutral gray
                currentR = 0.5f; currentG = 0.5f; currentB = 0.5f;
            }
        }
        else if (prefix == "f") {
            std::string token;
            while (ss >> token) {
                std::stringstream token_ss(token);
                std::string str_v;
                
                if (std::getline(token_ss, str_v, '/')) {
                    if (!str_v.empty()) {
                        try {
                            unsigned int v_idx = std::stoul(str_v);
                            if (v_idx > 0 && v_idx <= raw_positions.size()) {
                                Vertex final_vertex = raw_positions[v_idx - 1];
                                
                                // Set the actual material colors read out of the file
                                final_vertex.r = currentR;
                                final_vertex.g = currentG;
                                final_vertex.b = currentB;

                                mesh.vertices.push_back(final_vertex);
                                mesh.indices.push_back(mesh.vertices.size() - 1);
                            }
                        } catch (...) {}
                    }
                }
            }
        }
    }

    obj_file.close();
    
    std::cout << "\n=========================================================" << std::endl;
    std::cout << "SUCCESS: Integrated " << filepath << " with materials successfully!" << std::endl;
    std::cout << "Total Triangles: " << mesh.indices.size() / 3 << std::endl;
    std::cout << "=========================================================\n" << std::endl;
    
    return mesh;
}