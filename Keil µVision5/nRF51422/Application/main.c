//!
//! Copyright 2016 DessertHunter
//!
/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
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

#define ENABLE_BLE_SRV_HRS  0 ///< BLE Heart Rate Service
#define ENABLE_BLE_SRV_GLS  0 ///< BLE Glucose Service
#define ENABLE_BLE_SRV_BAS  0 ///< BLE Battery Service

/** @file
 *
 * @defgroup ble_sdk_app_ant_hrs_main main.c
 * @{
 * @ingroup ble_sdk_app_ant_hrs
 * @brief HRM sample application using both BLE and ANT.
 *
 * The application uses the BLE Heart Rate Service (and also the Device Information
 * services), and the ANT HRM RX profile.
 *
 * It will open a receive channel which will connect to an ANT HRM TX profile device when the
 * application starts. The received data will be propagated to a BLE central through the
 * BLE Heart Rate Service.
 *
 * The ANT HRM TX profile device simulator SDK application
 * (Board\pca10003\ant\ant_hrm\hrm_tx_buttons) can be used as a peer ANT device. By changing
 * ANT_HRMRX_NETWORK_KEY to the ANT+ Network Key, the application will instead be able to connect to
 * an ANT heart rate belt.
 *
 * @note The ANT+ Network Key is available for ANT+ Adopters. Please refer to
 *       http://thisisant.com to become an ANT+ Adopter and access the key.
 *
 * @note This application is based on the BLE Heart Rate Service Sample Application
 *       (Board\nrf6310\ble\ble_app_hrs). Please refer to this application for additional
 *       documentation.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "nrf.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#if (ENABLE_BLE_SRV_HRS == 1)
#include "ble_hrs.h"
#endif // ENABLE_BLE_SRV_HRS
#if (ENABLE_BLE_SRV_GLS == 1)
#include "ble_gls.h"
#include "ble_racp.h"
#endif // ENABLE_BLE_SRV_GLS
#if (ENABLE_BLE_SRV_BAS == 1)
#include "ble_bas.h"
#endif // ENABLE_BLE_SRV_BAS
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "boards.h"
#include "sensorsim.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "device_manager.h"
#include "ant_parameters.h"
#include "ant_interface.h"
#include "pstorage.h"
#include "app_trace.h"
#include "app_util.h"
#include "app_scheduler.h"
#include "bsp.h"
#include "ant_error.h"
#include "ant_stack_config.h"
#include "ant_key_manager.h"
#include "ant_state_indicator.h"
#include "SEGGER_RTT.h"
#include "main_Secret.h"


#define CR95HF_IS_ATTACHED              1
#define CR95HF_SLEEP_ENABLED            1

#define ENABLE_BATTERY_MEASURE          1 ///< ADC battery measurement
#define ENABLE_BATTERY_MEASURE_SDK10    1 ///< ADC battery measurement SDK v10
#define ENABLE_BATTERY_MEASURE_SDK11    0 ///< ADC battery measurement SDK v11


#if (ENABLE_BATTERY_MEASURE == 1)
#include "nrf_adc.h"

#define BATTERY_LEVEL_MEAS_INTERVAL     APP_TIMER_TICKS(2000, APP_TIMER_PRESCALER) /**< Battery level measurement interval (ticks). */
APP_TIMER_DEF(m_battery_timer_id);

static void battery_level_update(void);
static void battery_level_meas_timeout_handler(void * p_context);
void adc_sample(void);

#if (ENABLE_BATTERY_MEASURE_SDK10 == 1)
#define ADC_RESOLUTION              ADC_CONFIG_RES_10bit         //Calibration is only performed for 10-bit ADC resolution
#endif // ENABLE_BATTERY_MEASURE_SDK10


#if (ENABLE_BATTERY_MEASURE_SDK11 == 1)
static void adc_event_handler(nrf_drv_adc_evt_t const * p_event);

#define ADC_BUFFER_SIZE                 6 /**< Size of buffer for ADC samples. */
static nrf_adc_value_t       adc_buffer[ADC_BUFFER_SIZE]; /**< ADC buffer. */
static uint8_t adc_event_counter = 0;

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   1200
#define ADC_PRE_SCALING_COMPENSATION    3 
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS  270
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
                                 ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / 1023) * ADC_PRE_SCALING_COMPENSATION)
#endif // ENABLE_BATTERY_MEASURE_SDK11
#endif // ENABLE_BATTERY_MEASURE


#if (CR95HF_IS_ATTACHED == 1)
#include "CR95HF.h"
#define CR95HF_DEFAULT_TIMEOUT_MS       1000
#define CR95HF_READ_MAX_RETRY_CNT       3
#define CR95HF_BSP_USER_STATE           BSP_INDICATE_USER_STATE_1   // Gruene LED
#endif // CR95HF_IS_ATTACHED

#include "LibreSensor.h"


#define APP_SCHED_MAX_EVT_SIZE          5
#define APP_SCHED_QUEUE_SIZE            5

#define ANT_FGM_APP_VERSION             102  /**< Application Version (Major=VERSION / 100; Minor=VERSION % 100) */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)


#define ENABLE_ANT_FGM                  1
#if (ENABLE_ANT_FGM == 1)
#include "ANT_FGM.h"
#endif // ENABLE_ANT_FGM


#define WAKEUP_BUTTON_ID                0                                            /**< Button used to wake up the application. */
#define BOND_DELETE_ALL_BUTTON_ID       1                                            /**< Button used for deleting all bonded centrals during startup. */


#define DEVICE_NAME                     "antFGM (V" STR(ANT_FGM_APP_VERSION) ")"     /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "DessertHunter@GitHub"                       /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                40                                           /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                          /**< The advertising timeout in units of seconds. */

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                            /**< Whether or not to include the service_changed characteristic. If not enabled, the server's database cannot be changed for the lifetime of the device */
#define APP_TIMER_PRESCALER             0                                            /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                            /**< Size of timer operation queues. */

