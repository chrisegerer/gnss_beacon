#include <stdlib.h>
#include <string.h>
#include "location_service.h"
#include "uart_handler.h"

#define MAX_SUBSCRIBERS 5U
#define DECIMAL_PRECISION 6U

static locationServerAcceptorFnPtr location_notification_handle[MAX_SUBSCRIBERS];
static LocationDataType location;
static const char msg_invalid_location[] = "Invalid location!";

static bool validate_location_data(uint8_t *buffer, uint8_t received_bytes, LocationDataType * location);
static bool is_digit(const char c);

void location_service_init(void)
{
    for (uint8_t idx = 0U; idx < MAX_SUBSCRIBERS; ++idx)
    {
        location_notification_handle[idx] = NULL;
    }
    location_data_init(&location);
}

void location_service_update(void)
{
    /* size of buffer equal to latitude + longitude data + 3 bytes for comma and UART CR LF */
    static uint8_t buffer[LATITUDE_DATA_SIZE+LONGITUDE_DATA_SIZE+3U];
    static uint8_t bytes_received = 0U;

    if (uart_handler_receive(&bytes_received, buffer, sizeof(buffer)) && (bytes_received > 0U))
    {
        if (validate_location_data(buffer, bytes_received, &location))
        {
            for (uint8_t idx = 0U; idx < MAX_SUBSCRIBERS; ++idx)
            {
                if (NULL != location_notification_handle[idx])
                {
                    location_notification_handle[idx](&location);
                }
            }
        }
        else
        {
            uart_handler_transmit((uint8_t *)msg_invalid_location, sizeof(msg_invalid_location));
        }

        bytes_received = 0U;
    }
}

bool location_service_subscribe(const locationServerAcceptorFnPtr acceptorPtr)
{
    bool free_slot = false;

    for (uint8_t idx = 0U; idx < MAX_SUBSCRIBERS; ++idx)
    {
        if (NULL == location_notification_handle[idx])
        {
            location_notification_handle[idx] = acceptorPtr;
            free_slot = true;
            break;
        }
    }

    return free_slot;
}

void location_data_init(LocationDataType * location_data)
{
    memset(location_data->latitude, '0', LATITUDE_DATA_SIZE);
    memset(location_data->longitude, '0', LONGITUDE_DATA_SIZE);
    location_data->latitude[3U] = '.';
    location_data->longitude[4U] = '.';
}

static bool validate_location_data(uint8_t * buffer, uint8_t received_bytes, LocationDataType * location)
{
    bool is_valid_latitude = true;
    bool is_valid_longitude = true;
    uint8_t comma_pos = 0U;
    uint8_t decimal_pos = 0U;

    for (uint8_t idx = 0U; idx < received_bytes; ++idx)
    {
        if (',' == buffer[idx])
        {
            comma_pos = idx;
            break;
        }
    }

    if (0U != comma_pos)
    {
        for (uint8_t idx = 0U; idx < comma_pos; ++idx)
        {
            if ('.' == buffer[idx])
            {
                decimal_pos = idx;
                break;
            }
        }
        if ((decimal_pos > 0U) && (decimal_pos < 4U) &&
            (comma_pos - decimal_pos - 1U) == DECIMAL_PRECISION)
        {
            uint8_t idx = 0U;
            if (('+' == buffer[idx]) || ('-' == buffer[idx]))
            {
                ++idx;
            }
            while (idx < decimal_pos)
            {
                if (!is_digit(buffer[idx]))
                {
                    is_valid_latitude = false;
                    break;
                }
                ++idx;
            }
            for (idx = decimal_pos + 1U; idx < comma_pos; ++idx)
            {
                if (!is_digit(buffer[idx]))
                {
                    is_valid_latitude = false;
                    break;
                }
            }
        }
        else
        {
            is_valid_latitude = false;
        }

        if (is_valid_latitude)
        {
            decimal_pos = 0U;
            for (uint8_t idx = comma_pos+1U; idx < received_bytes; ++idx)
            {
                if ('.' == buffer[idx])
                {
                    decimal_pos = idx;
                    break;
                }
            }
            if (((decimal_pos - comma_pos - 1U) > 0U) &&
                ((decimal_pos - comma_pos - 1U) < 5U) &&
                (received_bytes - decimal_pos - 1U) == DECIMAL_PRECISION)
            {
                uint8_t idx = comma_pos + 1U;
                if (('+' == buffer[idx]) || ('-' == buffer[idx]))
                {
                    ++idx;
                }
                while (idx < decimal_pos)
                {
                    if (!is_digit(buffer[idx]))
                    {
                        is_valid_longitude = false;
                    }
                    ++idx;
                }
                for (idx = decimal_pos + 1U; idx < received_bytes; ++idx)
                {
                    if (!is_digit(buffer[idx]))
                    {
                        is_valid_longitude = false;
                    }
                }
            }
            else
            {
                is_valid_longitude = false;
            }
        }
    }
    else
    {
        is_valid_latitude = false;
        is_valid_longitude = false;
    }

    if (is_valid_latitude && is_valid_longitude)
    {
        memset(location->latitude, '\0', sizeof(location->latitude));
        memset(location->longitude, '\0', sizeof(location->longitude));
        memcpy(location->latitude, &buffer[0U], comma_pos);
        memcpy(location->longitude, &buffer[comma_pos+1U], received_bytes - comma_pos - 1U);
    }
    
    return (is_valid_latitude && is_valid_longitude);
}

static bool is_digit(const char c)
{
    return ((c >= '0') && (c <='9'));
}