#include "robot_physics.h"
#include <cmath>

// Define the global config declarations your header expects
const float ROBOT_TRACKWIDTH = 1.2f;
const float ROBOT_WHEELBASE = 1.0f;
const float ROBOT_RADIUS = 0.25f;

// Implementation of the parameterized kinematics function adapted to use the custom physics state
SwerveDriveStates CalculateSwerveKinematics(float fwd, float strafe, float rcw, const RobotPhysicsState& state, const RobotDimensions& dimensions) {
    SwerveDriveStates states;

    // --- FIELD-ORIENTED TRANSFORM ---
    // Extract the heading directly out of our single, unified tracking struct
    float cosHeading = std::cos(state.heading_rad);
    float sinHeading = std::sin(state.heading_rad);
    
    float robotFwd    = fwd * cosHeading + strafe * sinHeading;
    float robotStrafe = -fwd * sinHeading + strafe * cosHeading;

    // --- SWERVE MATRIX CALCULATION ---
    // L = Wheel Base (Front-to-Back), W = Track Width (Left-to-Right)
    float L = dimensions.wheel_base / 2.0f;
    float W = dimensions.track_width / 2.0f;

    // Correct swerve vector layout components
    float A = robotStrafe - rcw * L; // Back
    float B = robotStrafe + rcw * L; // Front
    float C = robotFwd    - rcw * W; // Left
    float D = robotFwd    + rcw * W; // Right

    // Assign module wheel vector sums
    states.front_right.speed = std::sqrt((B * B) + (D * D));
    states.front_left.speed  = std::sqrt((B * B) + (C * C));
    states.back_left.speed   = std::sqrt((A * A) + (C * C));
    states.back_right.speed  = std::sqrt((A * A) + (D * D));

    states.front_right.angle_rad = std::atan2(B, D);
    states.front_left.angle_rad  = std::atan2(B, C);
    states.back_left.angle_rad   = std::atan2(A, C);
    states.back_right.angle_rad  = std::atan2(A, D);

    // --- PROPORTIONAL SPEED NORMALIZATION ---
    float max_speed = states.front_right.speed;
    if (states.front_left.speed > max_speed)  max_speed = states.front_left.speed;
    if (states.back_left.speed > max_speed)   max_speed = states.back_left.speed;
    if (states.back_right.speed > max_speed)  max_speed = states.back_right.speed;

    if (max_speed > 1.0f) {
        states.front_right.speed /= max_speed;
        states.front_left.speed  /= max_speed;
        states.back_left.speed   /= max_speed;
        states.back_right.speed  /= max_speed;
    }

    return states;
}