#define SECOND_1_25_MS_UNITS            800                                          /**< Definition of 1 second, when 1 unit is 1.25 ms. */
#define SECOND_10_MS_UNITS              100                                          /**< Definition of 1 second, when 1 unit is 10 ms. */
#define MIN_CONN_INTERVAL               (SECOND_1_25_MS_UNITS / 2)                   /**< Minimum acceptable connection interval (0.5 seconds), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               (SECOND_1_25_MS_UNITS)                       /**< Maximum acceptable connection interval (1 second), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                            /**< Slave latency. */
#define CONN_SUP_TIMEOUT                (4 * SECOND_10_MS_UNITS)                     /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                            /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_TIMEOUT               30                                           /**< Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                  1                                            /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                            /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                         /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                            /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                            /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                           /**< Maximum encryption key size. */

#define DEAD_BEEF                       0xDEADBEEF                                   /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */


#if (ENABLE_ANT_FGM == 1)
#define FGM_CHANNEL_NUMBER       0x00    /**< Channel number assigned to FGM profile. */
#define FGM_DEVICE_NUMBER        123u    /**< Denotes the used ANT device number. 1 � 65535 (0 for searching) Set the Device Number parameter to zero to allow wildcard matching. Once the device number is learned, the receiving device should remember the number for future searches. */
//      FGM_DEVICE_TYPE          99u     /**< Selbst festgelegt */
#define FGM_TRANSMISSION_TYPE    1u      /**< Denotes the used ANT transmission type. */
// #define FGM_MSG_PERIOD_4Hz    0x1F86u ///< Default Message period, decimal 8070 (4.06 Hz).
#define FGM_NETWORK_NUMBER       0u      /**< Default Network Number */
#define FGM_ANTPUBLIC_RF_FREQ    60u     /**<*Provisory* ANT FGM RF Freq 2460 MHz */

#define FGM_MFG_ID               255u    /**< Manufacturer ID. he value 255 (0x00FF) has been reserved as a development ID and may be used by manufacturers that have not yet been assigned a value. */
#define FGM_SERIAL_NUMBER        0xFFFFu /**< Serial Number. */
#define FGM_HW_VERSION           5u      /**< HW Version. */
#define FGM_SW_VERSION           ANT_FGM_APP_VERSION   /**< SW Version. */
#define FGM_MODEL_NUMBER         2u      /**< Model Number. */

#endif // ENABLE_ANT_FGM


#if (ENABLE_ANT_FGM == 1)
/** @snippet [ANT FGM TX Instance] */
void ant_fgm_evt_handler(ant_fgm_profile_t * p_profile, ant_fgm_evt_t event);

FGM_SENS_CHANNEL_CONFIG_DEF(m_ant_fgm,
                            FGM_CHANNEL_NUMBER,
                            FGM_TRANSMISSION_TYPE,
                            FGM_DEVICE_NUMBER,
                            FGM_NETWORK_NUMBER);
FGM_SENS_PROFILE_CONFIG_DEF(m_ant_fgm,
                            true, // PAGE_1_PRESENT
                            ANT_FGM_PAGE_0,
                            ant_fgm_evt_handler);

static ant_fgm_profile_t m_ant_fgm;
/** @snippet [ANT FGM TX Instance] */
#endif // ENABLE_ANT_FGM


static uint16_t               m_conn_handle = BLE_CONN_HANDLE_INVALID;     /**< Handle of the current connection. */
static ble_gap_adv_params_t   m_adv_params;                                /**< Parameters to be passed to the stack when starting advertising. */

#if (ENABLE_BLE_SRV_HRS == 1)
static ble_hrs_t              m_hrs;                                       /**< Structure used to identify the heart rate service. */
#endif // ENABLE_BLE_SRV_HRS

#if (ENABLE_BLE_SRV_GLS == 1)
static ble_gls_t              m_gls;                                      /**< Structure used to identify the glucose service. */
#endif // ENABLE_BLE_SRV_GLS

static uint8_t                m_ant_network_key[] = ANT_PUBLIC_NETWORK_KEY; /**< ANT Public network key. */

#ifdef BONDING_ENABLE
static dm_application_instance_t        m_app_handle;                                /**< Application identifier allocated by device manager */
static bool                             m_app_initialized   = false;                 /**< Application initialized flag. */
#endif // BONDING_ENABLE

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
  bsp_indication_set(BSP_INDICATE_FATAL_ERROR);
  app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Start advertising.
 */
static void ble_advertising_start(void)
{
  uint32_t err_code;

  err_code = sd_ble_gap_adv_start(&m_adv_params);
  if (err_code != NRF_SUCCESS && err_code != NRF_ERROR_INVALID_STATE)
  {
    APP_ERROR_HANDLER(err_code);
  }

  err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
  APP_ERROR_CHECK(err_code);
}


#if (ENABLE_ANT_FGM == 1)
/**@brief Function for handling ANT FGM events.
 */
/** @snippet [ANT FGM simulator call] */
void ant_fgm_evt_handler(ant_fgm_profile_t * p_profile, ant_fgm_evt_t event)
{
    switch (event)
    {
        case ANT_FGM_PAGE_0_UPDATED:
            /* fall through */
        case ANT_FGM_PAGE_1_UPDATED:
            /* fall through */
        case ANT_FGM_PAGE_2_UPDATED:
            /* fall through */
        case ANT_FGM_PAGE_3_UPDATED:
            /* fall through */
        case ANT_FGM_PAGE_4_UPDATED:
          // TODO: ant_fgm_simulator_one_iteration(&m_ant_fgm_simulator);
            break;

        default:
            break;
    }
}
#endif // ENABLE_ANT_FGM


/**@brief Attempt to both open the ant channel and start ble advertising.
*/
static void ant_and_ble_adv_start(void)
{
  ble_advertising_start();

  // TODO: Hier Channel oeffnen
}


