#include "robot_physics.h"
#include <cmath>

// Implementation of the parameterized kinematics function
// NOTE: This function now relies entirely on the 'dimensions' passed in.
SwerveDriveStates CalculateSwerveKinematics(float fwd, float strafe, float rcw, float robotHeadingRad, const RobotDimensions& dimensions) {
    SwerveDriveStates states;

    // --- FIELD-ORIENTED TRANSFORM ---
    // Standard rotation matrix converting field commands into robot relative commands
    float cosHeading = std::cos(robotHeadingRad);
    float sinHeading = std::sin(robotHeadingRad);
    
    float robotFwd    = fwd * cosHeading + strafe * sinHeading;
    float robotStrafe = -fwd * sinHeading + strafe * cosHeading;

    // --- SWERVE MATRIX CALCULATION ---
    // Use dimensions provided by the CAD model
    float L = dimensions.wheel_base / 2.0f;
    float W = dimensions.track_width / 2.0f;

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
