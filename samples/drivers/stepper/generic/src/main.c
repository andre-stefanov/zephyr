/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper, CONFIG_STEPPER_LOG_LEVEL);

static const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper));

enum stepper_mode {
	STEPPER_MODE_ENABLE,
	STEPPER_MODE_STEP_FORWARD,
	STEPPER_MODE_STEP_BACKWARD,
	STEPPER_MODE_DISABLE,

	STEPPER_MODE_COUNT,
};

static enum stepper_mode current_mode = STEPPER_MODE_DISABLE;
static K_SEM_DEFINE(stepper_generic_sem, 0, 1);

static void stepper_callback(const struct device *dev, const enum stepper_event event,
			     void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (event) {
	case STEPPER_EVENT_STALL_DETECTED:
		LOG_DBG("Stall detected");
		break;
	case STEPPER_EVENT_FAULT_DETECTED:
		LOG_DBG("Fault detected");
		break;
	default:
		break;
	}
}

static void button_pressed(struct input_event *event, void *user_data)
{
	ARG_UNUSED(user_data);

	if (event->value == 0 && event->type == INPUT_EV_KEY) {
		return;
	}

	current_mode = (current_mode + 1) % STEPPER_MODE_COUNT;
	k_sem_give(&stepper_generic_sem);
}

INPUT_CALLBACK_DEFINE(NULL, button_pressed, NULL);

int main(void)
{
	LOG_INF("Starting generic stepper sample");
	if (!device_is_ready(stepper)) {
		LOG_ERR("Device %s is not ready", stepper->name);
		return -ENODEV;
	}
	LOG_DBG("stepper is %p, name is %s", stepper, stepper->name);

	stepper_set_event_callback(stepper, stepper_callback, NULL);

	do {
		switch (current_mode) {
		case STEPPER_MODE_ENABLE:
			LOG_INF("mode: enable");
			stepper_enable(stepper);
			break;
		case STEPPER_MODE_STEP_FORWARD:
			LOG_INF("mode: step forward");
			for (int i = 0; i < 200; i++) {
				stepper_step(stepper, STEPPER_DIRECTION_POSITIVE);
				k_msleep(10);
			}
			break;
		case STEPPER_MODE_STEP_BACKWARD:
			LOG_INF("mode: step backward");
			for (int i = 0; i < 200; i++) {
				stepper_step(stepper, STEPPER_DIRECTION_NEGATIVE);
				k_msleep(10);
			}
			break;
		case STEPPER_MODE_DISABLE:
			LOG_INF("mode: disable");
			stepper_disable(stepper);
			break;
		default:
			LOG_ERR("Unknown mode");
			break;
		}
	} while (k_sem_take(&stepper_generic_sem, K_FOREVER) == 0);

	return 0;
}
