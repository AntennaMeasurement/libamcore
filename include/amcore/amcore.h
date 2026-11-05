/* Last content update: Unreleased (latest tag: v0.2.0) */
#ifndef AMCORE_AMCORE_H
#define AMCORE_AMCORE_H

/**
 * @file amcore.h
 * @brief Umbrella header for the public amcore API.
 */

#include <amcore/utility.h>
#include <amcore/farfield.h>
#include <amcore/nearfield.h>

/**
 * @brief Return the library version as a static string.
 *
 * The returned string is owned by the library and must not be freed.
 */
AMCORE_API const char *amcore_version(void);

#endif
