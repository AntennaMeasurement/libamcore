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

/**
 * @brief Calculate the approximate line-of-sight range for a given height.
 *
 * Uses R = 3.57 * sqrt(height), with height in metres and range in kilometres.
 *
 * @param height_m Height above ground in metres; must be finite and
 *        non-negative.
 * @param range_km Receives the line-of-sight range in kilometres on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT for
 *         invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the result is not finite. On error,
 *         leaves @p range_km unchanged.
 */
AMCORE_API amcore_status amcore_far_field_line_of_sight_km(
    double height_m,
    double *range_km);

/**
 * @brief Calculate the approximate radio horizon for a given height.
 *
 * Uses R = 4.12 * sqrt(height), with height in metres and range in kilometres.
 *
 * @param height_m Height above ground in metres; must be finite and
 *        non-negative.
 * @param range_km Receives the radio horizon in kilometres on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT for
 *         invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the result is not finite. On error,
 *         leaves @p range_km unchanged.
 */
AMCORE_API amcore_status amcore_far_field_radio_horizon_km(
    double height_m,
    double *range_km);

/**
 * @brief Calculate free-space path loss under far-field conditions.
 *
 * Uses Lp = 32.45 + 20*log10(frequency_MHz * range_km) - tx_gain_dB -
 * rx_gain_dB. Gains are finite decibel values and may be negative.
 *
 * @param frequency_mhz Frequency in megahertz; must be finite and greater than
 *        zero.
 * @param range_km Separation in kilometres; must be finite and greater than
 *        zero.
 * @param transmit_gain_db Transmit antenna gain in decibels; must be finite.
 * @param receive_gain_db Receive antenna gain in decibels; must be finite.
 * @param path_loss_db Receives path loss in decibels on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT for
 *         invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the result is not finite. On error,
 *         leaves @p path_loss_db unchanged.
 */
AMCORE_API amcore_status amcore_far_field_path_loss_db(
    double frequency_mhz,
    double range_km,
    double transmit_gain_db,
    double receive_gain_db,
    double *path_loss_db);

#ifdef __cplusplus
}
#endif

#endif
