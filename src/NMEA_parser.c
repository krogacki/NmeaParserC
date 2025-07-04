/*
 ============================================================================
 Name        : NMEA_parser.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define GPRMC_UTC_TIME   		1
#define GPRMC_STATUS   			2
#define GPRMC_LATITUDE   		3
#define GPRMC_LAT_HEMISPHERE 	4
#define GPRMC_LONGITUTDE 		5
#define GPRMC_LONG_HEMISPHERE 	6
#define GPRMC_KNOTS_SPEED 		7
#define GPRMC_COURSE	 		8
#define GPRMC_UTC_DATE	 		9
#define GPRMC_MAG_VAR	 	 	10
#define GPRMC_UNUSED	 	 	11
#define GPRMC_MODE		 	 	12
#define GPRMC_CHECKSUM	 	 	13


/*-----------------------------------------------------------------------------------------*/

typedef struct Date_type {
	uint16_t year;
	uint16_t miliseconds;
	uint8_t  seconds;
	uint8_t  minutes;
	uint8_t  hours;
	uint8_t  day;
	uint8_t  month;
} Date_type;

typedef struct Coords_type {
	uint32_t latitudeFractional;
	uint32_t longitudeFractional;
	int16_t  latitudeDecimal;    /* +N, -S */
	int16_t  longitudeDecimal;   /* +E, -W */
	char     latitudeHemisphere;
	char     longitudeHemisphere;
	char     statusAV;
} Coords_type;

typedef struct Speed_type {
	uint16_t knotsDecimal;
	uint16_t knotsFractional;
	uint16_t kmph;
} Speed_type;

typedef struct Magnetic_type {
	uint16_t varDecimal;
	uint16_t varFractional;
	char	 varDirection;
} Magnetic_type;

typedef struct GPRMC_type {
	Coords_type   position;
	Date_type     utcDate;
	Speed_type    speed;
	Magnetic_type mag;
	uint16_t      checksum;
} GPRMC_type;
/*-----------------------------------------------------------------------------------------*/

char Nmea[] = "$GPRMC,101501.00,A,5036.04918,N,01920.19673,E,0.857,,250320,,,A*7C\r\n";
static GPRMC_type Gprmc;

/*-------------------------------- Declarations -------------------------------------------*/
bool IsDataValid(const uint8_t *input, const uint8_t size);
void GprmcParseData(const char * gprmcStr, const uint8_t sizeStr, GPRMC_type *outGprmc);
int16_t GprmcStartIndexOf(const char *inputData, uint16_t dataSize, uint8_t gprmcField);
int16_t GprmcEndIndexOf(const char *inputData, uint16_t dataSize, uint8_t gprmcField);
void GprmcGetDate(const char *rawGprmc, uint16_t rawSize, Date_type *inputUtc);

/*----------------------------------- Main ------------------------------------------------*/
int main(void) {


	GprmcParseData(Nmea, strlen(Nmea), &Gprmc);
	printf("Time: %02d:%02d:%02d", Gprmc.utcDate.hours, Gprmc.utcDate.minutes, Gprmc.utcDate.seconds);
	printf("\nDone!");
	return EXIT_SUCCESS;
}

/*-----------------------------------------------------------------------------------------*/

void GprmcParseData(const char * gprmcStr, const uint8_t sizeStr, GPRMC_type *outGprmc) {

	int8_t validData = false;
	const char * gprmcPattern = "$GPRMC";

	validData = (int8_t)IsDataValid((uint8_t *)gprmcStr, sizeStr);

	if (true == validData) {
		//check if GPRMC was added
		validData = strncmp(gprmcPattern, gprmcStr, strlen(gprmcPattern));
		if (0 == validData) {
			/* get date */
			GprmcGetDate(gprmcStr, sizeStr,&outGprmc->utcDate);
		}
	}
}


void GprmcGetDate(const char *rawGprmc, uint16_t rawSize, Date_type *inputUtc) {
	char data[10] = {0xFF};
	uint32_t time;
	uint16_t startIdx = 0;
	uint16_t endIdx = 0;
	uint16_t sliceSize;

	if ((NULL != inputUtc) && (NULL != rawGprmc)) {
		/* get time */
		startIdx = GprmcStartIndexOf(rawGprmc, rawSize, GPRMC_UTC_TIME);
		endIdx = GprmcEndIndexOf(rawGprmc, rawSize, GPRMC_UTC_TIME);
		sliceSize = endIdx - startIdx;

		memcpy(data, &rawGprmc[startIdx], sliceSize);

		if (isdigit(data[0])) {
			time = atoi(data);
			inputUtc->hours = ((data[0] - '0') * 10) | (data[1] - '0');
			inputUtc->minutes = ((data[2] - '0') * 10) | (data[3] - '0');
			inputUtc->seconds = ((data[4] - '0') * 10) | (data[5] - '0');
		}
	}
}

int16_t GprmcStartIndexOf(const char *inputData, uint16_t dataSize, uint8_t gprmcField) {

	int16_t indexOf = -1;
	uint8_t counter = 0;
	int16_t idx;

	for (idx = 0; idx < dataSize; idx++)
	{
		if (',' == inputData[idx] || '*' == inputData[idx])
		{
			counter++;
			if (gprmcField == counter){
				indexOf = idx + 1;
				break;
			}
		}
	}

	return indexOf;
}

int16_t GprmcEndIndexOf(const char *inputData, uint16_t dataSize, uint8_t gprmcField) {

	int16_t indexOf = -1;
	uint8_t counter = 0;
	int16_t idx;
	int16_t endIdx = gprmcField + 1;

	for (idx = 0; idx < dataSize; idx++)
	{
		if (',' == inputData[idx] || '*' == inputData[idx] || '\r' == inputData[idx])
		{
			counter++;
			if (endIdx == counter){
				indexOf = idx;
				break;
			}
		}
	}

	return indexOf;
}

bool IsDataValid(const uint8_t *input, const uint8_t size) {

	bool retVal = false;
	uint8_t crc_cal = 0xFF;
	uint8_t idx = 0;
	uint8_t crc_idx;
	char crcDataIn[3] = {0};
	char crcDataCalc[3] = {0};


	if (NULL != input) {
		if ('$' == input[0]) {
			crc_cal = 0;
			for (idx = 1; idx < size; idx++) {
				if ('*' == input[idx]) {
					break;
				}
				crc_cal ^= input[idx]; //0x7C - 124
			}

			//copy crc data
			crc_idx = size - idx;
			if (crc_idx >= 3) {
				idx++;
				memcpy(crcDataIn, &input[idx], 2);
				crcDataIn[2] = '\0';
			}
		}
	}

	/* Change hex value to ascii */
    sprintf(crcDataCalc, "%X", crc_cal);

	if (strcmp(crcDataCalc, crcDataIn) == 0) {
		retVal = true;
	}

	return retVal;
}

void Gprmc_Parser(const char *data_input, GPRMC_type *gprmc_input) {
	puts(data_input);
	printf("%d", gprmc_input->speed.kmph);
}
