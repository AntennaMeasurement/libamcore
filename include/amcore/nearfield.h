/* Last content update: Unreleased (latest tag: v0.2.0) */
#ifndef AMCORE_NEARFIELD_H
#define AMCORE_NEARFIELD_H

/**
 * @file nearfield.h
 * @brief Public near-field antenna measurement API.
 */

#include <amcore/utility.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate the recommended near-field measurement distance between
 *        the probe and the antenna aperture.
 *
 * Uses R = C * lambda. If @p coeff is zero, uses the default coefficient 5.0,
 * following IEEE 1720-2012, section 5.3.1.4. The coefficient must be finite
 * and non-negative.
 *
 * @param frequency_hz Frequency in hertz; must be finite and greater than zero.
 * @param coeff Separation coefficient; zero selects the default value 5.0.
 * @param separation_m Receives the separation distance in metres on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT
 *         for invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the result is not a positive finite
 *         double. On error, leaves @p separation_m unchanged.
 */
AMCORE_API amcore_status amcore_near_field_measurement_distance_m(
    double frequency_hz,
    double coeff,
    double *separation_m);

/**
 * @brief Calculate the ideal planar near-field step size.
 *
 * Uses half the free-space wavelength.
 *
 * @param frequency_hz Frequency in hertz; must be finite and greater than zero.
 * @param step_size_m Receives the ideal step size in metres on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT
 *         for invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the step size is not representable as a
 *         positive finite double. On error, leaves @p step_size_m unchanged.
 */
AMCORE_API amcore_status amcore_near_field_planar_ideal_step_size_m(
    double frequency_hz,
    double *step_size_m);

/**
 * @brief Calculate the ideal planar scan length for an angle of view.
 *
 * Uses L = 2 d tan(theta) + a + p, where a is the AUT (Antenna Under Test)
 * longest edge length, p is the probe's longest edge length, d is the
 * probe-to-aperture measurement distance, and theta is the desired angle of
 * view along one side.
 *
 * @param aut_longest_edge_length_m AUT longest edge length in metres; must be
 *        finite and non-negative.
 * @param probe_longest_edge_length_m Probe longest edge length in metres; must be
 *        finite and non-negative.
 * @param measurement_distance_m Probe-to-aperture measurement distance in
 *        metres; must be finite and greater than zero.
 * @param angle_of_view_deg Desired angle of view in degrees; must be finite
 *        and in the range [0, 90).
 * @param scan_length_m Receives the ideal scan length in metres on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT
 *         for invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the result is not finite. On error,
 *         leaves @p scan_length_m unchanged.
 */
AMCORE_API amcore_status amcore_near_field_planar_ideal_scan_length_m(
    double aut_longest_edge_length_m,
    double probe_longest_edge_length_m,
    double measurement_distance_m,
    double angle_of_view_deg,
    double *scan_length_m);

/**
 * @brief Calculate the ideal planar near-field angle of view.
 *
 * The scan region is assumed to be centered on the antenna. Uses
 * theta = atan2(scan_length - a - p, 2 d), converted to degrees, where a is
 * the AUT (Antenna Under Test) longest edge length and p is the probe's
 * longest edge length.
 *
 * @param aut_longest_edge_length_m AUT longest edge length in
 *        metres; must be finite and non-negative.
 * @param probe_longest_edge_length_m Probe longest edge length in metres; must be
 *        finite and non-negative.
 * @param measurement_distance_m Probe-to-aperture measurement distance in
 *        metres; must be finite and greater than zero.
 * @param scan_length_m Scan length in metres; must be finite and non-negative.
 * @param angle_of_view_deg Receives the angle of view in degrees on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT
 *         for invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the result is not finite. On error,
 *         leaves @p angle_of_view_deg unchanged.
 */
AMCORE_API amcore_status amcore_near_field_planar_ideal_angle_of_view_deg(
    double aut_longest_edge_length_m,
    double probe_longest_edge_length_m,
    double measurement_distance_m,
    double scan_length_m,
    double *angle_of_view_deg);

