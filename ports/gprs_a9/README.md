[![Build Status](https://travis-ci.org/pulkin/micropython.svg?branch=master)](https://travis-ci.org/pulkin/micropython)
[![Build Status](https://dev.azure.com/gpulkin/micropython/_apis/build/status/pulkin.micropython?branchName=master)](https://dev.azure.com/gpulkin/micropython/_build/latest?definitionId=1&branchName=master)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2RZCNXCUCP4YG&source=url)

# A9/A9G MicroPython Port

## General Information

The [Ai-Thinker A9/A9G series module](https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/hardware/pudding-dev-board.html), aka *Pudding*, is a cheap development board with GSM/GPRS (2G) and GPS functionalities. A9 relates to the MCU without a GPS (A9G with a GPS). A SIM card is required to access the internet (dedicated IoT mobile operators exist for such purpose). [MicroPython](https://micropython.org/) is Python for MCUs. It offers a high-level language to program the board. This port provides dedicated GSM/GPRS and GPS MicroPython modules, making the board the perfect candidate for a cheap and effective Internet of Things device.

## Contribute

See [CONTRIBUTING.md](https://github.com/pulkin/micropython/blob/binary-patches/ports/gprs_a9/CONTRIBUTING.md) or use the donate button.

## Module Pinout and Connections

![A9G](https://raw.githubusercontent.com/Ai-Thinker-Open/GPRS_C_SDK/master/doc/assets/pudding_pin.png)

Serial pins:

- HST_RX and HST_TX to burn the firmware,
- RX1 and TX1 for REPL access.

Note: a SIM card holder and a micro SD card holder are located on the back of the board.

## Firmware

The firmware image is automatically built by Azure pipelines: [download the firmware](https://github.com/pulkin/micropython/releases/tag/latest-build) and extract the file *firmware_debug_full.lod* to be burned.

To build the firmware from source follow these steps (see [.travis.yml](../../.travis.yml) file):

1. Install dependencies:
   ```bash
   sudo apt-get install build-essential gcc-multilib g++-multilib libzip-dev zlib1g lib32z1
   ```
   
2. Clone this repository with all its sub-modules (install Git if necessary: `sudo apt install git`):
   
    **Warning: large download size**.
   
   ```bash
   git clone https://github.com/pulkin/micropython.git --recursive
   ```
   or clone only what is mandatory (smaller download size):
   ```bash
   git clone https://github.com/pulkin/micropython.git
   git submodule update --init --recursive lib/axtls lib/GPRS_C_SDK lib/csdtk42-linux
   ```
   
3. Make:
   ```bash
   cd micropython
   make -C mpy-cross
   cd ports/gprs_a9
   make
   ```

   The build consists of the two *.lod* files generated in the *hex* subdirectory. The largest file *firmware_debug_full.lod* contains the complete firmware and needs to be burned.

## Burn

Follow [vendor instructions](https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/burn-debug.html)
to burn *firmware_debug_full.lod* onto the board.

Note: *coolwatcher*, the utility to burn the firmware, is available under the *lib/csdtk42-linux/cooltools/* directory of this repository. A Windows version is also available elsewhere.

## Connect

Connect the serial adapter to the RX1 and TX1 pins. Use a serial terminal to access the board's REPL. Choose the default transmission parameters and select 115200 baud as the transmission rate:

```bash
screen /dev/ttyUSB0 115200
```

## Upload scripts

Use a MicroPython file transmission utility such as [ampy](https://github.com/pycampers/ampy), [rshell](https://github.com/dhylands/rshell), etc.: 

```bash
ampy --port /dev/ttyUSB0 put script.py
```

## Run scripts

```python
>>> help()
>>> import script
```

## Functionality

- [x] GPIO: `machine.Pin`
- [x] ADC: `machine.ADC`
- [ ] PWM: `machine.PWM`
- [x] UART: `machine.UART` (hardware implementation)
- [x] SPI: `machine.SPI` (software bit-banging implementation)
- [x] I2C: `i2c` (hardware implementation)
- [x] Cellular misc. (IMEI, ICCID, etc.): `cellular`
- [x] GPS: `gps`
- [x] time: `utime`
- [x] File system
- [x] GPRS, DNS: `cellular`, `socket`, `ssl`
- [x] Power: `machine`
- [x] Calls: `cellular`
- [x] SMS: `cellular.SMS`

Note: additional libraries can be installed via [*upip*](https://docs.micropython.org/en/latest/reference/packages.html) (built into the module). To save some mobile data, one can download first the libraries onto the computer using [*micropip*](https://github.com/peterhinch/micropython-samples/tree/master/micropip) and copy them afterwards via serial connection onto the board.

## Code Examples

See the [example folder](examples). There are examples about:

- blinking the onboard LEDs,
- Enabling and feeding the watchdog,
- Getting GPS coordinates and time,
- Getting AGPS (GSM base station) coordinates,
- Accessing the internet and requesting a webpage,
- Securing an internet socket over ssl,
- Publishing and subscribing to a topic over MQTT.

## API

1. [`cellular`](#cellular): SMS, calls, connectivity,
2. [`usocket`](#usocket): sockets over GPRS,
3. [`ssl`](#ussl): SSL over sockets,
4. [`gps`](#gps): everything related to GPS and assisted positioning,
5. [`machine`](#machine): hardware and power control,
6. [`i2c`](#i2c): i2c implementation **(Note: to be moved under machine.i2c for API consistency)**,
7. [`st7735`](#ST7735): ST7735 over software SPI display implementation **(Note: obsolete, to be removed)**,
8. [Other modules](#Misc.),
9. [Notes](#Notes).

### `cellular`

Provides cellular functionalities.
The API does not give access to radio-level and low-level functionality (such as controlling the registration on the cellular network): these are performed in the background automatically.
The purpose of this module is to have an access to high-level networking (SMS, GPRS, calls) as well as to get the status of various components of cellular networking.

* `NETWORK_FREQ_BAND_GSM_900P`, `NETWORK_FREQ_BAND_GSM_900E`, `NETWORK_FREQ_BAND_GSM_850`, `NETWORK_FREQ_BAND_DCS_1800`, `NETWORK_FREQ_BAND_PCS_1900`, `NETWORK_FREQ_BANDS_ALL`: frequencies;
* `OPERATOR_STATUS_UNKNOWN`, `OPERATOR_STATUS_AVAILABLE`, `OPERATOR_STATUS_CURRENT`, `OPERATOR_STATUS_DISABLED`: cellular operator statuses;
* `NETWORK_MODE_MANUAL`, `NETWORK_MODE_AUTO`, `NETWORK_MODE_MANUAL_AUTO`: network registration modes;
* `SMS(phone_number: str, message: str)`: handles SMS messages;
  * `.phone_number` (str): phone number (sender or recipient);
  * `.message` (str): message content;
  * `.status` (int): integer with status bits;
  * `.inbox` (bool): incoming message if `True`, outgoing message if `False` or unknown status if `None`;
  * `.unread` (bool): unread message if `True`, previously read message if `False` or unknown status if `None`;
  * `.sent` (bool): message sent if `True`, message not sent if `False` or unknown status if `None`;
  * `.send()`: sends a message;
  * `.withdraw()`: withdraws SMS from the SIM storage;
  * `.poll()` (int) [staticmethod]: the number of new SMS received;
  * `.list()` (list) [staticmethod]: all SMS from the SIM card;
* `CellularError(message: str)`
* `get_imei()` (str): the International Mobile Equipment Identity (IMEI) number;
* `get_iccid()` (str): the Integrated Circuit Card ID (ICCID) number of the inserted SIM card;
* `get_imsi()` (str): the International Mobile Subscriber Identity (IMSI) number of the inserted SIM card;
* `network_status_changed()` (bool): indicates whether the network status changed since the last check;
* `get_network_status()` (int): cellular network status encoded in an integer. **TODO: Provide bit-wise specs**;
* `poll_network_exception()`: polls the network exception and raises it, if any;
* `is_sim_present()` (bool): checks whether a SIM card is present or not;
* `is_network_registered()` (bool): checks whether registered on the cellular network or not;
* `is_roaming()` (bool): checks whether registered on the roaming network or not;
* `get_signal_quality()` (int, int): the signal quality (0-31) and RXQUAL. These are replaced by `None` if no signal quality information is available. **TODO: The RXQUAL output is always `None`**;
* `flight_mode([flag: bool])` (bool): the flight mode status. Turns it on or off if the argument is specified;
* `set_bands(bands: int = NETWORK_FREQ_BANDS_ALL)`: sets frequency bands;
* `scan()` (list): lists available operators. Returns `(op_id: bytearray[6], op_name: str, op_status: int)` for each operator;
* `register([operator_id: bytearray[6], register_mode: int])` (op_id: bytearray[6], op_name: str, reg_status: int): registered network operator information. Registers on the network if arguments supplied. **TODO: Figure out how (and whether) registration works at all**;
* `stations()` (list): a list of nearby stations: `(mcc, mnc, lac, cell_id, bsic, rx_full, rx_sub, arfcn)`: all ints;
* `agps_station_data()` (int, int, list): a convenience function returning `(mcc, mnc, [(lac, cell_id, signal_strength), ...])` for use in agps location: all ints;
* `reset()`: resets network settings to defaults. Disconnects GPRS;
* `gprs([apn: {str, bool}[, user: str, pass: str]])` (bool): activate (3 arguments), deactivate (`gprs(False)`) or obtain the status of GPRS (on/off) if no arguments supplied;
* `call()` (list[str], [str, None]): calls missed (1st output) and the incoming call number or `None` if no incoming calls at the moment (2nd output);
* `dial(tn: {str, bool})`: dial a telephone number if string is supplied or hang up a call if `False`;

### `usocket` ###

*Alias: `socket`*

TCP/IP stack over GPRS based on lwIP.
See [micropython docs](https://docs.micropython.org/en/latest/library/usocket.html) for details.

* `AF_INET`, `AF_INET6`, `SOCK_STREAM`, `SOCK_DGRAM`, `SOCK_RAW`, `IPPROTO_TCP`, `IPPROTO_UDP`, `IPPROTO_IP`: lwIP constants;
* `socket(af: int, type: int, proto: int)`: socket class;
    * `close()`
    * `bind(address)` (not implemented)
    * `listen([backlog])` (not implemented)
    * `accept()` (not implemented)
    * `connect(address)`
    * `send(bytes)`
    * `sendall(bytes)`
    * `recv(bufsize)`
    * `sendto(bytes, address)`
    * `recvfrom(bufsize)`
    * `setsockopt(level, optname, value)` (not implemented)
    * `settimeout(value)` (not implemented)
    * `setblocking(flag)` (not implemented)
    * `makefile(mode, buffering)`
    * `read([size])`
    * `readinto(buf[, nbytes])`
    * `readline()`
    * `write(buf)`
* `get_local_ip()` (str): returns the local IP address;
* `getaddrinfo(host (str), port (str), af (int) = AF_INET, type (int) = SOCK_STREAM, proto (int) = IPPROTO_TCP, flags (int) = 0)` (tuple): translates host/port into arguments to socket constructor;
* `inet_ntop(af: int, bin_addr: bytearray)` (str) (not implemented);
* `inet_pton(af: int, txt_addr: str)` (bytearray) (Not implemented);
* `get_num_open()` (int): the number of open sockets (max 8).

### `ussl` ###

*Alias: `ssl`*

Provides SSL over GPRS sockets (axtls).

* `ssl_socket(af, type, proto)`: ssl socket wrapper;
    * `close()`
    * `read([size: int])`
    * `readinto(buf[, nbytes: int])`
    * `readline()`
    * `write(buf)`
* `wrap_socket(sock: socket, server_side: bool = False, keyfile=None, certfile=None, cert_reqs=CERT_NONE, ca_certs=None)` (ssl_socket): wraps a stream socket into ssl. Keyword arguments have never been tested;

### `gps` ###

Provides the GPS functionality.
This is only available in the A9G module where  the GPS is a separate chip connected via UART2.

* `GPSError(message: str)`
* `on()`: turns the GPS on;
* `off()`: turns the GPS off;
* `get_firmware_version()` (str): retrieves the firmware version;
* `get_location()` (longitude: float, latitude: float): retrieves the current GPS location. **Note: longitude and latitude will be swapped soon for better readability**;
* `get_last_location()` (longitude: float, latitude: float): retrieves the last known GPS location without polling the GPS module;
* `get_satellites()` (tracked: int, visible: int): the numbers of satellites in operation;
* `time()` (int): the number of seconds since the epoch (2000). Use `time.localtime` for converting it into date/time values (Note: this conversion will result in `OverflowError` until the GPS module starts reading meaningful satellite data).

### `machine`

Provides power-related functions (power mode, watchdog):

* `POWER_ON_CAUSE_ALARM`, `POWER_ON_CAUSE_CHARGE`, `POWER_ON_CAUSE_EXCEPTION`, `POWER_ON_CAUSE_KEY`, `POWER_ON_CAUSE_MAX`, `POWER_ON_CAUSE_RESET`: power-on flags;
* `reset()`: hard-resets the module;
* `off()`: powers the module down. **TODO: by fact, hard-resets the module, at least when USB-powered. Figure out what's wrong**;
* `idle()`: tunes the clock rate down and turns off peripherals;
* `get_input_voltage()` (float, float): the input voltage (mV) and the battery level (percents);
* `power_on_cause()` (int): the power-on flag, one of `POWER_ON_CAUSE_*`.  **TODO: never saw anything except `POWER_ON_CAUSE_CHARGE` returned, needs investigation***;
* `watchdog_on(timeout: int)`: arms the hardware watchdog with a timeout in seconds;
* `watchdog_off()`: disarms the hardware watchdog;
* `watchdog_reset()`: resets the timer on the hardware watchdog;
### `i2c`

Provides i2c functionality
(see [Ai-Thinker SDK documentation](https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/function-api/iic.html))

* `I2C_DEFAULT_TIME_OUT`
* `I2CError(message)`
* `init(id: int, freq: int)`: initializes i2c on the given port id and frequency;
* `close(id: int)`: closes i2c on the given port id;
* `receive(id: int, slave_address: int, data_length: int, timeout: int = I2C_DEFAULT_TIME_OUT)` (bytes): receives data;
* `transmit(id: int, slave_address: int, data: bytes, timeout: int = I2C_DEFAULT_TIME_OUT)`: transmits data;
* `mem_receive(id: int, slave_address: int, memory_address: int, memory_size: int, data_length: int, timeout: int = I2C_DEFAULT_TIME_OUT)` (bytes): reads memory data;
* `mem_transmit(id: int, slave_address: int, memory_address: int, memory_size: int, data: bytes, timeout: int = I2C_DEFAULT_TIME_OUT)` writes data to memory;

### `st7735`

ST7735 display over software SPI **(Note: obsolete, will be removed soon)**:

* `Display(spi: SPI, dc: Pin, reset: Pin, cs: Pin, width: int, height: int)`: a [framebuffer](https://docs.micropython.org/en/latest/library/framebuf.html) class for the ST7735 display.
  In addition to the drawing routines (the 565 color format is used), are implemented:
  * `init()`: runs display initialization,
  * `mode(rotation: int, rgb_bgr: bool)`: sets the display rotation and color order.

## Misc. ##

Other available platform-specific modules are:

* [`upip`](https://docs.micropython.org/en/latest/reference/packages.html): a package manager over the GPRS connection (built into the firmware),

* [`agps`](https://github.com/pulkin/mpy-agps): assisted GPS services (not installed by default).

## Notes ##

* The module halts on a fatal error and creates the empty file *.reboot_on_fatal* if a reboot is required.
* The size of the MicroPython heap is roughly 512KB. 400KB can be realistically allocated right after a hard reset.

