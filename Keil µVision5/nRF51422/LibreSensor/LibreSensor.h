#ifndef LIBRE_SENSOR_H
#define LIBRE_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "LibreSensor_Secret.h"


typedef enum {
	GLUCOSE_PREDICTION_UNKOWN = 0,
	GLUCOSE_PREDICTION_FALLING, 
	GLUCOSE_PREDICTION_FALLING_SLOW, 
	GLUCOSE_PREDICTION_CONSTANT, 
	GLUCOSE_PREDICTION_RISING_SLOW, 
	GLUCOSE_PREDICTION_RISING 
} E_GlucosePrediction;


#define LIBRE_SENSOR_ID_SIZE    11 // Länge Sensor-Identifizierungsnummer ohne Nullterminator. Beispiel: "0M00006NFC4"

typedef struct
{
	uint16_t current_glucose; // glucose value [mg/dL]
	int8_t glucose_climb_sink_rate; // glucose climb/sink rate [mg/dL/15min???] TODO
	uint16_t sensor_time; // sensor running time in minutes bis max. 0x5400 min
	E_GlucosePrediction trend_prediction; // @see E_GlucosePrediction, e.g. GLUCOSE_PREDICTION_FALLING_SLOW
	char sensor_id[LIBRE_SENSOR_ID_SIZE+1]; // Sensor-Identifizierungsnummer (Nullterminiert). Beispiel: "0M00006NFC4\0"
} ST_LibreSensorData;


bool LibreSensor_Init(void);

bool LibreSensor_ParseSensorData(const uint8_t tag_uid[8], const uint8_t* pu8DataBlock3To39, const unsigned int data_length, ST_LibreSensorData* pSensorDataOut);

#endif // LIBRE_SENSOR_H