/**@brief Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
#if (ENABLE_BATTERY_MEASURE == 1)
  uint32_t err_code;
#endif // ENABLE_BATTERY_MEASURE
  
  // Initialize timer module
  APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);

  // Create timers.
 #if (ENABLE_BATTERY_MEASURE == 1)
  err_code = app_timer_create(&m_battery_timer_id,
                              APP_TIMER_MODE_REPEATED,
                              battery_level_meas_timeout_handler);
  APP_ERROR_CHECK(err_code);
#endif // ENABLE_BATTERY_MEASURE
}

/**@brief Function for starting application timers.
 */
static void application_timers_start(void)
{
#if (ENABLE_BATTERY_MEASURE == 1)
  uint32_t err_code;
#endif // ENABLE_BATTERY_MEASURE

  // Start application timers.
#if (ENABLE_BATTERY_MEASURE == 1)
  err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
  APP_ERROR_CHECK(err_code);
#endif // ENABLE_BATTERY_MEASURE
}


/**@brief GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void ble_gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Advertising functionality initialization.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void ble_advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    uint8_t       flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    ble_uuid_t adv_uuids[] =
    {
        {BLE_UUID_HEART_RATE_SERVICE,         BLE_UUID_TYPE_BLE},
        {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}
    };

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags                   = flags;
    advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = adv_uuids;

    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);

    // Initialise advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    m_adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
    m_adv_params.p_peer_addr = NULL;
    m_adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval    = APP_ADV_INTERVAL;
    m_adv_params.timeout     = APP_ADV_TIMEOUT_IN_SECONDS;
}


/**@brief Initialize services that will be used by the application.
 *
 * @details Initialize the Heart Rate and Device Information services.
 */
static void ble_services_init(void)
{
    uint32_t       err_code;

#if (ENABLE_BLE_SRV_HRS == 1)
    ble_hrs_init_t hrs_init;
    uint8_t        body_sensor_location;
#endif // ENABLE_BLE_SRV_HRS

#if (ENABLE_BLE_SRV_GLS == 1)
    ble_gls_init_t gls_init;
#endif // ENABLE_BLE_SRV_GLS

#if (ENABLE_BLE_SRV_BAS == 1)
    ble_bas_init_t bas_init;
#endif // ENABLE_BLE_SRV_BAS

    ble_dis_init_t dis_init;


#if (ENABLE_BLE_SRV_HRS == 1)
    // Initialize Heart Rate Service.
    body_sensor_location = BLE_HRS_BODY_SENSOR_LOCATION_FINGER;

    memset(&hrs_init, 0, sizeof(hrs_init));

    hrs_init.evt_handler                 = NULL;
    hrs_init.is_sensor_contact_supported = false;
    hrs_init.p_body_sensor_location      = &body_sensor_location;

    // Here the sec level for the Heart Rate Service can be changed/increased.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&hrs_init.hrs_hrm_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hrs_init.hrs_hrm_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hrs_init.hrs_hrm_attr_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&hrs_init.hrs_bsl_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hrs_init.hrs_bsl_attr_md.write_perm);

    err_code = ble_hrs_init(&m_hrs, &hrs_init);
    APP_ERROR_CHECK(err_code);
#endif // ENABLE_BLE_SRV_HRS

#if (ENABLE_BLE_SRV_GLS == 1)
    // Initialize Glucose Service - sample selection of feature bits.
    memset(&gls_init, 0, sizeof(gls_init));

    gls_init.evt_handler          = NULL;
    gls_init.error_handler        = service_error_handler;
    gls_init.feature              = 0;
    gls_init.feature             |= BLE_GLS_FEATURE_LOW_BATT;
    gls_init.feature             |= BLE_GLS_FEATURE_TEMP_HIGH_LOW;
    gls_init.feature             |= BLE_GLS_FEATURE_GENERAL_FAULT;
    gls_init.is_context_supported = false;

    err_code = ble_gls_init(&m_gls, &gls_init);
    APP_ERROR_CHECK(err_code);
#endif // ENABLE_BLE_SRV_GLS

#if (ENABLE_BLE_SRV_BAS == 1)
    // Initialize Battery Service.
    memset(&bas_init, 0, sizeof(bas_init));

    // Here the sec level for the Battery Service can be changed/increased.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init.battery_level_char_attr_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_report_read_perm);

    bas_init.evt_handler          = NULL;
    bas_init.support_notification = true;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 100;

    err_code = ble_bas_init(&m_bas, &bas_init);
    APP_ERROR_CHECK(err_code);
#endif // ENABLE_BLE_SRV_BAS

    // Initialize Device Information Service
    memset(&dis_init, 0, sizeof(dis_init));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, MANUFACTURER_NAME);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}

#if (ENABLE_BLE_SRV_HRS == 1)
/**@brief Connection Parameters Module handler.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in]   p_evt   Event received from the Connection Parameters Module.
 */
static void ble_hrm_on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    switch (p_evt->evt_type)
    {
        case BLE_CONN_PARAMS_EVT_FAILED:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Connection Parameters module error handler.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void ble_hrm_conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Initialize the Connection Parameters module.
 */
static void ble_hrm_conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = m_hrs.hrm_handles.cccd_handle;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = ble_hrm_on_conn_params_evt;
    cp_init.error_handler                  = ble_hrm_conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}
#endif // ENABLE_BLE_SRV_HRS

#if (ENABLE_BLE_SRV_GLS == 1)
/**@brief Function for initializing the Connection Parameters module.
 */
static void ble_gls_conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAM_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = ble_conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}
#endif // ENABLE_BLE_SRV_GLS


/**@brief Function for dispatching a ANT stack event to all modules with a ANT stack event handler.
 *
 * @details This function is called from the ANT Stack event interrupt handler after a ANT stack
 *          event has been received.
 *
 * @param[in] p_ant_evt  ANT stack event.
 */
