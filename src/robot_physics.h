#ifndef ROBOT_PHYSICS_H
#define ROBOT_PHYSICS_H

extern const float ROBOT_TRACKWIDTH;
extern const float ROBOT_WHEELBASE;
extern const float ROBOT_RADIUS;

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

SwerveDriveStates CalculateSwerveKinematics(float fwd, float strafe, float rcw, float robotHeadingRad);

#endif 
