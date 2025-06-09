/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/stepper_motion.h
 * @brief Public API for Stepper Motion Control
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_MOTION_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_MOTION_H_
/**
 * @brief Stepper Motion Control Interface
 * @defgroup stepper_motion_interface Stepper Motion Control Interface
 * @since 4.0
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stepper Motion Control Events
 */
enum stepper_motion_event {
	/** Steps set using move_by or move_to have been executed */
	STEPPER_MOTION_EVENT_STEPS_COMPLETED = 0,
	/** Stepper has stopped */
	STEPPER_MOTION_EVENT_STOPPED = 1,
};

/**
 * @brief Stepper ramp types for motion profiles
 */
enum stepper_ramp_type {
	STEPPER_RAMP_TYPE_SQUARE,
	STEPPER_RAMP_TYPE_TRAPEZOIDAL,
};

/**
 * @brief Square ramp profile configuration
 */
struct stepper_ramp_square_profile {
	uint64_t interval_ns;
};

/**
 * @brief Trapezoidal ramp profile configuration
 */
struct stepper_ramp_trapezoidal_profile {

	/**
	 * Interval in nanoseconds which should be reached after acceleration
	 * and used in the constant speed phase (target speed).
	 */
	uint64_t interval_ns;

	/**
	 * Acceleration rate in steps/s/s to be used during the acceleration phase.
	 */
	uint32_t acceleration_rate;

	/**
	 * Deceleration rate in steps/s/s to be used during the deceleration phase.
	 */
	uint32_t deceleration_rate;
};

/**
 * @brief Stepper ramp profile structure
 */
struct stepper_ramp_profile {
	enum stepper_ramp_type type;
	union {
		struct stepper_ramp_square_profile square;
		struct stepper_ramp_trapezoidal_profile trapezoidal;
	};
};

/**
 * @brief Callback function for stepper motion control events
 */