void ant_evt_dispatch(ant_evt_t * p_ant_evt)
{
#if (ENABLE_ANT_FGM == 1)
    ant_fgm_sens_evt_handler(&m_ant_fgm, p_ant_evt);
#endif // ENABLE_ANT_FGM
    ant_state_indicator_evt_handler(p_ant_evt);
}

/**
 * @brief Function for ANT stack initialization.
 *
 * @details Initializes the SoftDevice and the ANT event interrupt.
 */
static void softdevice_setup(void)
{
    uint32_t err_code;

    // TODO: Bereits gemacht! err_code = softdevice_ant_evt_handler_set(ant_evt_dispatch);
    // TODO: Bereits gemacht!     APP_ERROR_CHECK(err_code);

    // TODO: err_code = softdevice_handler_init(NRF_CLOCK_LFCLKSRC, NULL, 0, NULL);
    // TODO: APP_ERROR_CHECK(err_code);

    err_code = ant_stack_static_config(); // set ant resource
    APP_ERROR_CHECK(err_code);

    //err_code = ant_plus_key_set(ANTPLUS_NETWORK_NUMBER);
    err_code = sd_ant_network_address_set(FGM_NETWORK_NUMBER, m_ant_network_key);
    APP_ERROR_CHECK(err_code);
}

#if (ENABLE_BLE_SRV_HRS == 1)
static void ble_hrs_update_heart_rate(const uint16_t computed_heart_rate)
{
   uint32_t err_code;

  // TODO: R-R interval
  // TODO: ble_hrs_rr_interval_add(&m_hrs, beat_time - prev_beat);

  // Notify the received heart rate measurement
  err_code = ble_hrs_heart_rate_measurement_send(&m_hrs, computed_heart_rate);
  if (
      (err_code != NRF_SUCCESS)
      &&
      (err_code != NRF_ERROR_INVALID_STATE)
      &&
      (err_code != BLE_ERROR_NO_TX_BUFFERS)
      &&
      (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
  )
  {
      APP_ERROR_HANDLER(err_code);
  }
}
#endif // ENABLE_BLE_SRV_HRS


/**@brief Application's Stack BLE event handler.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            err_code = bsp_indication_set(BSP_INDICATE_IDLE);
            APP_ERROR_CHECK(err_code);

            // Need to close the ANT channel to make it safe to write bonding information to flash
        // TODO: err_code = sd_ant_channel_close(ANT_HRMRX_ANT_CHANNEL);
        // TODO: APP_ERROR_CHECK(err_code);

            // Note: Bonding information will be stored, advertising will be restarted and the
            //       ANT channel will be reopened when ANT event CHANNEL_CLOSED is received.
            break;

#ifndef BONDING_ENABLE
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
                                                   BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
                                                   NULL,
                                                   NULL);
            APP_ERROR_CHECK(err_code);
            break;
#endif // BONDING_ENABLE

        case BLE_GAP_EVT_TIMEOUT:
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISING)
            {
                err_code = bsp_indication_set(BSP_INDICATE_IDLE);
                APP_ERROR_CHECK(err_code);
                // err_code = bsp_buttons_enable((1 << WAKEUP_BUTTON_ID) | (1 << BOND_DELETE_ALL_BUTTON_ID));
                // APP_ERROR_CHECK(err_code);

                // Go to system-off mode (this function will not return; wakeup will cause a reset)
              NRF_LOG_PRINTF("WARNING: BLE_GAP_EVT_TIMEOUT!\r\n");
                // TOOD_CS: NEIN WOLLEN WIR NICHT! err_code = sd_power_system_off();
                // TOOD_CS: NEIN WOLLEN WIR NICHT!  APP_ERROR_CHECK(err_code);
            }
            break;

#ifndef BONDING_ENABLE
            case BLE_GATTS_EVT_SYS_ATTR_MISSING:
                err_code = sd_ble_gatts_sys_attr_set(m_conn_handle,
                                                     NULL,
                                                     0,
                                                     BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS | BLE_GATTS_SYS_ATTR_FLAG_USR_SRVCS);
                APP_ERROR_CHECK(err_code);
                break;
#endif // BONDING_ENABLE

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Dispatches a stack event to all modules with a stack BLE event handler.
 *
 * @details This function is called from the Stack event interrupt handler after a stack BLE
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Stack Bluetooth event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
#ifdef BONDING_ENABLE
  dm_ble_evt_handler(p_ble_evt);
#endif // BONDING_ENABLE

#if (ENABLE_BLE_SRV_HRS == 1)
  ble_hrs_on_ble_evt(&m_hrs, p_ble_evt);
  ble_hrs_conn_params_on_ble_evt(p_ble_evt);
#endif // ENABLE_BLE_SRV_HRS

#if (ENABLE_BLE_SRV_GLS == 1)
  // TODO
#endif // ENABLE_BLE_SRV_GLS


  on_ble_evt(p_ble_evt);
}


#ifdef BONDING_ENABLE
/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    uint32_t err_code;
    uint32_t count;

    pstorage_sys_event_handler(sys_evt);

    // Verify if BLE bond data storage access is in progress or not.
    if (m_app_initialized == true)
    {
        err_code = pstorage_access_status_get(&count);
        if ((err_code == NRF_SUCCESS) && (count == 0))
        {
            ant_and_adv_start();
        }
    }
    else
    {
        m_app_initialized = true;
    }
}


/**@brief Function for handling the Device Manager events.
 *
 * @param[in]   p_evt   Data associated to the device manager event.
 */
static uint32_t device_manager_evt_handler(dm_handle_t const    * p_handle,
                                           dm_event_t const     * p_event,
                                           ret_code_t             event_result)
{
    APP_ERROR_CHECK(event_result);
    return NRF_SUCCESS;
}


/**@brief Function for the Device Manager initialization.
 */
static void device_manager_init(void)
{
    uint32_t                err_code;
    dm_init_param_t         init_data;
    dm_application_param_t  register_param;

    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    // Clear all bonded centrals if the Bonds Delete button is pushed.
    err_code = bsp_button_is_pressed(BOND_DELETE_ALL_BUTTON_ID,&(init_data.clear_persistent_data));
    APP_ERROR_CHECK(err_code);

    err_code = dm_init(&init_data);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));

    register_param.sec_param.timeout      = SEC_PARAM_TIMEOUT;
    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.mitm         = SEC_PARAM_MITM;
    register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);
}
#endif // BONDING_ENABLE


