//!
//! Copyright 2016 DessertHunter
//!
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

/** @file
 *
 * @defgroup ant_sdk_profiles_cgm *Provisory* Continuous Glucose Monitor (or FGM system) profile
 * @{
 * @ingroup ant_sdk_profiles
 * @brief This module implements the Continuous Glucose Monitor (or FGM system) profile.
 *
 */

#ifndef ANT_FGM_H__
#define ANT_FGM_H__

#include <stdint.h>
#include <stdbool.h>
#include "app_util.h"
#include "ant_parameters.h"
#include "ant_stack_handler_types.h"
#include "ant_channel_config.h"
#include "ANT_FGM_Pages.h"
#include "sdk_errors.h"

/*
Aus dem FIT_SDK Profile.xlsx:
1		antfs
11	bike_power
12	environment_sensor_legacy
15	multi_sport_speed_distance
16	control
17	fitness_equipment
18	blood_pressure
19	geocache_node
20	light_electric_vehicle
25	env_sensor
26	racquet
27	control_hub
31	muscle_oxygen
35	bike_light_main
36	bike_light_shared
38	exd
40	bike_radar
119	weight_scale
120	heart_rate
121	bike_speed_cadence
122	bike_cadence
123	bike_speed
124	stride_speed_distance

--> Gr��er als 126 l�sst ANTwareII nicht zu, also nehmen wir einfach die 99
*/
#define FGM_DEVICE_TYPE       99u                            ///< Device type use for *Provisory* ANT FGM (dezimal 240).
// TODO: #define FGM_ANTPLUS_RF_FREQ   0x39u                          ///< Frequency, decimal 57 (2457 MHz).
#define FGM_ANTPUBLIC_RF_FREQ         60u                    ///< *Provisory* ANT FGM RF Freq 2460 MHz


#define FGM_MSG_PERIOD_4Hz    0x1F86u                        ///< Message period, decimal 8070 (4.06 Hz).
#define FGM_MSG_PERIOD_2Hz    0x3F0Cu                        ///< Message period, decimal 16140 (2.03 Hz).
#define FGM_MSG_PERIOD_1Hz    0x7E18u                        ///< Message period, decimal 32280 (1.02 Hz).

#define FGM_EXT_ASSIGN        0x00                           ///< ANT ext assign.
#define FGM_DISP_CHANNEL_TYPE CHANNEL_TYPE_SLAVE_RX_ONLY     ///< Display FGM channel type.
#define FGM_SENS_CHANNEL_TYPE CHANNEL_TYPE_MASTER            ///< Sensor FGM channel type.

/**@brief Initialize an ANT channel configuration structure for the FGM profile (Display).
 *
 * @param[in]  NAME                 Name of related instance.
 * @param[in]  CHANNEL_NUMBER       Number of the channel assigned to the profile instance.
 * @param[in]  TRANSMISSION_TYPE    Type of transmission assigned to the profile instance.
 * @param[in]  DEVICE_NUMBER        Number of the device assigned to the profile instance.
 * @param[in]  NETWORK_NUMBER       Number of the network assigned to the profile instance.
 * @param[in]  FGM_MSG_PERIOD       Channel period in 32 kHz counts. The FGM profile supports only the following periods:
 *                                  @ref FGM_MSG_PERIOD_4Hz, @ref FGM_MSG_PERIOD_2Hz, @ref FGM_MSG_PERIOD_1Hz.
 */
#define FGM_DISP_CHANNEL_CONFIG_DEF(NAME,                               \
                                    CHANNEL_NUMBER,                     \
                                    TRANSMISSION_TYPE,                  \
                                    DEVICE_NUMBER,                      \
                                    NETWORK_NUMBER,                     \
                                    FGM_MSG_PERIOD)                     \
static const ant_channel_config_t   NAME##_channel_cgm_disp_config =    \
    {                                                                   \
        .channel_number    = (CHANNEL_NUMBER),                          \
        .channel_type      = FGM_DISP_CHANNEL_TYPE,                     \
        .ext_assign        = FGM_EXT_ASSIGN,                            \
        .rf_freq           = FGM_ANTPUBLIC_RF_FREQ,                     \
        .transmission_type = (TRANSMISSION_TYPE),                       \
        .device_type       = FGM_DEVICE_TYPE,                           \
        .device_number     = (DEVICE_NUMBER),                           \
        .channel_period    = (FGM_MSG_PERIOD),                          \
        .network_number    = (NETWORK_NUMBER),                          \
    }
#define FGM_DISP_CHANNEL_CONFIG(NAME) &NAME##_channel_cgm_disp_config

/**@brief Initialize an ANT channel configuration structure for the FGM profile (Sensor).
 *
 * @param[in]  NAME                 Name of related instance.
 * @param[in]  CHANNEL_NUMBER       Number of the channel assigned to the profile instance.
 * @param[in]  TRANSMISSION_TYPE    Type of transmission assigned to the profile instance.
 * @param[in]  DEVICE_NUMBER        Number of the device assigned to the profile instance.
 * @param[in]  NETWORK_NUMBER       Number of the network assigned to the profile instance.
 */
