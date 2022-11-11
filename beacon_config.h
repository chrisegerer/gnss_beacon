#ifndef BEACON_CONFIG_H__
#define BEACON_CONFIG_H__

#include "app_util.h"

#define DEVICE_NAME                     "GNSS Beacon"   /**< Device name. */
#define APP_BLE_CONN_CFG_TAG            1               /**< A tag identifying the SoftDevice BLE configuration. */

#define NON_CONNECTABLE_ADV_INTERVAL    MSEC_TO_UNITS(100, UNIT_0_625_MS) /**< The advertising interval for non-connectable advertisement (100 ms). This value can vary between 100ms to 10.24s). */

#define APP_BEACON_INFO_LENGTH          0x16            /**< Total length of information advertised by the Beacon. */
#define APP_COMPANY_IDENTIFIER          0xFFFF          /**< Undefined company ID. */

#endif // BEACON_CONFIG_H__