/**@brief BLE + ANT stack initialization.
 *
 * @details Initializes the SoftDevice and the stack event interrupt.
 */
static void ble_ant_stack_init(void)
{
    uint32_t                err_code;

    // Initialize SoftDevice
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);

    // Initialize BLE stack
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Subscribe for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Subscribe for ANT events.
    err_code = softdevice_ant_evt_handler_set(ant_evt_dispatch);
    APP_ERROR_CHECK(err_code);

#ifdef BONDING_ENABLE
    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
#endif // BONDING_ENABLE
}


/** @snippet [ANT HRM simulator call] */

/**
 * @brief Function for HRM profile initialization.
 *
 * @details Initializes the HRM profile and open ANT channel.
 */
static void profile_setup(void)
{
  uint32_t err_code;

#if (ENABLE_ANT_FGM == 1)
/** @snippet [ANT FGM Profile Setup] */
  err_code = ant_fgm_sens_init(&m_ant_fgm,
                               FGM_SENS_CHANNEL_CONFIG(m_ant_fgm),
                               FGM_SENS_PROFILE_CONFIG(m_ant_fgm));
  APP_ERROR_CHECK(err_code);

  m_ant_fgm.FGM_PROFILE_manuf_id   = FGM_MFG_ID;
  m_ant_fgm.FGM_PROFILE_serial_num = FGM_SERIAL_NUMBER;
  m_ant_fgm.FGM_PROFILE_hw_version = FGM_HW_VERSION;
  m_ant_fgm.FGM_PROFILE_sw_version = FGM_SW_VERSION;
  m_ant_fgm.FGM_PROFILE_model_num  = FGM_MODEL_NUMBER;

  err_code = ant_fgm_sens_open(&m_ant_fgm);
  APP_ERROR_CHECK(err_code);

  err_code = ant_state_indicator_channel_opened();
  APP_ERROR_CHECK(err_code);
  NRF_LOG_PRINTF("ANT FGM Channel is now open (%X)\r\n", err_code);
/** @snippet [ANT FGM Profile Setup] */
#endif // ENABLE_ANT_FGM
}


/**@brief Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code;

    // Wait for events
    err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}




#if (ENABLE_BATTERY_MEASURE == 1)
/**@brief Function for performing battery measurement and updating the Battery Level characteristic
 *        in Battery Service.
 */
static void battery_level_update(void)
{
  //battery_level = (uint8_t)sensorsim_measure(&m_battery_sim_state, &m_battery_sim_cfg);

  NRF_LOG_DEBUG("\r\n    Triggering battery level update...\r\n");
  app_sched_event_put(0,0,(app_sched_event_handler_t)adc_sample); //Put adc_sample function into the scheduler queue, which will then be executed in the main context (lowest priority) when app_sched_execute is called in the main loop
}


/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void battery_level_meas_timeout_handler(void * p_context)
{
  UNUSED_PARAMETER(p_context);
  battery_level_update();
}


#if (ENABLE_BATTERY_MEASURE_SDK10 == 1)
bool adc_calibrate(uint16_t adc_result, uint16_t * adc_result_calibrated, uint8_t adc_resolution)
{
  uint32_t uicr_value;
  uint8_t offset_error;
  uint8_t gain_error;
  bool is_calibrated;

  // Apply ADC calibration described on 
  // https://devzone.nordicsemi.com/question/21653/how-to-calibrate-the-nrf51-adc-to-correct-offset-and-gain-error/
  uicr_value = *(uint32_t *)0x10000024; // Read ADC gain and offset error from UICR
  offset_error = uicr_value;
  gain_error = uicr_value >> 8;

  if(adc_resolution == ADC_CONFIG_RES_10bit)
  {
    *adc_result_calibrated = adc_result * (1024 + gain_error) / 1024 + offset_error; //calibrate
    is_calibrated = true;
  }
  else
  {
    // Calibration method not valid for given ADC resolution. Return adc value uncalibrated.
    *adc_result_calibrated = adc_result;      
    is_calibrated = false;
  }
  return is_calibrated;
}

/* Interrupt handler for ADC data ready event */
void ADC_IRQHandler(void)
{
  uint16_t adc_result_calibrated;
  uint8_t adc_result[2];

  /* Clear dataready event */
  NRF_ADC->EVENTS_END = 0;

  // Attempt to calibrate the ADC result
  adc_calibrate(NRF_ADC->RESULT, &adc_result_calibrated, ADC_RESOLUTION);

  NRF_LOG_PRINTF("ADC result: %X\r\n", adc_result_calibrated); // log ADC reult on UART

  adc_result[0] = adc_result_calibrated;
  adc_result[1] = adc_result_calibrated >> 8;

  NRF_LOG_PRINTF("ADC result: %X%X\r\n", adc_result[0], adc_result[1]); // log ADC reult on UART

  // Release the external crystal
  sd_clock_hfclk_release();
}
#endif // ENABLE_BATTERY_MEASURE_SDK10

#if (ENABLE_BATTERY_MEASURE_SDK11 == 1)
/**
 * @brief ADC interrupt handler.
 */
