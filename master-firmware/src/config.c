#include <ch.h>
#include <stdio.h>
#include <error/error.h>
#include "config.h"

parameter_namespace_t global_config;

parameter_namespace_t actuator_config;

parameter_namespace_t master_config;

static parameter_namespace_t odometry_config;
static parameter_t robot_size, robot_alignement_length, opponent_size, calib_dir;
static parameter_t odometry_ticks, odometry_track, odometry_left_corr, odometry_right_corr;

static struct {
    parameter_namespace_t ns;
    struct {
        parameter_namespace_t ns;
        struct {
            parameter_namespace_t ns;
            parameter_t kp;
            parameter_t ki;
            parameter_t kd;
            parameter_t ilimit;
        } angle, distance;
    } control;

    struct {
        parameter_namespace_t ns;
        struct {
            parameter_namespace_t ns;
            struct {
                parameter_namespace_t ns;
                parameter_t init;
                parameter_t slow;
                parameter_t fast;
            } speed, acceleration;
        } angle, distance;
    } trajectories;
} aversive;

static parameter_namespace_t beacon_config;
static parameter_t beacon_reflector_radius, beacon_angular_offset;

static struct {
    parameter_namespace_t ns;
    struct {
        parameter_namespace_t ns;
        parameter_t offset_x;
        parameter_t offset_y;
        parameter_t offset_a;
    } right;
    struct {
        parameter_namespace_t ns;
        parameter_t retracted;
        parameter_t deployed;
    } servo;
} lever;

#ifdef DEBRA
static parameter_namespace_t arms_config, arms_main_config, motor_offsets_config;
static parameter_t upperarm_length, forearm_length;
static parameter_t main_offset_x, main_offset_y, main_offset_a;
static parameter_t main_z_offset, main_shoulder_offset, main_elbow_offset;

static struct {
    parameter_namespace_t ns;
    struct {
        parameter_namespace_t ns;
        struct {
            parameter_namespace_t ns;
            parameter_t kp;
            parameter_t ki;
            parameter_t kd;
            parameter_t ilimit;
        } x, y;
    } control;
} main_arm;
#endif

