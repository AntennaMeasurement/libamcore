/* Last content update: Unreleased (latest tag: v0.2.0) */
#ifndef AMCORE_FARFIELD_H
#define AMCORE_FARFIELD_H

/**
 * @file farfield.h
 * @brief Public far-field antenna measurement API.
 */

#include <amcore/utility.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate the minimum far-field distance per IEEE 149-2021, 7.1.2.
 *
 * Uses the formula for the minimum far-field distance:
 * \f$ R_{ff} = \max(5D, 1.6 \lambda, 2D^2 / \lambda) \f$
 * where D is the largest antenna aperture length and lambda is the free-space 
 * wavelength.
 * 
 * @ref [1] IEEE 149-2021 Recommended Practice for Antenna Measurements, Section 7.1.2
 *
 * @param frequency_hz Frequency in hertz; must be finite and greater than zero.
 * @param aperture_length_m Largest antenna aperture length in metres; must be
 *        finite and greater than zero.
 * @param distance_m Receives the minimum far-field distance in metres on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT for
 *         invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the wavelength or distance is not
 *         representable as a positive finite double. On error, leaves
 *         @p distance_m unchanged.
 */
AMCORE_API amcore_status amcore_far_field_distance(
    double frequency_hz,
    double aperture_length_m,
    double *distance_m);

#ifdef __cplusplus
}
#endif

#endif