static void adc_event_handler(nrf_drv_adc_evt_t const * p_event)
{
  uint32_t err_code;
  uint16_t adc_sum_value = 0;
  uint16_t adc_average_value;
  uint16_t adc_result_millivolts;
  uint8_t  adc_result_percent;

  sd_clock_hfclk_release(); // Release the external crystal

  adc_event_counter++;
  printf("    ADC event counter: %d\r\n", adc_event_counter);
  if (p_event->type == NRF_DRV_ADC_EVT_DONE)
  {
    uint32_t i;
    for (i = 0; i < p_event->data.done.size; i++)
    {
      printf("Sample value %d: %d\r\n", i+1, p_event->data.done.p_buffer[i]);
      adc_sum_value += p_event->data.done.p_buffer[i];                           //Sum all values in ADC buffer
    }
    adc_average_value = adc_sum_value / p_event->data.done.size;                   //Calculate average value from all samples in the ADC buffer
    printf("Average ADC value: %d\r\n", adc_average_value);

    adc_result_millivolts = ADC_RESULT_IN_MILLI_VOLTS(adc_average_value);          //Transform the average ADC value into millivolts value
    printf("ADC result in millivolts: %d\r\n", adc_result_millivolts);

    adc_result_percent = battery_level_in_percent(adc_result_millivolts);          //Transform the millivolts value into battery level percent.
    printf("ADC result in percent: %d\r\n", adc_result_percent);

#if (ENABLE_BLE_SRV_BAS == 1)
    //Send the battery level over BLE
    err_code = ble_bas_battery_level_update(&m_bas, adc_result_percent);           // Send the battery level over BLE
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) && 
        (err_code != BLE_ERROR_NO_TX_PACKETS) && 
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
    {
      APP_ERROR_HANDLER(err_code); //Assert on error
    }
#endif // ENABLE_BLE_SRV_BAS
  }
}
#endif // ENABLE_BATTERY_MEASURE_SDK11

/**
 * @brief Configure and initialize the ADC
 */
static void adc_config(void)
{
#if (ENABLE_BATTERY_MEASURE_SDK10 == 1)
  /* Enable interrupt on ADC sample ready event*/
  NRF_ADC->INTENSET = ADC_INTENSET_END_Msk;   
  sd_nvic_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);  
  sd_nvic_EnableIRQ(ADC_IRQn);

  NRF_ADC->CONFIG	= (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_Pos) /* Bits 17..16 : ADC external reference pin selection. */
                  | (ADC_CONFIG_PSEL_AnalogInput5 << ADC_CONFIG_PSEL_Pos)   /*!< Use analog input 2 as analog input. */
                  | (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos)        /*!< Use internal 1.2V bandgap voltage as reference for conversion. */
                  | (ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) /*!< Analog input specified by PSEL with no prescaling used as input for the conversion. */
                  | (ADC_CONFIG_RES_8bit << ADC_CONFIG_RES_Pos);            /*!< 8bit ADC resolution. */ 

  /* Enable ADC*/
  NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;
#endif // ENABLE_BATTERY_MEASURE_SDK10

#if (ENABLE_BATTERY_MEASURE_SDK11 == 1)
  ret_code_t ret_code;
  nrf_drv_adc_config_t adc_config = NRF_DRV_ADC_DEFAULT_CONFIG;                                          // Get default ADC configuration
  static nrf_drv_adc_channel_t adc_channel_config = NRF_DRV_ADC_DEFAULT_CHANNEL(NRF_ADC_CONFIG_INPUT_2); // Get default ADC channel configuration

  //Uncomment the following two lines to sample the supply voltage of the nRF51 directly (not from a pin)
  //adc_channel_config.config.config.input = NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD;
  //adc_channel_config.config.config.ain = NRF_ADC_CONFIG_INPUT_DISABLED;

  ret_code = nrf_drv_adc_init(&adc_config, adc_event_handler);              //Initialize the ADC
  APP_ERROR_CHECK(ret_code);

  nrf_drv_adc_channel_enable(&adc_channel_config);                          //Configure and enable an ADC channel
#endif // ENABLE_BATTERY_MEASURE_SDK11
}

/**
 * @brief Function to trigger ADC sampling
 */
void adc_sample(void)
{
#if (ENABLE_BATTERY_MEASURE_SDK10 == 1)
  uint32_t p_is_running = 0;
    
  // Start the HFCLK crystal for best ADC accuracy. It will increase current consumption.
  sd_clock_hfclk_request();
  while(! p_is_running) {
    //wait for the hfclk to be available
    sd_clock_hfclk_is_running((&p_is_running));
  }               
  NRF_ADC->TASKS_START = 1; //Start ADC sampling
#endif // ENABLE_BATTERY_MEASURE_SDK10
  
#if (ENABLE_BATTERY_MEASURE_SDK11 == 1)
  ret_code_t ret_code;
  uint32_t p_is_running = 0;

  ret_code = nrf_drv_adc_buffer_convert(adc_buffer, ADC_BUFFER_SIZE);       // Allocate buffer for ADC
  APP_ERROR_CHECK(ret_code);

  //Request the external high frequency crystal for best ADC accuracy. For lowest current consumption, don't request the crystal.
  sd_clock_hfclk_request();
  while(! p_is_running) {          //wait for the hfclk to be available
      sd_clock_hfclk_is_running((&p_is_running));
  }  

  for (uint32_t i = 0; i < ADC_BUFFER_SIZE; i++)
  {
      while((NRF_ADC->BUSY & ADC_BUSY_BUSY_Msk) == ADC_BUSY_BUSY_Busy) {}   //Wait until the ADC is finished sampling
      printf("Start sampling ... \r\n");
      nrf_drv_adc_sample();        // Trigger ADC conversion
  }
#endif // ENABLE_BATTERY_MEASURE_SDK11
}

#endif // ENABLE_BATTERY_MEASURE



/**@brief Application main function.
 */
