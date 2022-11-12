#ifndef LOCATION_SERVICE_H__
#define LOCATION_SERVICE_H__

#include <stdint.h>
#include <stdbool.h>

#define LATITUDE_MAX_DATA_SIZE      10U     /**< Maximum length of received latitude data. */
#define LONGITUDE_MAX_DATA_SIZE     11U     /**< Maximum length of received longitude data. */

typedef struct Coordinate
{
    int8_t sign;
    uint8_t degrees;
    uint32_t decimal;
} CoordinateType;

typedef struct LocationData
{
    CoordinateType latitude;
    CoordinateType longitude;
} LocationDataType;

typedef void (*locationServerAcceptorFnPtr)(const LocationDataType* const);

void location_service_init(void);
void location_service_update(void);
int8_t location_service_subscribe(const locationServerAcceptorFnPtr acceptorPtr);

void location_data_init(LocationDataType* location_data);
void location_data_serialize(const LocationDataType* location_data, uint8_t *buffer, uint8_t buffer_size);

#endif // LOCATION_SERVICE_H__