/* Last content update: Unreleased (latest tag: v0.2.0) */
#ifndef AMCORE_UTILITY_H
#define AMCORE_UTILITY_H

/**
 * @file utility.h
 * @brief Public utility API and physical constants.
 */

#if defined(AMCORE_SHARED)
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(AMCORE_BUILDING_LIBRARY)
#define AMCORE_API __declspec(dllexport)
#else
#define AMCORE_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#define AMCORE_API __attribute__((visibility("default")))
#else
#define AMCORE_API
#endif
#else
#define AMCORE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status returned by an amcore operation.
 */
typedef enum amcore_status {
    AMCORE_STATUS_SUCCESS = 0,
    AMCORE_STATUS_INVALID_ARGUMENT,
    AMCORE_STATUS_OUT_OF_RANGE
} amcore_status;

/**
 * @brief Speed of light in vacuum, in metres per second.
 */
AMCORE_API extern const double speed_of_light_m_per_s;

/**
 * @brief Calculate the free-space wavelength for a frequency.
 *
 * @param frequency_hz Frequency in hertz; must be finite and greater than zero.
 * @param wavelength_m Receives the wavelength in metres on success.
 * @return AMCORE_STATUS_SUCCESS on success. If an argument is invalid or the
 *         result is not representable as a positive finite double, returns an
 *         error status and leaves @p wavelength_m unchanged.
 */
AMCORE_API amcore_status amcore_free_space_wavelength(
    double frequency_hz,
    double *wavelength_m);

#ifdef __cplusplus
}
#endif

#endif
