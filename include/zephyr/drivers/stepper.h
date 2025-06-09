/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file drivers/stepper.h
 * @brief Public API for Stepper Driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_

/**
 * @brief Stepper Driver Interface
 * @defgroup stepper_interface Stepper Driver Interface
 * @since 4.0
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stepper Motor micro-step resolution options
 */
enum stepper_micro_step_resolution {
	/** Full step resolution */
	STEPPER_MICRO_STEP_1 = 1,
	/** 2 micro-steps per full step */
	STEPPER_MICRO_STEP_2 = 2,
	/** 4 micro-steps per full step */
	STEPPER_MICRO_STEP_4 = 4,
	/** 8 micro-steps per full step */
	STEPPER_MICRO_STEP_8 = 8,
	/** 16 micro-steps per full step */
	STEPPER_MICRO_STEP_16 = 16,
	/** 32 micro-steps per full step */
	STEPPER_MICRO_STEP_32 = 32,
	/** 64 micro-steps per full step */
	STEPPER_MICRO_STEP_64 = 64,
	/** 128 micro-steps per full step */
	STEPPER_MICRO_STEP_128 = 128,
	/** 256 micro-steps per full step */
	STEPPER_MICRO_STEP_256 = 256,
};

/**
 * @brief Macro to calculate the index of the microstep resolution
 * @param res Microstep resolution
 */
#define MICRO_STEP_RES_INDEX(res) LOG2(res)

#define VALID_MICRO_STEP_RES(res)                                                                  \
	((res) == STEPPER_MICRO_STEP_1 || (res) == STEPPER_MICRO_STEP_2 ||                         \
	 (res) == STEPPER_MICRO_STEP_4 || (res) == STEPPER_MICRO_STEP_8 ||                         \
	 (res) == STEPPER_MICRO_STEP_16 || (res) == STEPPER_MICRO_STEP_32 ||                       \
	 (res) == STEPPER_MICRO_STEP_64 || (res) == STEPPER_MICRO_STEP_128 ||                      \
	 (res) == STEPPER_MICRO_STEP_256)

/**
 * @brief Stepper Motor direction options
 */
enum stepper_direction {
	/** Negative direction */
	STEPPER_DIRECTION_NEGATIVE = -1,
	/** Positive direction */
	STEPPER_DIRECTION_POSITIVE = 1,
};

/**
 * @brief Stepper Hardware Events
 */
enum stepper_event {
	/** Stall detected */
	STEPPER_EVENT_STALL_DETECTED = 0,
	/** Left end switch status changes to pressed */
	STEPPER_EVENT_LEFT_END_STOP_DETECTED = 1,
	/** Right end switch status changes to pressed */
	STEPPER_EVENT_RIGHT_END_STOP_DETECTED = 2,
	/** Fault with the stepper controller detected */
	STEPPER_EVENT_FAULT_DETECTED = 3,
};

/**
 * @brief Callback function for stepper hardware events
 */
typedef void (*stepper_event_callback_t)(const struct device *dev, const enum stepper_event event,
					 void *user_data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * Stepper driver API definition and system call entry points.
 *
 */

/**
 * @brief Enable the stepper driver.
 *
 * @see stepper_enable() for details.
 */
typedef int (*stepper_enable_t)(const struct device *dev);

/**
 * @brief Disable the stepper driver.
 *
 * @see stepper_disable() for details.
 */
typedef int (*stepper_disable_t)(const struct device *dev);

/**
 * @brief Set the micro-step resolution
 *
 * @see stepper_set_micro_step_res() for details.
 */
typedef int (*stepper_set_micro_step_res_t)(const struct device *dev,
					    const enum stepper_micro_step_resolution resolution);

/**
 * @brief Get the micro-step resolution
 *
 * @see stepper_get_micro_step_res() for details.
 */
typedef int (*stepper_get_micro_step_res_t)(const struct device *dev,
					    enum stepper_micro_step_resolution *resolution);

/**
 * @brief Set the callback function to be called when a stepper hardware event occurs
 *
 * @see stepper_set_event_callback() for details.
 */
typedef int (*stepper_set_event_callback_t)(const struct device *dev,
					    stepper_event_callback_t callback, void *user_data);

/**
 * @brief Perform a single step in the specified direction
 *
 * @see stepper_step() for details.
 */
typedef int (*stepper_step_t)(const struct device *dev, enum stepper_direction direction);

/**
 * @brief Stepper Driver API
 */
__subsystem struct stepper_driver_api {
	stepper_enable_t enable;
	stepper_disable_t disable;
	stepper_set_micro_step_res_t set_micro_step_res;
	stepper_get_micro_step_res_t get_micro_step_res;
	stepper_set_event_callback_t set_event_callback;
	stepper_step_t step;
};

/**
 * @endcond
 */

/**
 * @brief Enable stepper driver
 *
 * @details Enabling the driver will energize the coils, however not set the stepper in motion.
 *
 * @param dev pointer to the stepper driver instance
 *
 * @retval -EIO Error during Enabling
 * @retval 0 Success
 */
__syscall int stepper_enable(const struct device *dev);

static inline int z_impl_stepper_enable(const struct device *dev)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	return api->enable(dev);
}