int main(void)
{
  uint32_t err_code;

  NRF_LOG_DEBUG("\r\n***************************\r\n");
  NRF_LOG_DEBUG(DEVICE_NAME);
  NRF_LOG_DEBUG("\r\n***************************\r\n");

  // Initialize peripherals
  timers_init();

  // Initialize scheduler
  APP_SCHED_INIT(APP_SCHED_MAX_EVT_SIZE, APP_SCHED_QUEUE_SIZE);

  // Initialize S310 SoftDevice
  ble_ant_stack_init();

  err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS, APP_TIMER_TICKS(100, APP_TIMER_PRESCALER), NULL);
  APP_ERROR_CHECK(err_code);
  // err_code = bsp_buttons_enable((1 << WAKEUP_BUTTON_ID) | (1 << BOND_DELETE_ALL_BUTTON_ID));
  // APP_ERROR_CHECK(err_code);

  // Initialize Bluetooth stack parameters.
  ble_gap_params_init();
  ble_advertising_init();
  ble_services_init();

#if (ENABLE_BLE_SRV_HRS == 1)
  ble_hrm_conn_params_init();
#endif // ENABLE_BLE_SRV_HRS

#if (ENABLE_BLE_SRV_GLS == 1)
  ble_gls_conn_params_init();
#endif // ENABLE_BLE_SRV_GLS


  //utils_setup();
  softdevice_setup();

#if (ENABLE_ANT_FGM == 1)
  ant_state_indicator_init(m_ant_fgm.channel_number, FGM_SENS_CHANNEL_TYPE);
#endif // ENABLE_ANT_FGM

  profile_setup();


#ifdef BONDING_ENABLE
  uint32_t count;

  // Initialize device manager.
  device_manager_init();

  err_code = pstorage_access_status_get(&count);
  if ((err_code == NRF_SUCCESS) && (count == 0))
#endif // BONDING_ENABLE
  {
    ant_and_ble_adv_start();
  }

#if (CR95HF_IS_ATTACHED == 1)
  NRF_LOG_PRINTF("Init CR95HF!\r\n");
  UNUSED_VARIABLE(bsp_indication_set(CR95HF_BSP_USER_STATE));
  initCR95HF();
  wakeCR95HF(CR95HF_DEFAULT_TIMEOUT_MS); // wakeup if sleeping
  {
    CR95HF_IDN tChipIdentify;
    identifyCR95HF(&tChipIdentify, CR95HF_DEFAULT_TIMEOUT_MS);
    NRF_LOG_PRINTF("CR95HF identification '%s' (CRC=%X)\r\n", tChipIdentify.deviceID, tChipIdentify.romCRC);
  }
  UNUSED_VARIABLE(bsp_indication_set(BSP_INDICATE_USER_STATE_OFF));
#if (CR95HF_SLEEP_ENABLED == 1)
  NRF_LOG_PRINTF("CR95HF will sleep/hybernate\r\n");
#else
  NRF_LOG_PRINTF("CR95HF hypernate DEACTIVATED!\r\n");
#endif // CR95HF_SLEEP_ENABLED
#endif // CR95HF_IS_ATTACHED

#if (ENABLE_BATTERY_MEASURE == 1)
  adc_config(); // Initialize ADC
