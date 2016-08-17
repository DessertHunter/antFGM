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
#include "nrf_assert.h"
#include "app_error.h"
#include "ant_interface.h"
#include "app_util.h"
#include "ANT_FGM.h"
#include "ANT_FGM_Utils.h"
#include "ANT_FGM_Page_Logger.h"
#include "app_error.h"

#define BACKGROUND_DATA_INTERVAL 64 /**< The number of main data pages sent between background data page.
                                         Background data page is sent every 65th message. */
#define TX_TOGGLE_DIVISOR        4  /**< The number of messages between changing state of toggle bit. */

/**@brief FGM message data layout structure. */
typedef struct
{
    ant_fgm_page_t page_number         : 7;
    uint8_t        toggle_bit          : 1;
    uint8_t        page_payload[7];
} ant_fgm_message_layout_t;

/**@brief Function for initializing the ANT FGM profile instance.
 *
 * @param[in]  p_profile        Pointer to the profile instance.
 * @param[in]  p_channel_config Pointer to the ANT channel configuration structure.
 *
 * @retval     NRF_SUCCESS      If initialization was successful. Otherwise, an error code is returned.
 */
static ret_code_t ant_fgm_init(ant_fgm_profile_t          * p_profile,
                               ant_channel_config_t const * p_channel_config)
{
    p_profile->channel_number = p_channel_config->channel_number;

    p_profile->page_0 = DEFAULT_ANT_FGM_PAGE0();
    p_profile->page_1 = DEFAULT_ANT_FGM_PAGE1();
    p_profile->page_2 = DEFAULT_ANT_FGM_PAGE2();
    p_profile->page_3 = DEFAULT_ANT_FGM_PAGE3();
    p_profile->page_4 = DEFAULT_ANT_FGM_PAGE4();

    LOG_FGM("ANT FGM channel %u init\n\r", p_profile->channel_number);
    return ant_channel_init(p_channel_config);
}


ret_code_t ant_fgm_disp_init(ant_fgm_profile_t          * p_profile,
                             ant_channel_config_t const * p_channel_config,
                             ant_fgm_evt_handler_t        evt_handler)
{
    ASSERT(p_profile != NULL);
    ASSERT(p_channel_config != NULL);
    ASSERT(evt_handler != NULL);

    p_profile->evt_handler = evt_handler;

    return ant_fgm_init(p_profile, p_channel_config);
}


ret_code_t ant_fgm_sens_init(ant_fgm_profile_t           * p_profile,
                             ant_channel_config_t const  * p_channel_config,
                             ant_fgm_sens_config_t const * p_sens_config)
{
    ASSERT(p_profile != NULL);
    ASSERT(p_channel_config != NULL);
    ASSERT(p_sens_config != NULL);
    ASSERT(p_sens_config->p_cb != NULL);
    ASSERT(p_sens_config->evt_handler != NULL);

    ASSERT((p_sens_config->main_page_number == ANT_FGM_PAGE_0)
           || (p_sens_config->main_page_number == ANT_FGM_PAGE_4));

    p_profile->evt_handler       = p_sens_config->evt_handler;
    p_profile->_cb.p_sens_cb     = p_sens_config->p_cb;

    ant_fgm_sens_cb_t * p_cgm_cb = p_profile->_cb.p_sens_cb;
    p_cgm_cb->page_1_present     = p_sens_config->page_1_present;
    p_cgm_cb->main_page_number   = p_sens_config->main_page_number;
    p_cgm_cb->ext_page_number    = p_cgm_cb->page_1_present ? ANT_FGM_PAGE_1 : ANT_FGM_PAGE_2;
    p_cgm_cb->message_counter    = 0;
    p_cgm_cb->toggle_bit         = true;

    return ant_fgm_init(p_profile, p_channel_config);
}


/**@brief Function for getting next page number to send.
 *
 * @param[in]  p_profile        Pointer to the profile instance.
 *
 * @return     Next page number.
 */
static ant_fgm_page_t next_page_number_get(ant_fgm_profile_t * p_profile)
{
    ant_fgm_sens_cb_t * p_cgm_cb = p_profile->_cb.p_sens_cb;
    ant_fgm_page_t      page_number;

    if (p_cgm_cb->message_counter == (BACKGROUND_DATA_INTERVAL))
    {
        page_number = p_cgm_cb->ext_page_number;

        p_cgm_cb->message_counter = 0;

        p_cgm_cb->ext_page_number++;

        if (p_cgm_cb->ext_page_number > ANT_FGM_PAGE_3)
        {
            p_cgm_cb->ext_page_number = p_cgm_cb->page_1_present ? ANT_FGM_PAGE_1 : ANT_FGM_PAGE_2;
        }
    }
    else
    {
        page_number = p_cgm_cb->main_page_number;
    }

    if (p_cgm_cb->message_counter % TX_TOGGLE_DIVISOR == 0)
    {
        p_cgm_cb->toggle_bit ^= 1;
    }

    p_cgm_cb->message_counter++;

    return page_number;
}


/**@brief Function for encoding FGM message.
 *
 * @note Assume to be call each time when Tx window will occur.
 */
