#include "LibreSensor.h"
#include "nrf_log.h"
#include "app_error.h"
#include "app_util.h"
#include <string.h>
#include <stdlib.h> // strtoul
#include <stdio.h>
#include <math.h>


#define LIBRE_DEBUG      NRF_LOG_PRINTF


// Greetings to Arduino :)
// Constrains a number to be within a range.
// Parameters
//   x: the number to constrain, all data types
//   a: the lower end of the range, all data types
//   b: the upper end of the range, all data types
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

#define REAL float
//#define REAL double

static bool extractTrendAndHistory(const uint8_t* pu8DataBlock3To39, const unsigned int data_length);
static bool extractSensorId(const uint8_t tag_uid[8], char* pszSensorId);
static bool linearRegressionOfTrend(REAL* m, REAL* b);


static uint16_t history[32]; // 15min * 32 = 8h
static uint16_t trend[16]; // 16?



inline static REAL sqr(REAL x)
{
	return x*x;
}

#if 0==1
/*
// Lineare Regression
*/
static int linreg(int n, const REAL x[], const REAL y[], REAL* m, REAL* b, REAL* r)
{
	REAL   sumx = 0.0;   /* sum of x     */
	REAL   sumx2 = 0.0;  /* sum of x**2  */
	REAL   sumxy = 0.0;  /* sum of x * y */
	REAL   sumy = 0.0;   /* sum of y     */
	REAL   sumy2 = 0.0;  /* sum of y**2  */

	for (int i=0; i<n; i++)
	{
		sumx  += x[i];
		sumx2 += sqr(x[i]);
		sumxy += x[i] * y[i];
		sumy  += y[i];
		sumy2 += sqr(y[i]);
	}

	REAL denom = (n * sumx2 - sqr(sumx));
	if (denom == 0)
	{
		 // singular matrix. can't solve the problem.
		 *m = 0;
		 *b = 0;
		 *r = 0;
		 return 1;
	}

	*m = (n * sumxy  -  sumx * sumy) / denom;
	*b = (sumy * sumx2  -  sumx * sumxy) / denom;
	if (r!=NULL) {
		*r = (sumxy - sumx * sumy / n) /          /* compute correlation coeff     */
					sqrt((sumx2 - sqr(sumx)/n) *
					(sumy2 - sqr(sumy)/n));
	}

	return 0;
}

static int run_lin_reg_sample()
{
    int n = 6;
    REAL x[6]= {1, 2, 4,  5,  10, 20};
    REAL y[6]= {4, 6, 12, 15, 34, 68};

    REAL m,b,r;
    linreg(n,x,y,&m,&b,&r);
    printf("m=%g b=%g r=%g\n",m,b,r);
    return 0;
}
#endif

// Angepasste Lineare Regression f�r den Glukosetrend
// Ermittelt Steigung m und Verschiebung b f�r die Formel f(x) = m*x+b
// Verwendet die Trendpunkte als Punktewolke
static bool linearRegressionOfTrend(REAL* m, REAL* b)
{
	REAL   sumx = 0.0;   // sum of x
	REAL   sumx2 = 0.0;  // sum of x**2
	REAL   sumxy = 0.0;  // sum of x * y
	REAL   sumy = 0.0;   // sum of y
	REAL   sumy2 = 0.0;  // sum of y**2

	const int n = 16;
	STATIC_ASSERT((sizeof(trend) / sizeof(trend[0])) == n);

	//const REAL x[n] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}; // TODO: Vorbeechenbar! Kl�ren geht 0 ?

	// Unser Trick: Die letzten 15 Werte liegen in der Vergangenheit, also nehmen wir negative X-Werte
	//              dadurch ist der Geradenoffset automatisch unser linearisierter Wert und die Steigung ist unsere Sink-/Steigrate
	const REAL x[n] = {-15, -14, -13, -12, -11, -10, -9, -8, -7. -6, -5, -4, -3, -2, -1, 0}; // TODO: Vorbeechenbar! Kl�ren geht 0 ?


	for (int i=0; i<n; i++)
	{
		sumx  += x[i];
		sumx2 += sqr(x[i]);
		sumxy += x[i] * trend[i];
		sumy  += trend[i];
		sumy2 += sqr(trend[i]);
	}

	REAL denom = (n * sumx2 - sqr(sumx));
	if (denom == 0)
	{
		 // singular matrix. can't solve the problem.
		 *m = 0;
		 *b = 0;
		 return false;
	}

	*m = (n * sumxy - sumx * sumy) / denom;
	*b = (sumy * sumx2 - sumx * sumxy) / denom;

	return true;
}