/**
 * @brief Disable stepper driver
 *
 * @details Disabling the driver shall cancel all active movements and de-energize the coils.
 *
 * @param dev pointer to the stepper driver instance
 *
 * @retval -EIO Error during Disabling
 * @retval 0 Success
 */
__syscall int stepper_disable(const struct device *dev);

static inline int z_impl_stepper_disable(const struct device *dev)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	return api->disable(dev);
}

/**
 * @brief Set the micro-step resolution in stepper driver
 *
 * @param dev pointer to the stepper driver instance
 * @param resolution micro-step resolution
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval -ENOTSUP If the requested resolution is not supported
 * @retval 0 Success
 */
__syscall int stepper_set_micro_step_res(const struct device *dev,
					 enum stepper_micro_step_resolution resolution);

static inline int z_impl_stepper_set_micro_step_res(const struct device *dev,
						    enum stepper_micro_step_resolution resolution)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->set_micro_step_res == NULL) {
		return -ENOSYS;
	}
	return api->set_micro_step_res(dev, resolution);
}

/**
 * @brief Get the micro-step resolution in stepper driver
 *
 * @param dev pointer to the stepper driver instance
 * @param resolution micro-step resolution
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_get_micro_step_res(const struct device *dev,
					 enum stepper_micro_step_resolution *resolution);

static inline int z_impl_stepper_get_micro_step_res(const struct device *dev,
						    enum stepper_micro_step_resolution *resolution)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->get_micro_step_res == NULL) {
		return -ENOSYS;
	}
	return api->get_micro_step_res(dev, resolution);
}

/**
 * @brief Set the callback function to be called when a stepper hardware event occurs
 *
 * @param dev pointer to the stepper driver instance
 * @param callback Callback function to be called when a stepper hardware event occurs
 * passing NULL will disable the callback
 * @param user_data User data to be passed to the callback function
 *
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_set_event_callback(const struct device *dev,
					 stepper_event_callback_t callback, void *user_data);

static inline int z_impl_stepper_set_event_callback(const struct device *dev,
						    stepper_event_callback_t callback,
						    void *user_data)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->set_event_callback == NULL) {
		return -ENOSYS;
	}
	return api->set_event_callback(dev, callback, user_data);
}

/**
 * @brief Perform a single step in the specified direction
 *
 * @details This function performs exactly one micro-step in the specified direction.
 * It provides precise control for applications that need to control each step individually.
 * The stepper motor must be enabled before calling this function.
 *
 * @param dev Pointer to the stepper driver instance
 * @param direction The direction to step (STEPPER_DIRECTION_POSITIVE or STEPPER_DIRECTION_NEGATIVE)
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
__syscall int stepper_step(const struct device *dev, enum stepper_direction direction);

static inline int z_impl_stepper_step(const struct device *dev, enum stepper_direction direction)
{
	const struct stepper_driver_api *api = (const struct stepper_driver_api *)dev->api;

	if (api->step == NULL) {
		return -ENOSYS;
	}
	return api->step(dev, direction);
}

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/stepper.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_H_ */
