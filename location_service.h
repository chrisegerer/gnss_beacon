#ifndef LOCATION_SERVICE_H__
#define LOCATION_SERVICE_H__

#include <stdint.h>
#include <stdbool.h>

#define LATITUDE_DATA_SIZE  10U
#define LONGITUDE_DATA_SIZE 11U

typedef struct LocationData
{
    uint8_t latitude[LATITUDE_DATA_SIZE];
    uint8_t longitude[LONGITUDE_DATA_SIZE];
} LocationDataType;

typedef void (*locationServerAcceptorFnPtr)(const LocationDataType* const);

void location_service_init(void);
void location_service_update(void);
bool location_service_subscribe(const locationServerAcceptorFnPtr acceptorPtr);

void location_data_init(LocationDataType* location_data);

#endif // LOCATION_SERVICE_H__