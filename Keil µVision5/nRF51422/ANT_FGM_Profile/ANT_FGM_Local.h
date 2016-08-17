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


#ifndef ANT_FGM_LOCAL_H__
#define ANT_FGM_LOCAL_H__

#include <stdint.h>
#include <stdbool.h>
#include "ANT_FGM.h"

/**
 * @addtogroup ant_sdk_profiles_hrm
 * @{
 */

/** @brief CGM Sensor control block. */
typedef struct
{
    uint8_t        toggle_bit;
    ant_fgm_page_t main_page_number;
    uint8_t        page_1_present;
    ant_fgm_page_t ext_page_number;
    uint8_t        message_counter;
} ant_fgm_sens_cb_t;

/**
 * @}
 */

#endif // ANT_FGM_LOCAL_H__
