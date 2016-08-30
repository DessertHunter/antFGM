/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#ifndef ANT_FGM_UTILS_H__
#define ANT_FGM_UTILS_H__

#include "app_util.h"
#include "nrf_assert.h"
#include "nrf.h"

/** @file
 *
 * @defgroup ant_sdk_profiles_cgm_utils Continuous Glucose Monitor (or CGM system) profile utilities
 * @{
 * @ingroup ant_sdk_profiles_cgm
 * @brief This module implements utilities for the Continuous Glucose Monitor (or CGM system) profile.
 *
 */

/**@brief Unit for CGM operating time.
 *
 * @details According to the ANT FGM specification, the operating time unit is 2 seconds.
 */
#define ANT_FGM_OPERATING_TIME_UNIT                 2u

/**@brief This macro should be used to get the seconds part of the operating time.
 */
#define ANT_FGM_OPERATING_SECONDS(OPERATING_TIME)   (((OPERATING_TIME) * ANT_FGM_OPERATING_TIME_UNIT) % 60)

/**@brief This macro should be used to get the minutes part of the operating time.
 */
#define ANT_FGM_OPERATING_MINUTES(OPERATING_TIME)   ((((OPERATING_TIME) * ANT_FGM_OPERATING_TIME_UNIT) / 60) % 60)

/**@brief This macro should be used to get the hours part of the operating time.
 */
#define ANT_FGM_OPERATING_HOURS(OPERATING_TIME)     ((((OPERATING_TIME) * ANT_FGM_OPERATING_TIME_UNIT) / (60 * 60)) % 24)

/**@brief This macro should be used to get the days part of the operating time.
 */
#define ANT_FGM_OPERATING_DAYS(OPERATING_TIME)      ((((OPERATING_TIME) * ANT_FGM_OPERATING_TIME_UNIT) / (60 * 60)) / 24)

/** @} */

#endif // ANT_FGM_UTILS_H__