//static const char* const ORIGINAL_BASE32_TABLE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567=";
static const char* const LIBRE_BASE32_TABLE = "0123456789ACDEFGHJKLMNPQRTUVWXYZ?"; // TODO: Letztes Zeichen " "???

/*
SensorID extrahieren (Die die auch das Messger�t f�r den aktiven Sensor anzeigt).
War nicht besonders schwer nur etwas seltsam, da Abbott eine Seriennummer gew�hlt hat,
die fast direkt aus der Tag-UID zu lesen ist, wenn man das Alphabet mit den Zahlen
kombiniert und dabei die Buchstaben B,I,O,S weg l�sst (also diese Zeichenfolge erh�lt:
0123456789ACDEFGHJKLMNPQRTUVWXYZ ).
Dann muss man nur noch die UID in 5 Bit Schritten lesen und den entsprechenden
Buchstaben aus der Liste w�hlen. Dann kommt noch eine 0 davor und man hat die Seriennummer.
Quelle: http://unendlichkeit.net/wordpress/?p=94
*/
// Quelle: http://pastebin.com/kwhzXB7m (UID Bytes gedreht)
// sensor_id wird mit 11 Zeichen + Nullterminator beschrieben!
static bool extractSensorId(const uint8_t tag_uid[8], char* sensor_id)
{
	// Converts the tag id of a Libre sensor to the 0M00... serial number that's written on it."""

	// Take last 6 bytes of the tag (6*8 = 48 bits)
	// and encode it into 10 characters (50 bits)
	// using the chartable above. Here's the bit
	// distribution from the tag bytes:
	// 22222 22233 33333 34444 44445 55555 55666 66666 77777 777--
	//   1     2     3     4     5     6     7     8     9     10
	sensor_id[0] = '0'; // Wohl immer eine 0
	sensor_id[1] = LIBRE_BASE32_TABLE[tag_uid[2] >> 3];
	sensor_id[2] = LIBRE_BASE32_TABLE[((tag_uid[2] & 0x07) << 2) | tag_uid[3] >> 6];
	sensor_id[3] = LIBRE_BASE32_TABLE[((tag_uid[3] >> 1) & 0x1F)];
	sensor_id[4] = LIBRE_BASE32_TABLE[((tag_uid[3] & 1) << 4) | tag_uid[4] >> 4];
	sensor_id[5] = LIBRE_BASE32_TABLE[((tag_uid[4] & 0x0F) << 1) | tag_uid[5] >> 7];
	sensor_id[6] = LIBRE_BASE32_TABLE[((tag_uid[5] >> 2) & 0x1F)];
	sensor_id[7] = LIBRE_BASE32_TABLE[((tag_uid[5] & 0x03) << 3) | tag_uid[6] >> 5];
	sensor_id[8] = LIBRE_BASE32_TABLE[tag_uid[6] & 0x1F];
	sensor_id[9] = LIBRE_BASE32_TABLE[tag_uid[7] >> 3];
	sensor_id[10] = LIBRE_BASE32_TABLE[(tag_uid[7] << 2) & 0x1F];
	sensor_id[11] = '\0'; // Nullterminator

	return true;
}

bool LibreSensor_Init(void)
{
  APP_ERROR_CHECK(NRF_LOG_INIT()); // Nur, falls noch nicht geschehen

	return true;
}


bool LibreSensor_ParseSensorData(const uint8_t tag_uid[8], const uint8_t* pu8DataBlock3To39, const unsigned int data_length, ST_LibreSensorData* pSensorDataOut)
{
	bool bRet = extractTrendAndHistory(pu8DataBlock3To39, data_length);

	// Sensor ID
	if (bRet)
	{
	  bRet = extractSensorId(tag_uid, pSensorDataOut->sensor_id);
	}

	// Trendauswertung
	REAL m, b;
	if (bRet)
	{
	  bRet = linearRegressionOfTrend(&m, &b);
	}

	if (bRet)
	{
		pSensorDataOut->current_glucose = constrain(b, 0, UINT16_MAX);
		pSensorDataOut->glucose_climb_sink_rate = constrain(m, INT8_MIN, INT8_MAX);

    // TODO; Ordentlich ;)		
		if ((m > 5.0) && (m < 20.0))
			pSensorDataOut->trend_prediction = GLUCOSE_PREDICTION_RISING;
		else if (m > 1.0)
			pSensorDataOut->trend_prediction = GLUCOSE_PREDICTION_RISING_SLOW;
		else if (m > -1.0)
			pSensorDataOut->trend_prediction = GLUCOSE_PREDICTION_CONSTANT;
		else if (m > -5.0)
			pSensorDataOut->trend_prediction = GLUCOSE_PREDICTION_FALLING_SLOW;
		else if ((m < -5.0) && (m > -20.0))
			pSensorDataOut->trend_prediction = GLUCOSE_PREDICTION_FALLING;
		else
			pSensorDataOut->trend_prediction = GLUCOSE_PREDICTION_UNKOWN;
	}
	else
	{
		pSensorDataOut->current_glucose = UINT16_MAX;
		pSensorDataOut->glucose_climb_sink_rate = 0;
	}

	return bRet;
}



