/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Andre Stefanov
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stepper_ramp.h"
#include <zephyr/drivers/stepper/stepper_ramp_trapezoidal.h>

#include <sys/errno.h>
#include <zephyr/logging/log.h>

#include "zephyr/sys_clock.h"

LOG_MODULE_REGISTER(stepper_ramp, CONFIG_STEPPER_LOG_LEVEL);

/**
 * Computes the integer square root of a given 64-bit unsigned integer using
 * the Babylonian method (also known as Heron's method) for approximation.
 *
 * This function returns the largest integer value whose square is less than
 * or equal to the input value.
 *
 * @param n The 64-bit unsigned integer input for which the integer square root
 *          is to be computed.
 * @return The largest integer that, when squared, does not exceed the input value.
 */
static uint32_t isqrt(const uint64_t n)
{
	if (n <= 1) {
		return (uint32_t)n;
	}

	uint64_t x = n;
	uint64_t y = (x + 1) / 2;

	while (y < x) {
		x = y;
		y = (x + n / x) / 2;
	}
	return (uint32_t)x;
}

static uint64_t avr446_start_interval(const uint32_t acceleration)
{
	if (acceleration == 0) {
		LOG_ERR("Error: Acceleration cannot be zero");
		return 0;
	}

	/* the value of (2 * factor * factor) may not overflow uint64_t but at the same time be as
	 * large as possible to ensure maximal possible precision of isqrt */
	const uint64_t factor = 3037000499ULL;

	/* Calculate the start velocity based on the acceleration
	 *
	 * Using the formula: t = f * sqrt(2 * d / a)
	 * where f = counter frequency, d = 1 step, a = acceleration
	 *
	 * This value will be used in approximation as described in AVR446 section 2.3.1
	 * The approximation introduces an error which has to be corrected by multiplying first
	 * interval by factor of 0.676 The resulting formula is:
	 *
	 * start_interval = f * sqrt(2 / acceleration) * 0.676
	 *
	 * Since division of integer 2 by acceleration is problematic without usage of floating
	 * points,the formula is rewritten as:
	 *
	* start_interval = f * sqrt(2 * factor * factor / acceleration) / factor */
	const uint64_t step_interval_in_ns = NSEC_PER_SEC * 676ULL / 1000ULL *
	                                     isqrt(2ULL * factor * factor / acceleration) / factor;

	return step_interval_in_ns;
}

static uint32_t avr446_acceleration_steps_needed(const uint32_t interval_in_ns,
                                                 const uint32_t acceleration)
{
	if (interval_in_ns == 0) {
		return 0;
	}

	return (NSEC_PER_SEC / interval_in_ns) * (NSEC_PER_SEC / interval_in_ns) / (
		       acceleration * 2);
}

static void avr446_calculate_next_accel_step(struct stepper_ramp_trapezoidal_data *data)
{
	data->accel_steps_left--;

	if (data->acceleration_idx++ == 0) {
		data->interval_calculation_rest = 0;
		data->current_interval = data->first_acceleration_interval;
		return;
	}

	const uint64_t numerator = 2 * data->current_interval + data->interval_calculation_rest;
	const uint64_t denominator = 4 * data->acceleration_idx;

	data->interval_calculation_rest = numerator % denominator;
	data->current_interval -= numerator / denominator;
}

static void avr446_calculate_next_pre_decel_step(struct stepper_ramp_trapezoidal_data *data)
{
	const uint64_t numerator = 2 * data->current_interval + data->interval_calculation_rest;
	const uint64_t denominator = 4 * (data->pre_decel_steps_left + data->decel_steps_left);

	data->interval_calculation_rest = numerator % denominator;
	data->current_interval += numerator / denominator;

	data->pre_decel_steps_left--;
}

static void avr446_calculate_next_decel_step(struct stepper_ramp_trapezoidal_data *data)
{
	if (--data->decel_steps_left == 0) {
		data->interval_calculation_rest = 0;
		data->current_interval = data->last_deceleration_interval;
		return;
	}

	const uint64_t numerator = 2 * data->current_interval + data->interval_calculation_rest;
	const uint64_t denominator = 4 * data->decel_steps_left;

	data->interval_calculation_rest = numerator % denominator;
	data->current_interval += numerator / denominator;
}