static void sens_message_encode(ant_fgm_profile_t * p_profile, uint8_t * p_message_payload)
{
    ant_fgm_message_layout_t * p_cgm_message_payload =
        (ant_fgm_message_layout_t *)p_message_payload;
    ant_fgm_sens_cb_t * p_cgm_cb = p_profile->_cb.p_sens_cb;

    p_cgm_message_payload->page_number = next_page_number_get(p_profile);
    p_cgm_message_payload->toggle_bit  = p_cgm_cb->toggle_bit;

    LOG_FGM("FGM TX Page number:               %u\n\r", p_cgm_message_payload->page_number);

    ant_fgm_page_0_encode(p_cgm_message_payload->page_payload, &(p_profile->page_0)); // Page 0 is present in each message

    switch (p_cgm_message_payload->page_number)
    {
        case ANT_FGM_PAGE_0:
            // No implementation needed
            break;

        case ANT_FGM_PAGE_1:
            ant_fgm_page_1_encode(p_cgm_message_payload->page_payload, &(p_profile->page_1));
            break;

        case ANT_FGM_PAGE_2:
            ant_fgm_page_2_encode(p_cgm_message_payload->page_payload, &(p_profile->page_2));
            break;

        case ANT_FGM_PAGE_3:
            ant_fgm_page_3_encode(p_cgm_message_payload->page_payload, &(p_profile->page_3));
            break;

        case ANT_FGM_PAGE_4:
            ant_fgm_page_4_encode(p_cgm_message_payload->page_payload, &(p_profile->page_4));
            break;

        default:
            LOG_FGM("\r\n");
            return;
    }
    LOG_FGM("\r\n");
    p_profile->evt_handler(p_profile, (ant_fgm_evt_t)p_cgm_message_payload->page_number);
}


/**@brief Function for setting payload for ANT message and sending it.
 *
 * @param[in]  p_profile        Pointer to the profile instance.
 */
static void ant_message_send(ant_fgm_profile_t * p_profile)
{
	uint32_t err_code;
	uint8_t  p_message_payload[ANT_STANDARD_DATA_PAYLOAD_SIZE];

	sens_message_encode(p_profile, p_message_payload);
	err_code =
			sd_ant_broadcast_message_tx(p_profile->channel_number,
																	sizeof (p_message_payload),
																	p_message_payload);
	APP_ERROR_CHECK(err_code);
}


void ant_fgm_sens_evt_handler(ant_fgm_profile_t * p_profile, ant_evt_t * p_ant_event)
{
	if (p_ant_event->channel == p_profile->channel_number)
	{
		switch (p_ant_event->event)
		{
			case EVENT_TX:
					ant_message_send(p_profile);
					break;

			default:
					break;
		}
	}
}


ret_code_t ant_fgm_disp_open(ant_fgm_profile_t * p_profile)
{
    ASSERT(p_profile != NULL);

    LOG_FGM("ANT FGM channel %u open\n\r", p_profile->channel_number);
    return sd_ant_channel_open(p_profile->channel_number);
}


ret_code_t ant_fgm_sens_open(ant_fgm_profile_t * p_profile)
{
    ASSERT(p_profile != NULL);

    // Fill tx buffer for the first frame
    ant_message_send(p_profile);

    LOG_FGM("ANT FGM channel %u open\n\r", p_profile->channel_number);
    return sd_ant_channel_open(p_profile->channel_number);
}


/**@brief Function for decoding FGM message.
 *
 * @note Assume to be call each time when Rx window will occur.
 */
static void disp_message_decode(ant_fgm_profile_t * p_profile, uint8_t * p_message_payload)
{
    const ant_fgm_message_layout_t * p_cgm_message_payload =
        (ant_fgm_message_layout_t *)p_message_payload;

    LOG_FGM("FGM RX Page Number:               %u\n\r", p_cgm_message_payload->page_number);

    ant_fgm_page_0_decode(p_cgm_message_payload->page_payload, &(p_profile->page_0)); // Page 0 is present in each message

    switch (p_cgm_message_payload->page_number)
    {
        case ANT_FGM_PAGE_0:
            // No implementation needed
            break;

        case ANT_FGM_PAGE_1:
            ant_fgm_page_1_decode(p_cgm_message_payload->page_payload, &(p_profile->page_1));
            break;

        case ANT_FGM_PAGE_2:
            ant_fgm_page_2_decode(p_cgm_message_payload->page_payload, &(p_profile->page_2));
            break;

        case ANT_FGM_PAGE_3:
            ant_fgm_page_3_decode(p_cgm_message_payload->page_payload, &(p_profile->page_3));
            break;

        case ANT_FGM_PAGE_4:
            ant_fgm_page_4_decode(p_cgm_message_payload->page_payload, &(p_profile->page_4));
            break;

        default:
            LOG_FGM("\r\n");
            return;
    }
    LOG_FGM("\r\n");

    p_profile->evt_handler(p_profile, (ant_fgm_evt_t)p_cgm_message_payload->page_number);
}


void ant_fgm_disp_evt_handler(ant_fgm_profile_t * p_profile, ant_evt_t * p_ant_event)
{
    if (p_ant_event->channel == p_profile->channel_number)
    {
        ANT_MESSAGE * p_message = (ANT_MESSAGE *)p_ant_event->evt_buffer;

        switch (p_ant_event->event)
        {
            case EVENT_RX:

                if (p_message->ANT_MESSAGE_ucMesgID == MESG_BROADCAST_DATA_ID
                    || p_message->ANT_MESSAGE_ucMesgID == MESG_ACKNOWLEDGED_DATA_ID
                    || p_message->ANT_MESSAGE_ucMesgID == MESG_BURST_DATA_ID)
                {
                    disp_message_decode(p_profile, p_message->ANT_MESSAGE_aucPayload);
                }
                break;

            default:
                break;
        }
    }
}
