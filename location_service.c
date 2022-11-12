#include <stdlib.h>
#include <ctype.h>
#include "location_service.h"
#include "gnss_handler.h"

#define MAX_SUBSCRIBERS             5U  /**< Maximum number of clients that can subscribe to location server. */
#define DECIMAL_PRECISION           6U  /**< Decimal precision of location data. */
#define MAX_ABS_LATITUDE           90U  /**< Maximum absolute latitude */
#define MAX_ABS_LONGITUDE         180U  /**< Maximum absolute longitude */

static const char msg_invalid_location[] = "Invalid location!";

// Private data
static locationServerAcceptorFnPtr location_notification_handle[MAX_SUBSCRIBERS];
static LocationDataType location;

// Private method declarations
static bool validate_location_data(const uint8_t *buffer, uint8_t received_bytes, LocationDataType *location);
static void set_location_data(const uint8_t *buffer, uint8_t received_bytes, LocationDataType *location);
static int8_t search_char(const uint8_t *buffer, uint8_t buffer_size, char c);
static bool validate_coordinate(const uint8_t *buffer, uint8_t buffer_size, uint8_t decimal_pos, uint8_t max_degrees);void set_coordinate(const uint8_t *buffer, uint8_t buffer_size, CoordinateType *coordinate);

/*
 * Public methods
 */

/**@brief Inits location service module */
void location_service_init(void)
{
    for (uint8_t idx = 0U; idx < MAX_SUBSCRIBERS; ++idx)
    {
        location_notification_handle[idx] = NULL;
    }
    location_data_init(&location);
}

/**@brief Set location data to default values. */
void location_data_init(LocationDataType *location_data)
{
    location_data->latitude.sign    = 1;
    location_data->latitude.degrees = 0U;
    location_data->latitude.decimal = 0UL;

    location_data->longitude.sign    = 1;
    location_data->longitude.degrees = 0U;
    location_data->longitude.decimal = 0UL;
}

/**@brief Converts location data to string */
void location_data_serialize(const LocationDataType *location_data, uint8_t *buffer, uint8_t buffer_size)
{
    if (buffer_size >= (LATITUDE_MAX_DATA_SIZE + LONGITUDE_MAX_DATA_SIZE + 1U))
    {
        buffer[LATITUDE_MAX_DATA_SIZE] = ',';

        buffer[0U] = (location_data->latitude.sign > 0) ? '+' : '-';
        buffer[3U] = '.';
        uint8_t idx = 2U;
        uint32_t tmp = location_data->latitude.degrees;
        while (idx > 0U)
        {
            buffer[idx] = '0'+ (tmp % 10U);
            tmp /= 10U;
            --idx;
        }
        idx = DECIMAL_PRECISION;
        tmp = location_data->latitude.decimal;
        while (idx > 0U)
        {
            buffer[3U + idx] = '0' + (tmp % 10U);
            tmp /= 10U;
            --idx;
        }

        buffer[LATITUDE_MAX_DATA_SIZE + 1U] = (location_data->longitude.sign > 0) ? '+' : '-';
        buffer[LATITUDE_MAX_DATA_SIZE + 5U] = '.';
        idx = 3U;
        tmp = location_data->longitude.degrees;
        while (idx > 0U)
        {
            buffer[LATITUDE_MAX_DATA_SIZE + 1U + idx] = '0' + (tmp % 10U);
            tmp /= 10U;
            --idx;
        }
        idx = DECIMAL_PRECISION;
        tmp = location_data->longitude.decimal;
        while (idx > 0U)
        {
            buffer[LATITUDE_MAX_DATA_SIZE + 5U + idx] = '0' + (tmp % 10U);
            tmp /= 10U;
            --idx;
        }
    }
}

/**@brief Updates location data and notifies subscribed clients.
 * 
 * @details Checks for new data received on UART, validates and sets new location data and notifies
 *          subscribed clients.
*/
void location_service_update(void)
{
    /* size of buffer equal to latitude + longitude data + 3 bytes for comma and UART CR LF */
    static uint8_t buffer[LATITUDE_MAX_DATA_SIZE + LONGITUDE_MAX_DATA_SIZE + 3U];
    static uint8_t bytes_received = 0U;

    if (gnss_handler_receive(&bytes_received, buffer, sizeof(buffer)) && (bytes_received > 0U))
    {
        if (validate_location_data(buffer, bytes_received, &location))
        {
            set_location_data(buffer, bytes_received, &location);
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
            gnss_handler_transmit((uint8_t *)msg_invalid_location, sizeof(msg_invalid_location));
        }

        bytes_received = 0U;
    }
}

/**@brief Function to subscribe to location server.
 * 
 * @details Callback function can be registered with location server to get notified about new data.
 * 
 * @param[in]   acceptorPtr     Function pointer for registering with location server.
 * 
 * @returns handle >= 0 if subscription was successfull, -1 otherwise.
*/
int8_t location_service_subscribe(const locationServerAcceptorFnPtr acceptorPtr)
{
    int8_t handle = -1;

    for (uint8_t idx = 0U; idx < MAX_SUBSCRIBERS; ++idx)
    {
        if (NULL == location_notification_handle[idx])
        {
            location_notification_handle[idx] = acceptorPtr;
            handle = idx;
            break;
        }
    }

    return handle;
}


