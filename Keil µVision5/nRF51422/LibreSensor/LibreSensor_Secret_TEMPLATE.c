#include "LibreSensor_Secret.h"


uint16_t LibreSensor_CalculateGlucoseReading(const uint8_t* pu8Data, const unsigned int offsetLowByte)
{
	float glucose;

	const uint16_t raw_val = ((uint16_t)pu8Data[offsetLowByte+1] << 8) + pu8Data[offsetLowByte];
  const int bitmask = 0xFFFF;

#error Go find the magic formula!
	glucose = (float)((raw_val & bitmask) - 0.00f) / 1.00f;

	return (uint16_t)glucose;
}
