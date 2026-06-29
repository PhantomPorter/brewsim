// src/gfx/renderer.cpp
#include "renderer.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h> // REQUIRED: To define glBegin, glVertex3f, etc.
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Structure to hold the indices for a single triangle face
struct Triplet {
  unsigned int v_idx;
  unsigned int vt_idx;
  unsigned int vn_idx;
};

// ----------------------------------------------------------------------
// THE COMPLETE OBJ PARSER IMPLEMENTATION
// ----------------------------------------------------------------------

RobotMesh LoadRobotMesh(const std::string &filepath) {
  RobotMesh mesh;
  std::ifstream file(filepath);

  if (!file.is_open()) {
    std::cerr << "ERROR: Could not open file: " << filepath << std::endl;
    return mesh;
  }

  std::string line;
  std::vector<Vertex> raw_positions;
  std::vector<Triplet> face_triplets;

  // Track the active color state (defaulting to a visible Magenta/Pink
  // so we instantly know if a material isn't being caught by our if-statements)
  float currentR = 1.0f;
  float currentG = 0.0f;
  float currentB = 1.0f;

  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string prefix;
    ss >> prefix;

    if (prefix == "usemtl") {
      std::string matName;
      ss >> matName;

      // Temporary variables to extract the scanned floats
      float r = 0.5f, g = 0.5f, b = 0.5f;

      // Use sscanf to automatically extract up to 3 underscore-separated floats
      // from the name string. Example match:
      // "0.603922_0.647059_0.686275_0.000000_0.000000"
      int matched = std::sscanf(matName.c_str(), "%f_%f_%f", &r, &g, &b);

      if (matched >= 3) {
        // If parsing succeeded, apply the exact CAD values dynamically!
        currentR = r;
        currentG = g;
        currentB = b;
      } else {
        // Fallback default color if a weird named material shows up
        currentR = 0.5f;
        currentG = 0.5f;
        currentB = 0.5f;
      }
    } else if (prefix == "v") {
      float x, y, z;
      if (ss >> x >> y >> z) {
        // Push vertex with whatever the active color state is
        raw_positions.push_back({x, y, z, currentR, currentG, currentB});
      }
    } else if (prefix == "f") {
      std::string token;
      while (ss >> token) {
        std::stringstream token_ss(token);
        std::string str_v, str_vt, str_vn;

        std::getline(token_ss, str_v, '/');
        std::getline(token_ss, str_vt, '/');
        std::getline(token_ss, str_vn, '/');

        if (!str_v.empty()) {
          try {
            unsigned int v_idx = std::stoul(str_v);
            face_triplets.push_back({v_idx, 0, 0});
          } catch (...) {
          }
        }
      }
    }
  }
  file.close();

  // --- Phase 2: Populate the Final Mesh Structures ---
  // Copy the raw positions into the final mesh structure
  mesh.vertices = std::move(raw_positions);

  // Convert 1-based OBJ face triplets into 0-based index buffers
  for (const auto &triplet : face_triplets) {
    if (triplet.v_idx > 0 && triplet.v_idx <= mesh.vertices.size()) {
      mesh.indices.push_back(triplet.v_idx - 1);
    }
  }

  std::cout << "\n========================================================="
            << std::endl;
  std::cout << "SUCCESS: Robot mesh loaded from " << filepath << "!"
            << std::endl;
  std::cout << "Total vertices found: " << mesh.vertices.size() << std::endl;
  std::cout << "Total indices found (triangles): " << mesh.indices.size()
            << std::endl;
  std::cout << "=========================================================\n"
            << std::endl;

  return mesh;
}
