/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stepper_motion_controller.h"

#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stepper_motion_controller_wrapper, CONFIG_STEPPER_LOG_LEVEL);

struct stepper_motion_controller_wrapper_config {
	// Motion controller configuration must be first
	const struct stepper_motion_controller_config motion_controller_config;
	// Reference to the underlying hardware stepper device
	const struct device *stepper_dev;
};

struct stepper_motion_controller_wrapper_data {
	// Motion controller data must be first
	struct stepper_motion_controller_data motion_controller_data;
	// Motion event callback
	stepper_motion_event_callback_t event_callback;
	void *event_callback_user_data;
	// Current direction for stepping
	enum stepper_direction current_direction;
};

static void wrapper_step_callback(const struct device *dev)
{
	const struct stepper_motion_controller_wrapper_config *config = dev->config;
	struct stepper_motion_controller_wrapper_data *data = dev->data;
	
	/* Step the underlying hardware stepper with current direction */
	stepper_step(config->stepper_dev, data->current_direction);
}

static void wrapper_set_direction_callback(const struct device *dev,
					   enum stepper_direction direction)
{
	struct stepper_motion_controller_wrapper_data *data = dev->data;
	
	/* Store the direction for use in step callback */
	data->current_direction = direction;
}

static void wrapper_event_callback(const struct device *dev, enum stepper_motion_event event)
{
	struct stepper_motion_controller_wrapper_data *data = dev->data;

	if (data->event_callback) {
		data->event_callback(dev, event, data->event_callback_user_data);
	}
}

static const struct stepper_motion_controller_callbacks_api wrapper_motion_controller_callbacks = {
	.step = wrapper_step_callback,
	.set_direction = wrapper_set_direction_callback,
	.event = wrapper_event_callback,
};

static int wrapper_stepper_motion_set_event_callback(const struct device *dev,
					      stepper_motion_event_callback_t callback, void *user_data)
{
	struct stepper_motion_controller_wrapper_data *data = dev->data;

	data->event_callback = callback;
	data->event_callback_user_data = user_data;

	return 0;
}

// Motion control layer API (stepper_motion.h interface)
static DEVICE_API(stepper_motion, wrapper_stepper_motion_api) = {
	.move_by = stepper_motion_controller_move_by,
	.is_moving = stepper_motion_controller_is_moving,
	.set_position = stepper_motion_controller_set_position,
	.get_position = stepper_motion_controller_get_position,
	.move_to = stepper_motion_controller_move_to,
	.run = stepper_motion_controller_run,
	.stop = stepper_motion_controller_stop,
	.set_event_callback = wrapper_stepper_motion_set_event_callback,
	.set_ramp = stepper_motion_controller_set_ramp,
};

static int stepper_motion_controller_wrapper_init(const struct device *dev)
{
	const struct stepper_motion_controller_wrapper_config *config = dev->config;
	int ret;

	/* Ensure the underlying stepper device is ready */
	if (!device_is_ready(config->stepper_dev)) {
		LOG_ERR("Stepper device %s is not ready", config->stepper_dev->name);
		return -ENODEV;
	}

	/* Enable the underlying stepper device */
	ret = stepper_enable(config->stepper_dev);
	if (ret < 0) {
		LOG_ERR("Failed to enable stepper device: %d", ret);
		return ret;
	}

	/* Initialize the motion controller */
	ret = stepper_motion_controller_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to init motion controller: %d", ret);
		return ret;
	}

	return 0;
}

#define STEPPER_MOTION_CONTROLLER_WRAPPER_DEFINE(inst)                                               \
	STEPPER_TIMING_SOURCE_DT_INST_DEFINE(inst)                                                   \
	static const struct stepper_motion_controller_wrapper_config wrapper_config_##inst = {       \
		.motion_controller_config =                                                          \
			{                                                                            \
				.timing_source = STEPPER_TIMING_SOURCE_DT_INST_GET(inst),            \
				.callbacks = &wrapper_motion_controller_callbacks,                   \
			},                                                                           \
		.stepper_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, stepper),                    \
					   (DEVICE_DT_GET(DT_INST_PHANDLE(inst, stepper))),         \
					   (DEVICE_DT_GET(DT_INST_PARENT(inst)))),                  \
	};                                                                                           \
	static struct stepper_motion_controller_wrapper_data wrapper_data_##inst = {                \
		.motion_controller_data = {},                                                        \
	};                                                                                           \
	DEVICE_DT_INST_DEFINE(inst, stepper_motion_controller_wrapper_init, NULL,                   \
			      &wrapper_data_##inst, &wrapper_config_##inst, POST_KERNEL,            \
			      CONFIG_STEPPER_INIT_PRIORITY, &wrapper_stepper_motion_api);

#define DT_DRV_COMPAT zephyr_stepper_motion_controller

DT_INST_FOREACH_STATUS_OKAY(STEPPER_MOTION_CONTROLLER_WRAPPER_DEFINE)
