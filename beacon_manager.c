#include <string.h>
#include "nordic_common.h"
#include "bsp.h"
#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_advdata.h"
#include "beacon_config.h"
#include "beacon_manager.h"
#include "location_service.h"
#include "uart_handler.h"

#define DEAD_BEEF 0xDEADBEEF /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#define BEACON_INFO_INIT_DATA \
         '+', '0', '0', '.', '0', '0', '0', '0', '0', '0', ',', \
    '+', '0', '0', '0', '.', '0', '0', '0', '0', '0', '0'

    static ble_gap_adv_params_t m_adv_params;                 /**< Parameters to be passed to the stack when starting advertising. */
static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET; /**< Advertising handle used to identify an advertising set. */
static uint8_t m_enc_advdata[2U][BLE_GAP_ADV_SET_DATA_SIZE_MAX];  /**< Buffer for storing an encoded advertising set. */
static uint8_t m_enc_srdata[2U][BLE_GAP_ADV_SET_DATA_SIZE_MAX];   /**< Buffer for storing an encoded scan response set. */

/**@brief Struct that contains pointers to the encoded advertising and scan response data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata[0U],
        .len = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = m_enc_srdata[0U],
        .len = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    }
};

static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH] = /**< Information advertised by the Beacon. */
{
    BEACON_INFO_INIT_DATA
};

static void ble_stack_init(void);
static void gap_params_init(void);
static void advertising_init(void);
static void beacon_manager_accept(const LocationDataType * location_data);

void beacon_manager_init(void)
{
    ble_stack_init();
    gap_params_init();
    advertising_init();

    if (false == location_service_subscribe(&beacon_manager_accept))
    {
        /* todo: error handling in case subscription fails. */
    }
}

/**@brief Function for starting advertising.
 */
void beacon_advertising_start(void)
{
    ret_code_t err_code;

    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
    APP_ERROR_CHECK(err_code);
}

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Subscription function for accepting new location data
 * 
 * @details This function can be used to subscribe to a location server and 
 *          updates the advertised location data.
*/
static void beacon_manager_accept(const LocationDataType * location_data)
{
    uint32_t err_code;
    ble_advdata_t advdata;
    ble_advdata_t srdata;
    uint8_t flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
    ble_advdata_manuf_data_t manuf_specific_data;

    // echo location data on UART
    uart_handler_transmit(location_data->latitude, sizeof(location_data->latitude));
    uart_handler_transmit(location_data->longitude, sizeof(location_data->longitude));

    memset(m_beacon_info, 0U, sizeof(m_beacon_info));
    memcpy(m_beacon_info, location_data->latitude, sizeof(location_data->latitude));
    m_beacon_info[10U] = ',';
    memcpy(&m_beacon_info[11U], location_data->longitude, sizeof(location_data->longitude));

    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;
    manuf_specific_data.data.p_data = (uint8_t *)m_beacon_info;
    manuf_specific_data.data.size = APP_BEACON_INFO_LENGTH;

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type = BLE_ADVDATA_NO_NAME;
    advdata.flags = flags;
    advdata.p_manuf_specific_data = &manuf_specific_data;

    // Build and set scan response data.
    memset(&srdata, 0, sizeof(srdata));

    srdata.name_type = BLE_ADVDATA_SHORT_NAME;
    srdata.short_name_len = 11U;

    m_adv_data.adv_data.p_data = (m_adv_data.adv_data.p_data != m_enc_advdata[0U]) ? m_enc_advdata[0U] : m_enc_advdata[1U];
    m_adv_data.scan_rsp_data.p_data = (m_adv_data.scan_rsp_data.p_data != m_enc_srdata[0U]) ? m_enc_srdata[0U] : m_enc_srdata[1U];

    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    err_code = ble_advdata_encode(&srdata, m_adv_data.scan_rsp_data.p_data, &m_adv_data.scan_rsp_data.len);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t err_code;
    ble_advdata_t advdata;
    ble_advdata_t srdata;
    uint8_t flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
    ble_advdata_manuf_data_t manuf_specific_data;

    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;
    manuf_specific_data.data.p_data = (uint8_t *)m_beacon_info;
    manuf_specific_data.data.size = APP_BEACON_INFO_LENGTH;

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type = BLE_ADVDATA_NO_NAME;
    advdata.flags = flags;
    advdata.p_manuf_specific_data = &manuf_specific_data;

    // Build and set scan response data.
    memset(&srdata, 0, sizeof(srdata));

    srdata.name_type = BLE_ADVDATA_SHORT_NAME;
    srdata.short_name_len = 11U;

    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    m_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED;
    m_adv_params.p_peer_addr = NULL; // Undirected advertisement.
    m_adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval = NON_CONNECTABLE_ADV_INTERVAL;
    m_adv_params.duration = 0; // Never time out.

    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    err_code = ble_advdata_encode(&srdata, m_adv_data.scan_rsp_data.p_data, &m_adv_data.scan_rsp_data.len);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing GAP parameters.
 */
static void gap_params_init(void)
{

    uint32_t err_code;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);
}