#define FGM_SENS_CHANNEL_CONFIG_DEF(NAME,                               \
                                    CHANNEL_NUMBER,                     \
                                    TRANSMISSION_TYPE,                  \
                                    DEVICE_NUMBER,                      \
                                    NETWORK_NUMBER)                     \
static const ant_channel_config_t   NAME##_channel_cgm_sens_config =    \
    {                                                                   \
        .channel_number    = (CHANNEL_NUMBER),                          \
        .channel_type      = FGM_SENS_CHANNEL_TYPE,                     \
        .ext_assign        = FGM_EXT_ASSIGN,                            \
        .rf_freq           = FGM_ANTPUBLIC_RF_FREQ,                     \
        .transmission_type = (TRANSMISSION_TYPE),                       \
        .device_type       = FGM_DEVICE_TYPE,                           \
        .device_number     = (DEVICE_NUMBER),                           \
        .channel_period    = FGM_MSG_PERIOD_4Hz,                        \
        .network_number    = (NETWORK_NUMBER),                          \
    }
#define FGM_SENS_CHANNEL_CONFIG(NAME) &NAME##_channel_cgm_sens_config

/**@brief Initialize an ANT profile configuration structure for the FGM profile (Sensor).
 *
 * @param[in]  NAME                 Name of related instance.
 * @param[in]  PAGE_1_PRESENT       Determines whether page 1 is included.
 * @param[in]  MAIN_PAGE_NUMBER     Determines the main data page (@ref ANT_FGM_PAGE_0 or @ref ANT_FGM_PAGE_4).
 * @param[in]  EVT_HANDLER          Event handler to be called for handling events in the FGM profile.
 */
#define FGM_SENS_PROFILE_CONFIG_DEF(NAME,                               \
                                    PAGE_1_PRESENT,                     \
                                    MAIN_PAGE_NUMBER,                   \
                                    EVT_HANDLER)                        \
static ant_fgm_sens_cb_t            NAME##_cgm_sens_cb;                 \
static const ant_fgm_sens_config_t  NAME##_profile_cgm_sens_config =    \
    {                                                                   \
        .page_1_present     = (PAGE_1_PRESENT),                         \
        .main_page_number   = (MAIN_PAGE_NUMBER),                       \
        .p_cb               = &NAME##_cgm_sens_cb,                      \
        .evt_handler        = (EVT_HANDLER),                            \
    }
#define FGM_SENS_PROFILE_CONFIG(NAME) &NAME##_profile_cgm_sens_config

/**@brief FGM page number type. */
typedef enum
{
    ANT_FGM_PAGE_0, ///< Main data page number 0.
    ANT_FGM_PAGE_1, ///< Background data page number 1. This page is optional.
    ANT_FGM_PAGE_2, ///< Background data page number 2.
    ANT_FGM_PAGE_3, ///< Background data page number 3.
    ANT_FGM_PAGE_4  ///< Main data page number 4.
} ant_fgm_page_t;

/**@brief FGM profile event type. */
typedef enum
{
    ANT_FGM_PAGE_0_UPDATED = ANT_FGM_PAGE_0, ///< Data page 0 has been updated (Display) or sent (Sensor).
    ANT_FGM_PAGE_1_UPDATED = ANT_FGM_PAGE_1, ///< Data page 0 and page 1 have been updated (Display) or sent (Sensor).
    ANT_FGM_PAGE_2_UPDATED = ANT_FGM_PAGE_2, ///< Data page 0 and page 2 have been updated (Display) or sent (Sensor).
    ANT_FGM_PAGE_3_UPDATED = ANT_FGM_PAGE_3, ///< Data page 0 and page 3 have been updated (Display) or sent (Sensor).
    ANT_FGM_PAGE_4_UPDATED = ANT_FGM_PAGE_4, ///< Data page 0 and page 4 have been updated (Display) or sent (Sensor).
} ant_fgm_evt_t;

// Forward declaration of the ant_fgm_profile_t type.
typedef struct ant_fgm_profile_s ant_fgm_profile_t;

/**@brief FGM event handler type. */
typedef void (* ant_fgm_evt_handler_t) (ant_fgm_profile_t *, ant_fgm_evt_t);

#include "ANT_FGM_Local.h"

/**@brief FGM sensor configuration structure. */
typedef struct
{
    bool                    page_1_present;   ///< Determines whether page 1 is included.
    ant_fgm_page_t          main_page_number; ///< Determines the main data page (@ref ANT_FGM_PAGE_0 or @ref ANT_FGM_PAGE_4).
    ant_fgm_sens_cb_t     * p_cb;             ///< Pointer to the data buffer for internal use.
    ant_fgm_evt_handler_t   evt_handler;      ///< Event handler to be called for handling events in the CGM profile.
} ant_fgm_sens_config_t;

