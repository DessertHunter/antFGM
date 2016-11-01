/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except in compliance
with this license.

Copyright (c) Dynastream Innovations Inc. 2015
All rights reserved.
*/
//!
//! Copyright 2016 DessertHunter
//!
/*
Based on n5_starterkit.h
Need #define BOARD_ANTFGM_N5150
Hardware: N5150M5CD
*/


#ifndef ANTFGM_N5150_H
#define ANTFGM_N5150_H

#include "nrf_gpio.h"

// LEDs definitions for antFGM
#define LEDS_NUMBER    1

// active high leds
#define LED_A        23  // LED A: BSP_INDICATE_USER_STATE_1  BSP_LED_1

#define LEDS_LIST { LED_A }

//#define BSP_LED_0      LED_DUMMY 
#define BSP_LED_1      LED_A      // BSP_INDICATE_USER_STATE_1

#define BSP_LED_0_MASK (0) // Dummy
#define BSP_LED_1_MASK (1<<BSP_LED_1)

#define LEDS_MASK      (BSP_LED_0_MASK | BSP_LED_1_MASK)
/* LEDs are lit when GPIO is high */
#define LEDS_INV_MASK  0x00000000

// there are no user buttons
#define BUTTONS_NUMBER 0
#define BUTTONS_LIST {}
#define BUTTONS_MASK   0x00000000

// there are no user switches  
#define SWITCHES_NUMBER 0
#define SWITCHES_MASK 0x00000000

// no UART peripheral. Dummy defines for compilation.
#define RX_PIN_NUMBER  0xFF
#define TX_PIN_NUMBER  0xFF
#define CTS_PIN_NUMBER 0xFF
#define RTS_PIN_NUMBER 0xFF

// Low frequency clock source to be used by the SoftDevice
#ifdef S210
#define NRF_CLOCK_LFCLKSRC      NRF_CLOCK_LFCLKSRC_XTAL_50_PPM
#else
#define NRF_CLOCK_LFCLKSRC      {.source        = NRF_CLOCK_LF_SRC_XTAL,            \
                                 .rc_ctiv       = 0,                                \
                                 .rc_temp_ctiv  = 0,                                \
                                 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_50_PPM}
#endif

#endif // ANTFGM_N5150_H
