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
#ifndef ANT_FGM_PAGE_0_H__
#define ANT_FGM_PAGE_0_H__

/** @file
 *
 * @defgroup ant_sdk_profiles_hrm_page0 FGM profile page 0
 * @{
 * @ingroup ant_sdk_profiles_hrm_pages
 */

#include <stdint.h>

/**@brief Data structure for FGM data page 0.
 *
 * This structure is used as a common page.
 */
typedef struct
{
    uint8_t   sequence_number;         ///< Sequence number. Starts at 0, increments for each new reading.
    uint16_t  time_offset;             ///< Seconds to offset from Base Time value.
    uint16_t  glucose_concentration;   ///< Measured glucose concentration.
	  uint8_t   glucose_prediction;      ///< Measured glucose trending prediction
	  int8_t    glucose_climb_sink_rate; ///< Calculated glucose trending sink/climb rate
} ant_fgm_page0_data_t;

/**@brief Initialize page 0.
 */
#define DEFAULT_ANT_FGM_PAGE0()     \
    (ant_fgm_page0_data_t)          \
    {                               \
        .sequence_number       = 0, \
        .time_offset           = 0, \
        .glucose_concentration = 0, \
			  .glucose_prediction    = 0, \
			  .glucose_climb_sink_rate = 0, \
    }

/**@brief Function for encoding page 0.
 *
 * @param[in]  p_page_data      Pointer to the page data.
 * @param[out] p_page_buffer    Pointer to the data buffer.
 */
void ant_fgm_page_0_encode(uint8_t                    * p_page_buffer,
                           ant_fgm_page0_data_t const * p_page_data);

/**@brief Function for decoding page 0.
 *
 * @param[in]  p_page_buffer    Pointer to the data buffer.
 * @param[out] p_page_data      Pointer to the page data.
 */
void ant_fgm_page_0_decode(uint8_t const        * p_page_buffer,
                           ant_fgm_page0_data_t * p_page_data);

#endif // ANT_FGM_PAGE_0_H__
/** @} */