/*
 * Private methods
 */

/**@brief Validates location data. */
static bool validate_location_data(const uint8_t * buffer, uint8_t received_bytes, LocationDataType * location)
{
    bool is_valid_latitude = true;
    bool is_valid_longitude = true;
    int8_t comma_pos = 0U;
    int8_t decimal_pos = 0U;

    // Check input params
    if ((NULL == buffer ) || (NULL == location) || (0U == received_bytes))
    {
        return false;
    }

    // Search comma
    comma_pos = search_char(buffer, received_bytes, ',');

    if ((comma_pos > 0) && (comma_pos < received_bytes))
    {
        decimal_pos = search_char(buffer, comma_pos, '.');
        if ((decimal_pos >= 0) && (decimal_pos < 4) &&
            ((comma_pos - decimal_pos - 1) == DECIMAL_PRECISION))
        {
            is_valid_latitude = validate_coordinate(buffer, comma_pos, decimal_pos, MAX_ABS_LATITUDE);
        }
        else
        {
            is_valid_latitude = false;
        }

        if (is_valid_latitude)
        {
            uint8_t longitude_received_bytes = received_bytes - comma_pos - 1;
            const uint8_t *longitude_buffer = &buffer[comma_pos + 1U];

            decimal_pos = search_char(longitude_buffer, longitude_received_bytes, '.');
            if ((decimal_pos >= 0) && (decimal_pos < 5) &&
                ((longitude_received_bytes - decimal_pos - 1) == DECIMAL_PRECISION))
            {
                is_valid_longitude = validate_coordinate(longitude_buffer, longitude_received_bytes, decimal_pos, MAX_ABS_LONGITUDE);
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
    
    return (is_valid_latitude && is_valid_longitude);
}

/**@brief Sets location data.
 * 
 * @attention   This function does not validate data in input buffer!
*/
static void set_location_data(const uint8_t *buffer, uint8_t received_bytes, LocationDataType *location)
{
    uint8_t comma_pos = 0U;

    if ((NULL != buffer) && (NULL != location) && (received_bytes > 0U))
    {
        comma_pos   = search_char(buffer, received_bytes, ',');

        set_coordinate(buffer, comma_pos, &location->latitude);

        const uint8_t *longitude_buffer = &buffer[comma_pos + 1U];
        uint8_t longitude_received_bytes = received_bytes - comma_pos - 1U;
        set_coordinate(longitude_buffer, longitude_received_bytes, &location->longitude);
    }
}

void set_coordinate(const uint8_t *buffer, uint8_t buffer_size, CoordinateType *coordinate)
{
    uint8_t idx = 0U;
    uint8_t decimal_pos = search_char(buffer, buffer_size, '.');

    if ('+' == buffer[idx])
    {
        coordinate->sign = 1;
        ++idx;
    }
    else if ('-' == buffer[idx])
    {
        coordinate->sign = -1;
        ++idx;
    }
    else
    {
        coordinate->sign = 1;
    }

    coordinate->degrees = 0U;
    for (; idx < decimal_pos; ++idx)
    {
        coordinate->degrees *= 10U;
        coordinate->degrees += (buffer[idx] - '0');
    }

    coordinate->decimal = 0UL;
    for (idx = decimal_pos + 1U; idx < buffer_size; ++idx)
    {
        coordinate->decimal *= 10U;
        coordinate->decimal += (buffer[idx] - '0');
    }
}

static int8_t search_char(const uint8_t * buffer, uint8_t buffer_size, char c)
{
    int8_t pos = -1;

    for (uint8_t idx = 0U; idx < buffer_size; ++idx)
    {
        if (c == buffer[idx])
        {
            pos = idx;
            break;
        }
    }

    return pos;
}

static bool validate_coordinate(const uint8_t *buffer, uint8_t buffer_size, uint8_t decimal_pos, uint8_t max_degrees)
{
    bool is_valid = true;
    uint8_t idx = 0U;
    uint8_t degrees = 0U;

    // Check leading sign
    if (('+' == buffer[idx]) || ('-' == buffer[idx]))
    {
        ++idx;
    }

    // Check degrees
    for (; idx < decimal_pos; ++idx)
    {
        char c = buffer[idx];
        if (isdigit(c) == 0)
        {
            is_valid = false;
            break;
        }
        else
        {
            degrees *= 10;
            degrees += (c - '0');
        }
    }

    // Check decimal degrees
    if (is_valid)
    {
        for (idx = decimal_pos + 1U; idx < buffer_size; ++idx)
        {
            if (degrees < max_degrees)
            {
                if (isdigit(buffer[idx]) == 0)
                {
                    is_valid = false;
                    break;
                }
            }
            else
            {
                if ('0' != buffer[idx])
                {
                    is_valid = false;
                    break;
                }
            }
        }
    }

    return is_valid;
}