#endif // ENABLE_BATTERY_MEASURE

  // Start execution.
  application_timers_start();
  
  // Enter main loop.
  for (;;)
  {
    // Little TODO-List:
    // - CR95HF schlafen legen
    // - Fehler oder Error Rate? sozusagen ein Retry-Z�hler als Art Qualityidicator
    // - Fehler/Erfolgsverhältnis?
    // - Batteriestand
    // - Sensorrestlaufzeit bzw. Fehler wenn abgelaufen

#if (CR95HF_IS_ATTACHED == 1)
    UNUSED_VARIABLE(bsp_indication_set(CR95HF_BSP_USER_STATE));

    CR95HF_STATES nfc_state = getStateCR95HF();

#if (ENABLE_ANT_FGM == 1)
    // New state...
    m_ant_fgm.FGM_PROFILE_nfc_state = (uint8_t)nfc_state;
#endif // ENABLE_ANT_FGM

    if (nfc_state == CR95HF_STATE_UNKNOWN)
    {
      // try startup sequence with reset ...
      resetCR95HF(); // send a reset command just in case the CR95HF hasn't been powered off

      nrf_delay_ms(100);
    }
    else if (nfc_state == CR95HF_STATE_SLEEPING)
    {
      // wake up CR95HF
      NRF_LOG_PRINTF("CR95HF wake up...\r\n");
      if (!wakeCR95HF(CR95HF_DEFAULT_TIMEOUT_MS)) // otherwise no communication possible
      {
        NRF_LOG_PRINTF("CR95HF state was SLEEPING, but wakeup failed!\r\n");
      }
    }
    else if (nfc_state == CR95HF_STATE_ANSWERING)
    {
      // NFC chip is answering
      // now set to protocol mode
      protocolISO15693_CR95HF(CR95HF_PROTOCOL_ISO_15693_WAIT_FOR_SOF
                              | CR95HF_PROTOCOL_ISO_15693_10_MODULATION
                              | CR95HF_PROTOCOL_ISO_15693_CRC); // ISO 15693 settings --> 0x0D = Wait for SOF, 10% modulation, append CRC
    }
    else if ((nfc_state == CR95HF_STATE_PROTOCOL) || (nfc_state == CR95HF_STATE_TAG_IN_RANGE))
    {
      // Tags can be read now... let's try:
      static CR95HF_TAG stKnownNfcTag;
      CR95HF_TAG tFoundNfcTag;
      bool fIsNewFoungTag = false;
      if (inventoryISO15693_CR95HF(&tFoundNfcTag, CR95HF_DEFAULT_TIMEOUT_MS)) // sensor in range?
      {
        // Yes, Sensor found!
        if (0 != memcmp(stKnownNfcTag.uid, tFoundNfcTag.uid, sizeof(tFoundNfcTag.uid)))
        {
          // is new / changed
          NRF_LOG_PRINTF("new sensor found: UID=%#02x:%#02x:%#02x:%#02x:%#02x:%#02x:%#02x:%#02x!\r\n",
                         tFoundNfcTag.uid[0], tFoundNfcTag.uid[1], tFoundNfcTag.uid[2], tFoundNfcTag.uid[3], tFoundNfcTag.uid[4], tFoundNfcTag.uid[5], tFoundNfcTag.uid[6], tFoundNfcTag.uid[7]);
          stKnownNfcTag = tFoundNfcTag;
          fIsNewFoungTag = true;

#if (ENABLE_ANT_FGM == 1)
          // we use last two tag-uid bytes for our serial number
          m_ant_fgm.FGM_PROFILE_serial_num = (tFoundNfcTag.uid[6] << 8) + tFoundNfcTag.uid[7];
#endif // ENABLE_ANT_FGM


#if (ENABLE_BLE_SRV_HRS == 1)
          // BLE HRS Service HRM update:
          const uint16_t glucose_value = tFoundNfcTag.uid[7]; // Letzte Stelle vom Sensor nehmen
          ble_hrs_update_heart_rate(glucose_value);
#endif // ENABLE_BLE_SRV_HRS

#if (ENABLE_BLE_SRV_GLS == 1)
          // TODO
#endif // ENABLE_BLE_SRV_GLS
        }

        bool fReadAllOk = true;
        uint8_t au8SensorRawData[LIBRE_SENSOR_RAW_DATA_LENGTH];
        for (uint8_t adr = LIBRE_SENSOR_RAW_DATA_START_BLOCK; adr <= LIBRE_SENSOR_RAW_DATA_END_BLOCK; adr++)
        {
          int len = readSingleCR95HF(adr, &au8SensorRawData[(adr - LIBRE_SENSOR_RAW_DATA_START_BLOCK) * LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE], LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE, CR95HF_DEFAULT_TIMEOUT_MS, CR95HF_READ_MAX_RETRY_CNT);
          if (LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE != len)
          {
            // Error
            NRF_LOG_DEBUG("Error while reading sensor data!\r\n");
            fReadAllOk = false;
            break;
          }

          if (fIsNewFoungTag)
          {
            // DEBUG: print new Sensor data:
            NRF_LOG_PRINTF("block %#04d: %#02x %#02x %#02x %#02x %#02x %#02x %#02x %#02x\r\n", adr,
                           au8SensorRawData[(adr - LIBRE_SENSOR_RAW_DATA_START_BLOCK) * LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE+0],
                           au8SensorRawData[(adr - LIBRE_SENSOR_RAW_DATA_START_BLOCK) * LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE+1],
                           au8SensorRawData[(adr - LIBRE_SENSOR_RAW_DATA_START_BLOCK) * LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE+2],
                           au8SensorRawData[(adr - LIBRE_SENSOR_RAW_DATA_START_BLOCK) * LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE+3],
                           au8SensorRawData[(adr - LIBRE_SENSOR_RAW_DATA_START_BLOCK) * LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE+4],
                           au8SensorRawData[(adr - LIBRE_SENSOR_RAW_DATA_START_BLOCK) * LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE+5],
                           au8SensorRawData[(adr - LIBRE_SENSOR_RAW_DATA_START_BLOCK) * LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE+6],
                           au8SensorRawData[(adr - LIBRE_SENSOR_RAW_DATA_START_BLOCK) * LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE+7]);
          }
        }

        if (fReadAllOk)
        {
          // Ok, all nfc tag data read
          ST_LibreSensorData tSensorData;
          if (LibreSensor_ParseSensorData(tFoundNfcTag.uid, &au8SensorRawData[0], sizeof(au8SensorRawData), &tSensorData))
          {
            // Parsing ok
            if (fIsNewFoungTag)
            {
              NRF_LOG_PRINTF("sensor_id: '%s'\r\n", tSensorData.sensor_id);
            }

#if (CR95HF_SLEEP_ENABLED == 1)
            NRF_LOG_PRINTF("CR95HF will hybernate, gn8...\r\n");
            if (!hybernateCR95HF())
            {
              NRF_LOG_PRINTF("CR95HF hybernate failed!\r\n");
            }
#endif // CR95HF_SLEEP_ENABLED

#if (ENABLE_ANT_FGM == 1)
            // New measurement...
            m_ant_fgm.FGM_PROFILE_meas_glucose = tSensorData.current_glucose;
            m_ant_fgm.FGM_PROFILE_meas_time_offset++;
            m_ant_fgm.FGM_PROFILE_meas_prediction = tSensorData.trend_prediction;
            m_ant_fgm.FGM_PROFILE_meas_climb_sink_rate = tSensorData.glucose_climb_sink_rate;
            m_ant_fgm.FGM_PROFILE_meas_sequence_num++;
#endif // ENABLE_ANT_FGM
          }
        }

      }
    }
    else // illegal nfc_state
    {
      NRF_LOG_PRINTF("Error illegal nfc_state=%d, try reset!\r\n", nfc_state);

      // try startup sequence with reset ...
      resetCR95HF();
      nrf_delay_ms(5000);
    }
    UNUSED_VARIABLE(bsp_indication_set(BSP_INDICATE_USER_STATE_OFF));
#endif // CR95HF_IS_ATTACHED

    NRF_LOG_DEBUG(".");

    for (int i = 1; i < 10; i++)
    {
      UNUSED_VARIABLE(bsp_indication_set(BSP_INDICATE_USER_STATE_0));
      nrf_delay_ms(5);
      UNUSED_VARIABLE(bsp_indication_set(BSP_INDICATE_USER_STATE_OFF));
      nrf_delay_ms(1000);
      
      power_manage();
      app_sched_execute(); // Let scheduler execute whatever is in the scheduler queue
    }
  }
}

/**
 * @}
 */