// Urspruengliche Quelle: https://github.com/SandraK82/LibreRead-iOS/blob/master/LibreRead/AppDelegate.m
static bool extractTrendAndHistory(const uint8_t* pu8DataBlock3To39, const unsigned int data_length)
{
	if (data_length < LIBRE_SENSOR_RAW_DATA_LENGTH)
	{
		LIBRE_DEBUG("ERROR: Zu kleiner Puffer �bergeben!\r\n");
		return -1;
	}

	// Wichtige Start-Positionen ermitteln:
	// Data is separated in two parts: First part contains data of the last 15 minutes (probably for calculating the trend direction of the glucose level).
	const uint8_t index_trend = pu8DataBlock3To39[26-24]; // Byte 26 contains the block number that will be written next for the first part (15 minutes trend)
  const uint8_t index_history = pu8DataBlock3To39[27-24]; // Byte 27 contains the block number that will be written next for the second part (historic data)
	LIBRE_DEBUG("ring pos: i_trend=%d, i_history=%d\r\n", index_trend, index_history);

	// Sensortime: Steht an Adresse 0x013C (316) und 0x013D (317) also in Block-Adresse 39.
  //             Wir lesen ab Adresse 3, daher sind 3*8=24 abzuziehen, dann steht der Wert im letzten Block!
	//             sensor running time in minutes is stored as a word
	const uint16_t sensor_time = ((uint16_t)pu8DataBlock3To39[317-24] << 8) + pu8DataBlock3To39[316-24]; // bis max. 0x5400 min
  int days = sensor_time/(24*60);
  int hours = (sensor_time - (days*24*60)) / 60;
  int minutes = (sensor_time - (days*24*60)) - (hours*60);
  const int lifetime_min_left = 14 * 24 * 60 - sensor_time; // Achtung kann negativ werden
	LIBRE_DEBUG("Sensor-Laufzeit: %d Tag(e), %d h, %d min (Restzeit %d min)\r\n", days, hours, minutes, lifetime_min_left);
	LIBRE_DEBUG("sensor_time=%d\r\n", sensor_time);

	// Bloecke a 6 Byte

	// Trend: Some large zones don't change from sensor to sensor. And some data is definitely stored and updated in 6 bytes long records.
	// 15 Minutes trend data is found on bytes 28 to 123

	// 16 = (sizeof(trend) / sizeof(trend[0]))
	// 16 Eintraege je 1min???
	uint8_t cur_index_trend = index_trend;
	for (int i = 0; i < 16; i++)
	{
		unsigned int cur_offset_trend = (28-24) + (LIBRE_SENSOR_SAMPLE_SIZE * cur_index_trend); // Offset des aktuellen Samples berechnen
		trend[i] = LibreSensor_CalculateGlucoseReading(pu8DataBlock3To39, cur_offset_trend);
		LIBRE_DEBUG("trend[%d] %d = %d/0x%#04x @ %d\r\n", i, cur_index_trend, trend[i], trend[i], cur_offset_trend); // sensor_time - (15 - i)
		cur_index_trend = (cur_index_trend + 1) % 16; // Ringbuffer linearisieren
	}

	// Historie alle 15min, 32 Eintr�ge macht 8h Verlaufsdaten
	// Historic trend data is found on bytes 124 to 315
	uint16_t h_time = sensor_time -(sensor_time%15);
  h_time -= (15*31);
	uint8_t cur_index_history = index_history;
	for(int i = 0; i < 32; i++)
	{
		const unsigned int cur_offset_history = (124-24) + (LIBRE_SENSOR_SAMPLE_SIZE * cur_index_history); // Offset des aktuellen Samples berechnen
    history[i] = LibreSensor_CalculateGlucoseReading(pu8DataBlock3To39, cur_offset_history);
		LIBRE_DEBUG("historie[%d] %d = %d/0x%#04x @ %d (h_time=%d)\r\n", i, cur_index_history, history[i], history[i], cur_offset_history, h_time);
		h_time+=15;
		cur_index_history = (cur_index_history + 1) % 32; // Ringbuffer linearisieren
	}

	return true;
}