static uint64_t prepare_move(struct stepper_ramp_base *ramp,
                             const uint32_t step_count)
{
	const struct stepper_ramp_trapezoidal *self = (struct stepper_ramp_trapezoidal *)ramp;
	struct stepper_ramp_trapezoidal_profile *profile = self->profile;
	struct stepper_ramp_trapezoidal_data *data = self->data;

	if (!profile) {
		LOG_ERR("Error: Profile cannot be NULL");
		return -EINVAL;
	}

	LOG_DBG("Parameters: current_interval=%llu run_interval=%llu step_count=%u "
	        "acceleration_rate=%u deceleration_rate=%u",
	        data->current_interval,
	        profile->run_interval,
	        step_count,
	        profile->acceleration_rate,
	        profile->deceleration_rate);

	data->first_acceleration_interval = avr446_start_interval(profile->acceleration_rate);

	data->last_deceleration_interval = avr446_start_interval(profile->deceleration_rate);

	/* steps needed to stop from the current velocity */
	const uint32_t stop_lim = avr446_acceleration_steps_needed(
		data->current_interval, profile->deceleration_rate);

	/* steps needed to speed up from zero to requested velocity */
	const uint32_t accel_lim = avr446_acceleration_steps_needed(
		profile->run_interval, profile->acceleration_rate);

	/* steps needed to decelerate from the requested velocity to zero */
	const uint32_t decel_lim = avr446_acceleration_steps_needed(
		profile->run_interval, profile->deceleration_rate);

	if (data->current_interval != 0 && data->current_interval < profile->run_interval) {
		/* the requested velocity is slower than the current one, slow down */

		/* steps needed to decelerate from the current velocity to the requested one */
		data->pre_decel_steps_left = stop_lim - decel_lim;

		data->accel_steps_left = 0;
		data->acceleration_idx = 0;

		const uint32_t total_decel_steps =
			data->pre_decel_steps_left + data->decel_steps_left;
		if (total_decel_steps < step_count) {
			data->run_steps_left = step_count - total_decel_steps;
		} else {
			data->run_steps_left = 0;
		}

		data->acceleration_idx = accel_lim;

		data->decel_steps_left = decel_lim;
	}

	if (data->current_interval == 0 || data->current_interval > profile->run_interval) {
		/* the requested velocity is faster than the current one, speed up */

		data->pre_decel_steps_left = 0;

		/* steps needed to speed up from the current velocity to the requested one */
		data->accel_steps_left = accel_lim - stop_lim;

		if (data->accel_steps_left + decel_lim >= step_count) {
			data->decel_steps_left = step_count * profile->acceleration_rate /
			                         (profile->deceleration_rate +
			                          profile->acceleration_rate);
			data->accel_steps_left = step_count - data->decel_steps_left;
		} else {
			data->decel_steps_left = decel_lim;
		}

		data->run_steps_left = step_count - data->accel_steps_left - data->decel_steps_left;

		data->acceleration_idx = 0;
	}

	data->run_interval = profile->run_interval;

	LOG_DBG(
		"Distance Profile: pre_decel_steps=%d accel_steps=%d run_steps=%d decel_steps=%d "
		"for steps=%d",
		data->pre_decel_steps_left, data->accel_steps_left, data->run_steps_left,
		data->decel_steps_left, step_count);

	return data->pre_decel_steps_left
	       + data->accel_steps_left
	       + data->run_steps_left
	       + data->decel_steps_left;
}

static uint64_t prepare_stop(struct stepper_ramp_base *ramp)
{
	LOG_DBG("Prepare decelerated stop");

	const struct stepper_ramp_trapezoidal *self = (struct stepper_ramp_trapezoidal *)ramp;
	struct stepper_ramp_trapezoidal_profile *profile = self->profile;
	struct stepper_ramp_trapezoidal_data *data = self->data;

	if (!profile) {
		LOG_ERR("Error: Profile cannot be NULL");
		return -EINVAL;
	}

	/* The deceleration rate may not be zero */
	if (profile->deceleration_rate == 0) {
		LOG_ERR("Error: Deceleration rate cannot be zero");
		return -EINVAL;
	}

	const uint32_t deceleration_steps = avr446_acceleration_steps_needed(
		data->current_interval, profile->deceleration_rate);

	data->pre_decel_steps_left = 0;
	data->accel_steps_left = 0;
	data->run_steps_left = 0;
	data->run_interval = 0;
	data->decel_steps_left = deceleration_steps;

	return deceleration_steps;
}

static uint64_t get_next_interval(struct stepper_ramp_base *ramp)
{
	const struct stepper_ramp_trapezoidal *self = (struct stepper_ramp_trapezoidal *)ramp;
	struct stepper_ramp_trapezoidal_data *data = self->data;

	if (data->pre_decel_steps_left > 0) {
		avr446_calculate_next_pre_decel_step(data);
	} else if (data->accel_steps_left > 0) {
		avr446_calculate_next_accel_step(data);
	} else if (data->run_steps_left > 0) {
		data->run_steps_left--;
		data->current_interval = data->run_interval;
	} else if (data->decel_steps_left > 0) {
		avr446_calculate_next_decel_step(data);
	} else {
		/* movement finished */
		data->current_interval = 0;
	}

	return data->current_interval;
}

const struct stepper_ramp_api stepper_ramp_trapezoidal_api = {
	.prepare_move = prepare_move,
	.prepare_stop = prepare_stop,
	.get_next_interval = get_next_interval,
};
