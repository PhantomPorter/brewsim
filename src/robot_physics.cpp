#include "robot_physics.h"
#include <cmath>

const float ROBOT_TRACKWIDTH = 0.6f;  
const float ROBOT_WHEELBASE   = 0.6f;  
const float ROBOT_RADIUS = std::sqrt((ROBOT_WHEELBASE * ROBOT_WHEELBASE) + (ROBOT_TRACKWIDTH * ROBOT_TRACKWIDTH));

SwerveDriveStates CalculateSwerveKinematics(float fwd, float strafe, float rcw, float robotHeadingRad) {
    SwerveDriveStates states;

    // --- FIELD-ORIENTED TRANSFORM ---
    // Standard rotation matrix converting field commands into robot relative commands
    float cosHeading = std::cos(robotHeadingRad);
    float sinHeading = std::sin(robotHeadingRad);
    
    float robotFwd    = fwd * cosHeading + strafe * sinHeading;
    float robotStrafe = -fwd * sinHeading + strafe * cosHeading;

    // --- STANDARD FRC SWERVE MATRIX ---
    // Half dimensions used for component velocity calculations
    float L = ROBOT_WHEELBASE / 2.0f;
    float W = ROBOT_TRACKWIDTH / 2.0f;

    // Calculate individual coordinate components for each module
    float A = robotStrafe - rcw * L;
    float B = robotStrafe + rcw * L;
    float C = robotFwd    - rcw * W;
    float D = robotFwd    + rcw * W;

    // Assign corrected vector combinations to each module pod
    states.front_right.speed = std::sqrt((B * B) + (C * C));
    states.front_left.speed  = std::sqrt((B * B) + (D * D));
    states.back_left.speed   = std::sqrt((A * A) + (D * D));
    states.back_right.speed  = std::sqrt((A * A) + (C * C));

    states.front_right.angle_rad = std::atan2(B, C);
    states.front_left.angle_rad  = std::atan2(B, D);
    states.back_left.angle_rad   = std::atan2(A, D);
    states.back_right.angle_rad  = std::atan2(A, C);

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
