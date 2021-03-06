#include <string.h>
#include "priorities.h"

#include "arm_trajectory_manager.h"

#define ARM_TRAJ_MANAGER_STACKSIZE 4096

struct arm_traj_manager main_arm_traj_manager;

void arm_traj_manager_init(struct arm_traj_manager* manager)
{
    memset(manager, 0, sizeof(struct arm_traj_manager));
    manager->state = ARM_READY;
}

void arm_traj_manager_set_tolerances(struct arm_traj_manager *manager,
                                     float x_tol_mm, float y_tol_mm,
                                     float z_tol_mm)
{
    manager->tol_mm.x = x_tol_mm;
    manager->tol_mm.y = y_tol_mm;
    manager->tol_mm.z = z_tol_mm;
}

void arm_traj_wait_for_end(void)
{
    chThdSleepMilliseconds(1000 / ARM_TRAJ_MANAGER_FREQUENCY);
    while (main_arm_traj_manager.state == ARM_MOVING) {
        chThdSleepMilliseconds(5);
    }
    NOTICE("Arm trajectory goal reached");
}

static THD_FUNCTION(arm_traj_thd, arg)
{
    chRegSetThreadName(__FUNCTION__);

    scara_t* arm = (scara_t *)arg;

    arm_traj_manager_init(&main_arm_traj_manager);
    arm_traj_manager_set_tolerances(&main_arm_traj_manager, 10.f, 10.f, 2.f);

    NOTICE("Start arm trajectory manager");
    while (true) {
        int num_points_in_trajectory = arm->trajectory.frame_count;
        if (num_points_in_trajectory > 0) {
            scara_waypoint_t target_waypoint = arm->trajectory.frames[num_points_in_trajectory - 1];
            position_3d_t target_pos = target_waypoint.position;
            scara_coordinate_t target_system = target_waypoint.coordinate_type;

            position_3d_t current_pos = scara_position(arm, target_system);

            position_3d_t error;
            error.x = fabsf(target_pos.x - current_pos.x);
            error.y = fabsf(target_pos.y - current_pos.y);
            error.z = fabsf(target_pos.z - current_pos.z);

            if (error.x <= main_arm_traj_manager.tol_mm.x &&
                error.y <= main_arm_traj_manager.tol_mm.y &&
                error.z <= main_arm_traj_manager.tol_mm.z) {
                main_arm_traj_manager.state = ARM_READY;
            } else {
                main_arm_traj_manager.state = ARM_MOVING;
            }
        } else {
            main_arm_traj_manager.state = ARM_READY;
        }

        chThdSleepMilliseconds(1000 / ARM_TRAJ_MANAGER_FREQUENCY);
    }
}

void arm_trajectory_manager_start(scara_t* arm)
{
    static THD_WORKING_AREA(arm_traj_thd_wa, ARM_TRAJ_MANAGER_STACKSIZE);
    chThdCreateStatic(arm_traj_thd_wa, sizeof(arm_traj_thd_wa),
                      ARM_TRAJ_MANAGER_PRIO, arm_traj_thd, arm);
}
