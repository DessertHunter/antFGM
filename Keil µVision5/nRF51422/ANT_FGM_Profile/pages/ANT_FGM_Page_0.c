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
#include "ANT_FGM_Page_0.h"
#include "ANT_FGM_Utils.h"
#include "ANT_FGM_Page_Logger.h"

/**@brief FGM page 0 data layout structure. */
typedef struct
{
	uint8_t glucose_climb_sink_rate; // TODO: signed
	uint8_t glucose_prediction;
	uint8_t time_offset_LSB;
	uint8_t time_offset_MSB;
	uint8_t sequence_number;
	uint8_t glucose_LSB;
	uint8_t glucose_MSB;
}ant_fgm_page0_data_layout_t; // 7 Byte

/**@brief Function for tracing page 0 and comhmon data.
 *
 * @param[in]  p_common_data    Pointer to the common data.
 * @param[in]  p_page_data      Pointer to the page 0 data.
 */
static void page0_data_log(ant_fgm_page0_data_t const * p_page_data)
{
    LOG_PAGE0("Measurement sequence number:      %u\n\r",     (unsigned int)p_page_data->sequence_number);
    LOG_PAGE0("Measurement glucose:              %u\n\r",     (unsigned int)p_page_data->glucose_concentration);
    LOG_PAGE0("Measurement time offset:          %03us\n\r.", (unsigned int)p_page_data->time_offset);
}


void ant_fgm_page_0_encode(uint8_t                    * p_page_buffer,
                           ant_fgm_page0_data_t const * p_page_data)
{
    ant_fgm_page0_data_layout_t * p_outcoming_data = (ant_fgm_page0_data_layout_t *)p_page_buffer;
    uint16_t                      time_offset        = p_page_data->time_offset;
    uint16_t                      glucose            = p_page_data->glucose_concentration;

    p_outcoming_data->glucose_climb_sink_rate = (uint8_t)p_page_data->glucose_climb_sink_rate;
	  p_outcoming_data->glucose_prediction = (uint8_t)p_page_data->glucose_prediction;
    p_outcoming_data->time_offset_LSB  = (uint8_t)(time_offset & UINT8_MAX);
    p_outcoming_data->time_offset_MSB  = (uint8_t)((time_offset >> 8) & UINT8_MAX);
    p_outcoming_data->sequence_number  = (uint8_t)p_page_data->sequence_number;
    p_outcoming_data->glucose_LSB      = (uint8_t)(glucose & UINT8_MAX);
    p_outcoming_data->glucose_MSB      = (uint8_t)((glucose >> 8) & UINT8_MAX);

    page0_data_log(p_page_data);
}


void ant_fgm_page_0_decode(uint8_t const        * p_page_buffer,
                           ant_fgm_page0_data_t * p_page_data)
{
    ant_fgm_page0_data_layout_t const * p_incoming_data =
        (ant_fgm_page0_data_layout_t *)p_page_buffer;

    uint16_t time_offset = (uint16_t)((p_incoming_data->time_offset_MSB << 8)
                                    + p_incoming_data->time_offset_LSB);

		p_page_data->glucose_climb_sink_rate = p_incoming_data->glucose_climb_sink_rate;
		p_page_data->glucose_prediction      = p_incoming_data->glucose_prediction;
    p_page_data->sequence_number         = p_incoming_data->sequence_number;
    p_page_data->glucose_concentration   = (uint16_t)((p_incoming_data->glucose_MSB << 8)
                                         + p_incoming_data->glucose_LSB);
    p_page_data->time_offset             = time_offset;

    page0_data_log(p_page_data);
}