void config_init(void)
{
    parameter_namespace_declare(&global_config, NULL, NULL);
    parameter_namespace_declare(&actuator_config, &global_config, "actuator");
    parameter_namespace_declare(&master_config, &global_config, "master");

    parameter_integer_declare_with_default(&robot_size, &master_config, "robot_size_x_mm", 0);
    parameter_integer_declare_with_default(&robot_alignement_length, &master_config, "robot_alignment_length_mm", 0);
    parameter_integer_declare_with_default(&opponent_size, &master_config, "opponent_size_x_mm_default", 0);
    parameter_integer_declare_with_default(&calib_dir, &master_config, "calibration_direction", 0);

    parameter_namespace_declare(&odometry_config, &master_config, "odometry");
    parameter_scalar_declare_with_default(&odometry_ticks, &odometry_config, "external_encoder_ticks_per_mm", 0);
    parameter_scalar_declare_with_default(&odometry_track, &odometry_config, "external_track_mm", 0);
    parameter_scalar_declare_with_default(&odometry_left_corr, &odometry_config, "left_wheel_correction_factor", 0);
    parameter_scalar_declare_with_default(&odometry_right_corr, &odometry_config,
                             "right_wheel_correction_factor", 0);

    parameter_namespace_declare(&beacon_config, &master_config, "beacon");
    parameter_scalar_declare_with_default(&beacon_reflector_radius, &beacon_config, "reflector_radius", 0);
    parameter_scalar_declare_with_default(&beacon_angular_offset, &beacon_config, "angular_offset", 0);

    parameter_namespace_declare(&aversive.ns, &master_config, "aversive");

    parameter_namespace_declare(&aversive.control.ns, &aversive.ns, "control");
    parameter_namespace_declare(&aversive.control.angle.ns, &aversive.control.ns, "angle");
    parameter_scalar_declare_with_default(&aversive.control.angle.kp, &aversive.control.angle.ns, "kp", 0);
    parameter_scalar_declare_with_default(&aversive.control.angle.ki, &aversive.control.angle.ns, "ki", 0);
    parameter_scalar_declare_with_default(&aversive.control.angle.kd, &aversive.control.angle.ns, "kd", 0);
    parameter_scalar_declare_with_default(&aversive.control.angle.ilimit, &aversive.control.angle.ns, "i_limit", 0);
    parameter_namespace_declare(&aversive.control.distance.ns, &aversive.control.ns, "distance");
    parameter_scalar_declare_with_default(&aversive.control.distance.kp, &aversive.control.distance.ns, "kp", 0);
    parameter_scalar_declare_with_default(&aversive.control.distance.ki, &aversive.control.distance.ns, "ki", 0);
    parameter_scalar_declare_with_default(&aversive.control.distance.kd, &aversive.control.distance.ns, "kd", 0);
    parameter_scalar_declare_with_default(&aversive.control.distance.ilimit,
                             &aversive.control.distance.ns,
                             "i_limit", 0);

    parameter_namespace_declare(&aversive.trajectories.ns, &aversive.ns, "trajectories");
    parameter_namespace_declare(&aversive.trajectories.angle.ns,
                                &aversive.trajectories.ns, "angle");
    parameter_namespace_declare(&aversive.trajectories.angle.speed.ns,
                                &aversive.trajectories.angle.ns, "speed");
    parameter_scalar_declare_with_default(&aversive.trajectories.angle.speed.init,
                                          &aversive.trajectories.angle.speed.ns,
                                          "init", 0.);
    parameter_scalar_declare_with_default(&aversive.trajectories.angle.speed.slow,
                                          &aversive.trajectories.angle.speed.ns,
                                          "slow", 0.);
    parameter_scalar_declare_with_default(&aversive.trajectories.angle.speed.fast,
                                          &aversive.trajectories.angle.speed.ns,
                                          "fast", 0.);
    parameter_namespace_declare(&aversive.trajectories.angle.acceleration.ns,
                                &aversive.trajectories.angle.ns, "acceleration");
    parameter_scalar_declare_with_default(&aversive.trajectories.angle.acceleration.init,
                                          &aversive.trajectories.angle.acceleration.ns,
                                          "init", 0.);
    parameter_scalar_declare_with_default(&aversive.trajectories.angle.acceleration.slow,
                                          &aversive.trajectories.angle.acceleration.ns,
                                          "slow", 0.);
    parameter_scalar_declare_with_default(&aversive.trajectories.angle.acceleration.fast,
                                          &aversive.trajectories.angle.acceleration.ns,
                                          "fast", 0.);
    parameter_namespace_declare(&aversive.trajectories.distance.ns,
                                &aversive.trajectories.ns, "distance");
    parameter_namespace_declare(&aversive.trajectories.distance.speed.ns,
                                &aversive.trajectories.distance.ns, "speed");
    parameter_scalar_declare_with_default(&aversive.trajectories.distance.speed.init,
                                          &aversive.trajectories.distance.speed.ns,
                                          "init", 0.);
    parameter_scalar_declare_with_default(&aversive.trajectories.distance.speed.slow,
                                          &aversive.trajectories.distance.speed.ns,
                                          "slow", 0.);
    parameter_scalar_declare_with_default(&aversive.trajectories.distance.speed.fast,
                                          &aversive.trajectories.distance.speed.ns,
                                          "fast", 0.);
    parameter_namespace_declare(&aversive.trajectories.distance.acceleration.ns,
                                &aversive.trajectories.distance.ns, "acceleration");
    parameter_scalar_declare_with_default(&aversive.trajectories.distance.acceleration.init,
                                          &aversive.trajectories.distance.acceleration.ns,
                                          "init", 0.);
    parameter_scalar_declare_with_default(&aversive.trajectories.distance.acceleration.slow,
                                          &aversive.trajectories.distance.acceleration.ns,
                                          "slow", 0.);
    parameter_scalar_declare_with_default(&aversive.trajectories.distance.acceleration.fast,
                                          &aversive.trajectories.distance.acceleration.ns,
                                          "fast", 0.);

    parameter_namespace_declare(&lever.ns, &master_config, "lever");
    parameter_namespace_declare(&lever.servo.ns, &lever.ns, "servo");
    parameter_scalar_declare_with_default(&lever.servo.deployed, &lever.servo.ns, "deployed", 0);
    parameter_scalar_declare_with_default(&lever.servo.retracted, &lever.servo.ns, "retracted", 0);
    parameter_namespace_declare(&lever.right.ns, &lever.ns, "right");
    parameter_scalar_declare_with_default(&lever.right.offset_x, &lever.right.ns, "offset_x", 0);
    parameter_scalar_declare_with_default(&lever.right.offset_y, &lever.right.ns, "offset_y", 0);
    parameter_scalar_declare_with_default(&lever.right.offset_a, &lever.right.ns, "offset_a", 0);

#ifdef DEBRA
    parameter_namespace_declare(&arms_config, &master_config, "arms");

    parameter_scalar_declare_with_default(&upperarm_length, &arms_config, "upperarm_length", 0);
    parameter_scalar_declare_with_default(&forearm_length, &arms_config, "forearm_length", 0);

    parameter_namespace_declare(&arms_main_config, &arms_config, "main_arm");
    parameter_scalar_declare_with_default(&main_offset_x, &arms_main_config, "offset_x", 0);
    parameter_scalar_declare_with_default(&main_offset_y, &arms_main_config, "offset_y", 0);
    parameter_scalar_declare_with_default(&main_offset_a, &arms_main_config, "offset_a", 0);

    parameter_namespace_declare(&motor_offsets_config, &arms_config, "motor_offsets");
    parameter_scalar_declare_with_default(&main_z_offset, &motor_offsets_config, "z-joint", 0);
    parameter_scalar_declare_with_default(&main_shoulder_offset, &motor_offsets_config, "shoulder-joint", 0);
    parameter_scalar_declare_with_default(&main_elbow_offset, &motor_offsets_config, "elbow-joint", 0);

    parameter_namespace_declare(&main_arm.ns, &master_config, "main_arm");
    parameter_namespace_declare(&main_arm.control.ns, &main_arm.ns, "control");
    parameter_namespace_declare(&main_arm.control.x.ns, &main_arm.control.ns, "x");
    parameter_scalar_declare_with_default(&main_arm.control.x.kp, &main_arm.control.x.ns, "kp", 0);
    parameter_scalar_declare_with_default(&main_arm.control.x.ki, &main_arm.control.x.ns, "ki", 0);
    parameter_scalar_declare_with_default(&main_arm.control.x.kd, &main_arm.control.x.ns, "kd", 0);
    parameter_scalar_declare_with_default(&main_arm.control.x.ilimit, &main_arm.control.x.ns, "i_limit", 0);
    parameter_namespace_declare(&main_arm.control.y.ns, &main_arm.control.ns, "y");
    parameter_scalar_declare_with_default(&main_arm.control.y.kp, &main_arm.control.y.ns, "kp", 0);
    parameter_scalar_declare_with_default(&main_arm.control.y.ki, &main_arm.control.y.ns, "ki", 0);
    parameter_scalar_declare_with_default(&main_arm.control.y.kd, &main_arm.control.y.ns, "kd", 0);
    parameter_scalar_declare_with_default(&main_arm.control.y.ilimit, &main_arm.control.y.ns, "i_limit", 0);
#endif
}

float config_get_scalar(const char *id)
{
    parameter_t *p;

    p = parameter_find(&global_config, id);

    if (p == NULL) {
        ERROR("Unknown parameter \"%s\"", id);
    }

    return parameter_scalar_get(p);
}

int config_get_integer(const char *id)
{
    parameter_t *p;

    p = parameter_find(&global_config, id);

    if (p == NULL) {
        ERROR("Unknown parameter \"%s\"", id);
    }

    return parameter_integer_get(p);
}