/**
 * @brief Calculate centered planar near-field sampling parameters within millimeter resolution.
 *
 * The half-wavelength spacing is rounded down to whole millimetres. The
 * requested scan length is rounded up to whole millimetres, converted to a
 * half-length rounded up to a whole millimetre, then extended to a multiple
 * of the sampling spacing. The resulting scan positions are symmetric about
 * zero and the sample count includes both endpoints.
 *
 * @param frequency_hz Frequency in hertz; must be finite and greater than zero.
 * @param minimum_scan_length_m Desired minimum scan length in metres; must be
 *        finite and non-negative.
 * @param step_size_mm Receives calculated step size in millimetres.
 * @param scan_length_m Receives the calculated scan length in metres.
 * @param scan_start_m Receives the negative scan endpoint in metres.
 * @param scan_stop_m Receives the positive scan endpoint in metres.
 * @param sample_count Receives the number of samples.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT
 *         for invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the spacing rounds to zero or the
 *         parameters cannot be represented. On error, leaves all outputs
 *         unchanged.
 */
AMCORE_API amcore_status amcore_near_field_planar_sampling_parameters(
    double frequency_hz,
    double minimum_scan_length_m,
    size_t *step_size_mm,
    double *scan_length_m,
    double *scan_start_m,
    double *scan_stop_m,
    size_t *sample_count);

/**
 * @brief Calculate the ideal cylindrical near-field Y scan length.
 *
 * Uses L_y = H_AUT + H_Probe + 2 (R_Probe - MRE cos(Az_max)) tan(El_max), with angles converted
 * from degrees, following the NSI2000 (Near-field Edition), Version 4,
 * Software Operating Manual.
 *
 * @param antenna_height_m Antenna height H_AUT in metres; finite and non-negative.
 * @param probe_longest_edge_m Probe longest edge H_Probe in metres; finite and
 *        non-negative.
 * @param probe_radius_m Probe radius R_Probe in metres; finite and greater
 *        than maximum_radial_extent_m.
 * @param maximum_radial_extent_m Maximum radial extent MRE in metres; finite
 *        and non-negative.
 * @param azimuth_max_deg Maximum azimuth in degrees; finite and in [0, 180].
 * @param elevation_max_deg Maximum elevation in degrees; finite and in [0, 90).
 * @param scan_length_m Receives the scan length in metres on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT
 *         for invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the result is not finite.
 *         On error, leaves the output unchanged.
 */
AMCORE_API amcore_status amcore_near_field_cylindrical_ideal_y_scan_length_m(
    double antenna_height_m,
    double probe_longest_edge_m,
    double probe_radius_m,
    double maximum_radial_extent_m,
    double azimuth_max_deg,
    double elevation_max_deg,
    double *scan_length_m);

/**
 * @brief Calculate the ideal cylindrical near-field Y step size.
 *
 * Uses half the free-space wavelength without rounding. The step size is
 * independent of the scan length.
 *
 * @param frequency_hz Frequency in hertz; must be finite and greater than zero.
 * @param step_size_m Receives the Y step size in metres on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT
 *         for invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the wavelength or step size is not
 *         representable as a positive finite double. On error, leaves
 *         @p step_size_m unchanged.
 */
AMCORE_API amcore_status amcore_near_field_cylindrical_ideal_y_step_size_m(
    double frequency_hz,
    double *step_size_m);

/**
 * @brief Calculate the spherical near-field maximum radius extent.
 *
 * Uses the Euclidean norm of the maximum displacements along the three axes.
 *
 * @param displacement_x_m Maximum displacement from the rotation centre along
 *        the x-axis in metres; must be finite.
 * @param displacement_y_m Maximum displacement from the rotation centre along
 *        the y-axis in metres; must be finite.
 * @param displacement_z_m Maximum displacement from the rotation centre along
 *        the z-axis in metres; must be finite.
 * @param extent_m Receives the maximum radius extent in metres on success.
 * @return AMCORE_STATUS_SUCCESS on success, AMCORE_STATUS_INVALID_ARGUMENT
 *         for invalid inputs (including a null output pointer), or
 *         AMCORE_STATUS_OUT_OF_RANGE if the result is not finite. On error,
 *         leaves @p extent_m unchanged.
 */
AMCORE_API amcore_status amcore_near_field_spherical_maximum_radius_extent_m(
    double displacement_x_m,
    double displacement_y_m,
    double displacement_z_m,
    double *extent_m);

#ifdef __cplusplus
}
#endif

#endif
