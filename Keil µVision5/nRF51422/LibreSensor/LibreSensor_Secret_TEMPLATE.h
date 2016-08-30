#ifndef LIBRE_SENSOR_SECRET_H
#define LIBRE_SENSOR_SECRET_H

#include <stdint.h>
#include <stdbool.h>

#error LIBRE_SENSOR_* Defines not set!
#define LIBRE_SENSOR_RAW_DATA_START_BLOCK    0
#define LIBRE_SENSOR_RAW_DATA_END_BLOCK      0
#define LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE     0
#define LIBRE_SENSOR_RAW_DATA_LENGTH         (LIBRE_SENSOR_RAW_DATA_BLOCK_SIZE * (1 + LIBRE_SENSOR_RAW_DATA_END_BLOCK - LIBRE_SENSOR_RAW_DATA_START_BLOCK))

#define LIBRE_SENSOR_SAMPLE_SIZE             0 // Each data sample is of size X bytes

uint16_t LibreSensor_CalculateGlucoseReading(const uint8_t* pu8Data, const unsigned int offsetLowByte);

#endif // LIBRE_SENSOR_SECRET_H