typedef void (*stepper_motion_event_callback_t)(const struct device *dev, 
					        const enum stepper_motion_event event,
					        void *user_data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * Stepper motion control driver API definition and system call entry points.
 *
 */

/**
 * @brief Set the reference position of the stepper
 *
 * @see stepper_motion_set_position() for details.
 */
typedef int (*stepper_motion_set_position_t)(const struct device *dev, const int32_t value);

/**
 * @brief Get the actual a.k.a reference position of the stepper
 *
 * @see stepper_motion_get_position() for details.
 */
typedef int (*stepper_motion_get_position_t)(const struct device *dev, int32_t *value);

/**
 * @brief Set the callback function to be called when a stepper motion event occurs
 *
 * @see stepper_motion_set_event_callback() for details.
 */
typedef int (*stepper_motion_set_event_callback_t)(const struct device *dev,
					           stepper_motion_event_callback_t callback, void *user_data);

/**
 * @brief Set the ramp to be used for the stepper
 *
 * @see stepper_motion_set_ramp() for details.
 */
typedef int (*stepper_motion_set_ramp_t)(const struct device *dev, const struct stepper_ramp_profile *ramp);

/**
 * @brief Move the stepper relatively by a given number of micro-steps.
 *
 * @see stepper_motion_move_by() for details.
 */
typedef int (*stepper_motion_move_by_t)(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Move the stepper to an absolute position in micro-steps.
 *
 * @see stepper_motion_move_to() for details.
 */
typedef int (*stepper_motion_move_to_t)(const struct device *dev, const int32_t micro_steps);

/**
 * @brief Run the stepper with a given step interval in a given direction
 *
 * @see stepper_motion_run() for details.
 */
typedef int (*stepper_motion_run_t)(const struct device *dev, const enum stepper_direction direction);

/**
 * @brief Stop the stepper
 *
 * @see stepper_motion_stop() for details.
 */
typedef int (*stepper_motion_stop_t)(const struct device *dev);

/**
 * @brief Is the target position fo the stepper reached
 *
 * @see stepper_motion_is_moving() for details.
 */
typedef int (*stepper_motion_is_moving_t)(const struct device *dev, bool *is_moving);

/**
 * @brief Stepper Motion Control Driver API
 */
__subsystem struct stepper_motion_driver_api {
	stepper_motion_set_position_t set_position;
	stepper_motion_get_position_t get_position;
	stepper_motion_set_event_callback_t set_event_callback;
	stepper_motion_set_ramp_t set_ramp;
	stepper_motion_move_by_t move_by;
	stepper_motion_move_to_t move_to;
	stepper_motion_run_t run;
	stepper_motion_stop_t stop;
	stepper_motion_is_moving_t is_moving;
};

/**
 * @endcond
 */

/**
 * @brief Set the reference position of the stepper
 *
 * @param dev Pointer to the stepper driver instance.
 * @param value The reference position to set in micro-steps.
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_motion_set_position(const struct device *dev, int32_t value);

static inline int z_impl_stepper_motion_set_position(const struct device *dev,
						     const int32_t value)
{
	const struct stepper_motion_driver_api *api = (const struct stepper_motion_driver_api *)dev->api;

	if (api->set_position == NULL) {
		return -ENOSYS;
	}
	return api->set_position(dev, value);
}

/**
 * @brief Get the actual a.k.a reference position of the stepper
 *
 * @param dev pointer to the stepper driver instance
 * @param value The actual position to get in micro-steps
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_motion_get_position(const struct device *dev, int32_t *value);

static inline int z_impl_stepper_motion_get_position(const struct device *dev, int32_t *value)
{
	const struct stepper_motion_driver_api *api = (const struct stepper_motion_driver_api *)dev->api;

	if (api->get_position == NULL) {
		return -ENOSYS;
	}
	return api->get_position(dev, value);
}

/**
 * @brief Set the callback function to be called when a stepper motion event occurs
 *
 * @param dev pointer to the stepper driver instance
 * @param callback Callback function to be called when a stepper motion event occurs
 * passing NULL will disable the callback
 * @param user_data User data to be passed to the callback function
 *
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_motion_set_event_callback(const struct device *dev,
					        stepper_motion_event_callback_t callback, void *user_data);

static inline int z_impl_stepper_motion_set_event_callback(const struct device *dev,
						           stepper_motion_event_callback_t callback,
						           void *user_data)
{
	const struct stepper_motion_driver_api *api = (const struct stepper_motion_driver_api *)dev->api;

	if (api->set_event_callback == NULL) {
		return -ENOSYS;
	}
	return api->set_event_callback(dev, callback, user_data);
}

/**
 * @brief Set the motion ramp for the stepper
 *
 * @details Configures the acceleration and deceleration profile used when moving the stepper motor.
 * The ramp defines how the stepper speeds up and decelerates, allowing for smooth motion control.
 *
 * @param dev Pointer to the stepper driver instance
 * @param ramp Pointer to ramp configuration structure
 *
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 * @retval -errno Other negative errno codes depending on implementation
 */
__syscall int stepper_motion_set_ramp(const struct device *dev, const struct stepper_ramp_profile *ramp);

static inline int z_impl_stepper_motion_set_ramp(const struct device *dev, const struct stepper_ramp_profile *ramp)
{
	const struct stepper_motion_driver_api *api = (const struct stepper_motion_driver_api *)dev->api;
	if (api->set_ramp == NULL) {
		return -ENOSYS;
	}
	return api->set_ramp(dev, ramp);
}

/**
 * @brief Set the micro-steps to be moved from the current position i.e. relative movement
 *
 * @details The stepper will move by the given number of micro-steps from the current position.
 * This function is non-blocking.
 *
 * @param dev pointer to the stepper driver instance
 * @param micro_steps target micro-steps to be moved from the current position
 *
 * @retval -ECANCELED If the stepper is disabled
 * @retval -EIO General input / output error
 * @retval 0 Success
 */
__syscall int stepper_motion_move_by(const struct device *dev, int32_t micro_steps);

static inline int z_impl_stepper_motion_move_by(const struct device *dev, const int32_t micro_steps)
{
	const struct stepper_motion_driver_api *api = (const struct stepper_motion_driver_api *)dev->api;

	return api->move_by(dev, micro_steps);
}

/**
 * @brief Set the absolute target position of the stepper
 *
 * @details The stepper will move to the given micro-steps position from the reference position.
 * This function is non-blocking.
 *
 * @param dev pointer to the stepper driver instance
 * @param micro_steps target position to set in micro-steps
 *
 * @retval -ECANCELED If the stepper is disabled
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_motion_move_to(const struct device *dev, int32_t micro_steps);

static inline int z_impl_stepper_motion_move_to(const struct device *dev, const int32_t micro_steps)
{
	const struct stepper_motion_driver_api *api = (const struct stepper_motion_driver_api *)dev->api;

	if (api->move_to == NULL) {
		return -ENOSYS;
	}
	return api->move_to(dev, micro_steps);
}

/**
 * @brief Run the stepper with a given step interval in a given direction
 *
 * @details The stepper shall be set into motion and run continuously until
 * stalled or stopped using some other command, for instance, stepper_enable(false). This
 * function is non-blocking.
 *
 * @param dev pointer to the stepper driver instance
 * @param direction The direction to set
 *
 * @retval -ECANCELED If the stepper is disabled
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_motion_run(const struct device *dev, enum stepper_direction direction);

static inline int z_impl_stepper_motion_run(const struct device *dev,
				     const enum stepper_direction direction)
{
	const struct stepper_motion_driver_api *api = (const struct stepper_motion_driver_api *)dev->api;

	if (api->run == NULL) {
		return -ENOSYS;
	}
	return api->run(dev, direction);
}

/**
 * @brief Stop the stepper
 * @details Cancel all active movements, however keep the coils energized.
 *
 * @param dev pointer to the stepper driver instance
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_motion_stop(const struct device *dev);

static inline int z_impl_stepper_motion_stop(const struct device *dev)
{
	const struct stepper_motion_driver_api *api = (const struct stepper_motion_driver_api *)dev->api;

	if (api->stop == NULL) {
		return -ENOSYS;
	}
	return api->stop(dev);
}

/**
 * @brief Check if the stepper is currently moving
 *
 * @param dev pointer to the stepper driver instance
 * @param is_moving Pointer to a boolean to store the moving status of the stepper
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_motion_is_moving(const struct device *dev, bool *is_moving);

static inline int z_impl_stepper_motion_is_moving(const struct device *dev, bool *is_moving)
{
	const struct stepper_motion_driver_api *api = (const struct stepper_motion_driver_api *)dev->api;

	if (api->is_moving == NULL) {
		return -ENOSYS;
	}
	return api->is_moving(dev, is_moving);
}

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/stepper_motion.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_MOTION_H_ */