/**@brief FGM profile structure. */
struct ant_fgm_profile_s
{
    uint8_t               channel_number; ///< Channel number assigned to the profile.
    union {
        void              * p_none;
        ant_fgm_sens_cb_t * p_sens_cb;
    } _cb;                                ///< Pointer to internal control block.
    ant_fgm_evt_handler_t evt_handler;    ///< Event handler to be called for handling events in the FGM profile.
    ant_fgm_page0_data_t  page_0;         ///< Page 0.
    ant_fgm_page1_data_t  page_1;         ///< Page 1.
    ant_fgm_page2_data_t  page_2;         ///< Page 2.
    ant_fgm_page3_data_t  page_3;         ///< Page 3.
    ant_fgm_page4_data_t  page_4;         ///< Page 4.
};

/** @name Defines for accessing ant_fgm_profile_t member variables
   @{ */
#define FGM_PROFILE_meas_sequence_num      page_0.sequence_number
#define FGM_PROFILE_meas_glucose           page_0.glucose_concentration
#define FGM_PROFILE_meas_time_offset       page_0.time_offset
#define FGM_PROFILE_meas_prediction        page_0.glucose_prediction
#define FGM_PROFILE_meas_climb_sink_rate   page_0.glucose_climb_sink_rate
#define FGM_PROFILE_operating_time         page_1.operating_time
#define FGM_PROFILE_nfc_state              page_1.nfc_state
#define FGM_PROFILE_manuf_id               page_2.manuf_id
#define FGM_PROFILE_serial_num             page_2.serial_num
#define FGM_PROFILE_hw_version             page_3.hw_version
#define FGM_PROFILE_sw_version             page_3.sw_version
#define FGM_PROFILE_model_num              page_3.model_num
#define FGM_PROFILE_manuf_spec             page_4.manuf_spec
/** @} */

/**@brief Function for initializing the ANT FGM Display profile instance.
 *
 * @param[in]  p_profile        Pointer to the profile instance.
 * @param[in]  p_channel_config Pointer to the ANT channel configuration structure.
 * @param[in]  evt_handler      Event handler to be called for handling events in the FGM profile.
 *
 * @retval     NRF_SUCCESS      If initialization was successful. Otherwise, an error code is returned.
 */
ret_code_t ant_fgm_disp_init(ant_fgm_profile_t          * p_profile,
                             ant_channel_config_t const * p_channel_config,
                             ant_fgm_evt_handler_t        evt_handler);

/**@brief Function for initializing the ANT FGM Sensor profile instance.
 *
 * @param[in]  p_profile        Pointer to the profile instance.
 * @param[in]  p_channel_config Pointer to the ANT channel configuration structure.
 * @param[in]  p_sens_config    Pointer to the FGM sensor configuration structure.
 *
 * @retval     NRF_SUCCESS      If initialization was successful. Otherwise, an error code is returned.
 */
ret_code_t ant_fgm_sens_init(ant_fgm_profile_t           * p_profile,
                             ant_channel_config_t const  * p_channel_config,
                             ant_fgm_sens_config_t const * p_sens_config);

/**@brief Function for opening the profile instance channel for ANT FGM Display.
 *
 * Before calling this function, pages should be configured.
 *
 * @param[in]  p_profile        Pointer to the profile instance.
 *
 * @retval     NRF_SUCCESS      If the channel was successfully opened. Otherwise, an error code is returned.
 */
ret_code_t ant_fgm_disp_open(ant_fgm_profile_t * p_profile);

/**@brief Function for opening the profile instance channel for ANT FGM Sensor.
 *
 * Before calling this function, pages should be configured.
 *
 * @param[in]  p_profile        Pointer to the profile instance.
 *
 * @retval     NRF_SUCCESS      If the channel was successfully opened. Otherwise, an error code is returned.
 */
ret_code_t ant_fgm_sens_open(ant_fgm_profile_t * p_profile);

/**@brief Function for handling the sensor ANT events.
 *
 * @details This function handles all events from the ANT stack that are of interest to the Heart Rate Monitor Sensor profile.
 *
 * @param[in]   p_profile       Pointer to the profile instance.
 * @param[in]   p_ant_event     Event received from the ANT stack.
 */
void ant_fgm_sens_evt_handler(ant_fgm_profile_t * p_profile, ant_evt_t * p_ant_event);

/**@brief Function for handling the display ANT events.
 *
 * @details This function handles all events from the ANT stack that are of interest to the Heart Rate Monitor Display profile.
 *
 * @param[in]   p_profile       Pointer to the profile instance.
 * @param[in]   p_ant_event     Event received from the ANT stack.
 */
void ant_fgm_disp_evt_handler(ant_fgm_profile_t * p_profile, ant_evt_t * p_ant_event);
#endif // ANT_FGM_H__
/** @} */
