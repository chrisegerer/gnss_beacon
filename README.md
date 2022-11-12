# GNSS Beacon
 Sample app creating a GNSS Beacon with nRF52840-DK.
 
 The Beacon transmits GNSS coordinates in ASCII characters as manufacturer specific data. Since there is no real GNSS receiver used for this sample app, GNSS coordinates are sent via UART from a connected PC.
 
## Setup
- Project is developed using nRF SDK 17.1.0: [nRF SDK](https://www.nordicsemi.com/Products/Development-software/nrf5-sdk)
- Soft device S140 is used: [S140 SoftDevice](https://www.nordicsemi.com/-/media/Software-and-other-downloads/SoftDevices/S140/s140nrf52720.zip)
- Toolchain used is ARM GCC 9 2020 q2 update on Windows: [ARM Toolchain](https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-win32.zip?revision=95631fd0-0c29-41f4-8d0c-3702650bdd74&rev=95631fd00c2941f48d0c3702650bdd74&hash=FA5132E052A0DA0FEE1C7E06E84F0B6A)
- In the `Makefile`, you need to set the `SDK_ROOT` variable to point to nRF SDK root directory.
- In the `Makefile.Windows`, located in the nRF SDK `<SDK_ROOT>\components\toolchain\gcc`, you need to set the compiler path.

## Architecture
Basic architectural overview is shown in following diagram:

![SW Layers](https://github.com/chrisegerer/gnss_beacon/blob/master/doc/layers.png)

On top of the nRF SDK, three components implement the main functionalities:
1. **GNSS Handler:** The GNSS Handler is a proxy for any possible GNSS receiver. It should provide a common interface for getting new location data. Currently, it is an interface to the UART for receiving location data from a PC.

2. **Beacon Manager:** The Beacon Manager interfaces with the SoftDevice. It is responsible for configuring the SoftDevice and updating the advertised data. The device name is transmitted as part of the scan response data.

3. **Location Service:** The Location Service implements a client/server-like interface where clients can subscribe to get new location data. Thus, it provides a subscribe function `location_service_subscribe`. Clients wanting to receive location updates need to implement an acceptor function defined by the `locationServerAcceptorFnPtr` function pointer and pass this function to the subscribe function. The `location_service_update` function needs to be called in order to poll for new location data. If new location data is received and is valid all subscribed clients are notified.

In the infinite main loop, the function `location_service_update` is called continuously to check for new locations received and handles the idle state.

## Providing location data from PC
New location data can be sent from PC via serial console. Baudrate is 115200, 1 stop bit, no parity, no flow control.

Valid data range is
- latitude: +/- 90.000000 deg
- longitude: +/- 180.000000 deg

6 digits of decimal precision are expected. Latitude and longitude need to be separated by a comma.
Sign in case of positive values can be ommitted. Also any leading zeros before the decimal point can be ommitted.

Thus valid location data are for example:
```
0.123456,0.123456
-0.123456,+0.123456
-.123456,.123456
+90.000000,+180.000000
-90.000000,-180.000000
```
In case of invalid location data, the user will be notified by sending `Invalid location!` over UART.
