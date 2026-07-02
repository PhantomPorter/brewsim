#ifndef ROBOT_PHYSICS_H
#define ROBOT_PHYSICS_H

#include <cmath> 

// ============================================================================
// 1. STRUCTURES AND VECTOR MATH
// ============================================================================
extern const float ROBOT_TRACKWIDTH;
extern const float ROBOT_WHEELBASE;
extern const float ROBOT_RADIUS;

typedef float scalar_t;

struct Vector3 {
    scalar_t x;
    scalar_t y;
    scalar_t z;
    Vector3(scalar_t _x = 0, scalar_t _y = 0, scalar_t _z = 0) : x(_x), y(_y), z(_z) {}
};

struct RobotPhysicsState {
    Vector3 position;
    Vector3 velocity;
    scalar_t heading_rad;
    scalar_t angular_velocity;

    RobotPhysicsState() : position(0,0,0), velocity(0,0,0), heading_rad(0), angular_velocity(0) {}
};

struct BallPhysicsState {
    Vector3 position;
    Vector3 velocity;
    float radius;

    BallPhysicsState() : position(0,0,0), velocity(0,0,0), radius(0.24f) {} 
};

struct RobotDimensions {
    float track_width;
    float wheel_base;
    float radius;
};

struct SwerveModuleState {
    float speed = 0.0f;       
    float angle_rad = 0.0f;   
};

struct SwerveDriveStates {
    SwerveModuleState front_left;
    SwerveModuleState front_right;
    SwerveModuleState back_left;
    SwerveModuleState back_right;
};

inline Vector3 vec_add(const Vector3& a, const Vector3& b) { return Vector3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline Vector3 vec_scale(const Vector3& v, scalar_t s) { return Vector3(v.x * s, v.y * s, v.z * s); }
inline Vector3 vec_sub(const Vector3& a, const Vector3& b) { return Vector3(a.x - b.x, a.y - b.y, a.z - b.z); }

// ============================================================================
// 2. COLLISION DETECTION AND RESPONSES
// ============================================================================
inline void HandleFieldCollisions(RobotPhysicsState& state, const RobotDimensions& dimensions) {
    const float field_min_x = -15.0f; const float field_max_x =  15.0f;
    const float field_min_y = -15.0f; const float field_max_y =  15.0f;
    float half_size = dimensions.radius;

    if (state.position.x - half_size < field_min_x) { state.position.x = field_min_x + half_size; state.velocity.x = 0; }
    else if (state.position.x + half_size > field_max_x) { state.position.x = field_max_x - half_size; state.velocity.x = 0; }
    if (state.position.y - half_size < field_min_y) { state.position.y = field_min_y + half_size; state.velocity.y = 0; }
    else if (state.position.y + half_size > field_max_y) { state.position.y = field_max_y - half_size; state.velocity.y = 0; }
}

inline void UpdateAndBounceBall(BallPhysicsState& ball, RobotPhysicsState& robot, const RobotDimensions& dimensions, scalar_t dt) {
    const float field_min_x = -15.0f; const float field_max_x =  15.0f;
    const float field_min_y = -15.0f; const float field_max_y =  15.0f;

    ball.position = vec_add(ball.position, vec_scale(ball.velocity, dt));
    ball.velocity = vec_scale(ball.velocity, 0.98f); 

    if (ball.position.x - ball.radius < field_min_x) { ball.position.x = field_min_x + ball.radius; ball.velocity.x *= -0.6f; }
    else if (ball.position.x + ball.radius > field_max_x) { ball.position.x = field_max_x - ball.radius; ball.velocity.x *= -0.6f; }
    if (ball.position.y - ball.radius < field_min_y) { ball.position.y = field_min_y + ball.radius; ball.velocity.y *= -0.6f; }
    else if (ball.position.y + ball.radius > field_max_y) { ball.position.y = field_max_y - ball.radius; ball.velocity.y *= -0.6f; }

    Vector3 delta = vec_sub(ball.position, robot.position);
    float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    float collision_dist = dimensions.radius + ball.radius;

    if (distance < collision_dist && distance > 0.001f) {
        Vector3 normal = vec_scale(delta, 1.0f / distance);
        ball.position = vec_add(robot.position, vec_scale(normal, collision_dist));

        ball.velocity.x = robot.velocity.x * 1.3f + (normal.x * 2.0f);
        ball.velocity.y = robot.velocity.y * 1.3f + (normal.y * 2.0f);
    }
}

// ============================================================================
// 3. INTEGRATION STEP (CAMERA-FIXED WITH DEADZONES)
// ============================================================================
inline void IntegrateRobotPhysics(RobotPhysicsState& state, float fwd_input, float strafe_input, float rot_input, const RobotDimensions& dimensions, scalar_t dt) {
    float translation_magnitude = std::sqrt(strafe_input * strafe_input + fwd_input * fwd_input);
    const float translation_deadzone = 0.15f; const float rotation_deadzone = 0.12f;
    float final_strafe = 0.0f; float final_fwd = 0.0f; float final_rot = 0.0f;

    if (translation_magnitude > translation_deadzone) {
        float scale = (translation_magnitude - translation_deadzone) / (1.0f - translation_deadzone);
        final_strafe = (strafe_input / translation_magnitude) * scale;
        final_fwd = (fwd_input / translation_magnitude) * scale;
    }
    if (std::abs(rot_input) > rotation_deadzone) {
        float rot_sign = (rot_input > 0.0f) ? 1.0f : -1.0f;
        final_rot = rot_sign * (std::abs(rot_input) - rotation_deadzone) / (1.0f - rotation_deadzone);
    }

    state.velocity.x = final_strafe * 8.0f; 
    state.velocity.y = final_fwd * 8.0f;
    state.angular_velocity = final_rot * 5.0f; 

    state.position = vec_add(state.position, vec_scale(state.velocity, dt));
    state.heading_rad += state.angular_velocity * dt;

    HandleFieldCollisions(state, dimensions);
}

SwerveDriveStates CalculateSwerveKinematics(float fwd, float strafe, float rcw, const RobotPhysicsState& state, const RobotDimensions& dimensions);

#endif
