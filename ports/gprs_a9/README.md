Your support is important for this port: please consider donating.

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2RZCNXCUCP4YG&source=url)

[![Build Status](https://travis-ci.org/pulkin/micropython.svg?branch=master)](https://travis-ci.org/pulkin/micropython)
[![Build Status](https://dev.azure.com/gpulkin/micropython/_apis/build/status/pulkin.micropython?branchName=master)](https://dev.azure.com/gpulkin/micropython/_build/latest?definitionId=1&branchName=master)

## Build

The firmware image is automatically built by Azure pipelines: [download](https://github.com/pulkin/micropython/releases/tag/latest-build).
Follow these steps to build from sources:

1. Install dependencies (Ubuntu example from `.travis.yml`):
   ```bash
   sudo apt-get install build-essential gcc-multilib g++-multilib libzip-dev zlib1g lib32z1
   ```
2. Clone this (install `sudo apt install git` if necessary)
   **Warning**: large download size
   ```bash
   git clone git@github.com:pulkin/micropython.git --recursive
   ```
   or (smaller download size):
   ```bash
   git clone git@github.com:pulkin/micropython.git
   git submodule update --init --recursive lib/axtls lib/GPRS_C_SDK lib/csdtk42-linux
   ```
3. Make
   ```bash
   cd micropython
   make -C mpy-cross
   cd ports/gprs_a9
   make
   ```

## Burn

Follow vendor [documentation](https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/installation_linux.html)
(`cooltools` is available under `lib/csdtk42-linux/cooltools/`).

## Connect

Use [pyserial](https://github.com/pyserial) or any other terminal.

```bash
miniterm.py /dev/ttyUSB1 115200 --raw
```

## Upload scripts

Use [ampy](https://github.com/pycampers/ampy).

```bash
ampy --port /dev/ttyUSB1 put frozentest.py 
```

## Run scripts

```python
>>> help()
>>> import frozentest
```

## Functionality

- [x] GPIO: `machine.Pin`
- [ ] ADC: `machine.ADC`
- [ ] PWM: `machine.PWM`
- [ ] UART: `machine.UART` (software UART?)
- [x] Cellular misc (IMEI, ICCID, ...): `cellular`
- [x] GPS: `gps`
- [x] I2C: `i2c`
- [x] SPI: `machine.SPI` (software only)
- [x] time: `utime`
- [x] File system
- [x] GPRS, DNS: `cellular`, `socket`, `ssl`
- [x] Power: `machine`
- [ ] Calls: `cellular`
- [x] SMS: `cellular.SMS`

## API

1. [`cellular`](#cellular): SMS, calls, connectivity
2. [`usocket`](#usocket): sockets over GPRS
3. [`ssl`](#ussl): SSL over sockets
4. [`gps`](#gps): everything related to GPS and assisted positioning
5. [`machine`](#machine): hardware and power control
6. [`i2c`](#i2c): i2c implementation
8. [Notes](#Notes)

### `cellular`

Provides cellular functionality.
As usual, the original API does not give access to radio-level and low-level functionality such as controlling the registration on the cellular network: these are performed in the background automatically.
The purpose of this module is to have an access to high-level networking (SMS, GPRS, calls) as well as to read the status of various components of cellular networking.

* `NETWORK_FREQ_BAND_GSM_900P`, `NETWORK_FREQ_BAND_GSM_900E`, `NETWORK_FREQ_BAND_GSM_850`, `NETWORK_FREQ_BAND_DCS_1800`, `NETWORK_FREQ_BAND_PCS_1900`, `NETWORK_FREQ_BANDS_ALL`: frequencies;
* `OPERATOR_STATUS_UNKNOWN`, `OPERATOR_STATUS_AVAILABLE`, `OPERATOR_STATUS_CURRENT`, `OPERATOR_STATUS_DISABLED`: operator statuses;
* `NETWORK_MODE_MANUAL`, `NETWORK_MODE_AUTO`, `NETWORK_MODE_MANUAL_AUTO`: network registration modes;
* `SMS(phone_number: str, message: str)`: handles SMS messages;
  * `.phone_number` (str): phone number (sender or destination);
  * `.message` (str): message contents;
  * `.status` (int): integer with status bits;
  * `.inbox` (bool): incoming message if `True`, outgoing message if `False` or unknown status if `None`;
  * `.unread` (bool): unread message if `True`, previously read message if `False` or unknown status if `None`;
  * `.sent` (bool): sent message if `True`, not sent message if `False` or unknown status if `None`;
  * `.send()`: sends a message;
  * `.withdraw()`: withdraws SMS from SIM storage;
  * `.poll()` (int) [staticmethod]: the number of new SMS received;
  * `.list()` (list) [staticmethod]: all SMS from the SIM card;
* `CellularError(message: str)`
* `get_imei()` (str): the International Mobile Equipment Identity (IMEI) number;
* `get_iccid()` (str): the Integrated Circuit Card ID (ICCID) number of the inserted SIM card;
* `get_imsi()` (str): the International Mobile Subscriber Identity (IMSI) number of the inserted SIM card;
* `network_status_changed()` (bool): indicates whether the network status changed since the last check;
* `get_network_status()` (int): cellular network status encoded in an integer. **TODO**: Provide bit-wise specs;
* `poll_network_exception()`: polls the network exception and raises it, if any;
* `is_sim_present()` (bool): checks whether a SIM card is present;
* `is_network_registered()` (bool): checks whether registered on the cellular network;
* `is_roaming()` (bool): checks whether registered on the roaming network;
* `get_signal_quality()` (int, int): the signal quality (0-31) and RXQUAL. These are replaced by `None` if no signal quality information is available. **TODO**: The RXQUAL output is always `None`;
* `flight_mode([flag: bool])` (bool): the flight mode status. Turns in on or off if the argument is specified;
* `set_bands(bands: int = NETWORK_FREQ_BANDS_ALL)`: sets frequency bands;
* `scan()` (list): lists available operators: returns `(op_id: bytearray[6], op_name: str, op_status: int)` for each;
* `register([operator_id: bytearray[6], register_mode: int])` (op_id: bytearray[6], op_name: str, reg_status: int): registered network operator information. Registers on the network if arguments supplied. **TODO**: Figure out how (and whether) registration works at all;
* `stations()` (list): a list of nearby stations: `(mcc, mnc, lac, cell_id, bsic, rx_full, rx_sub, arfcn)`: all ints;
* `reset()`: resets network settings to defaults. Disconnects GPRS;
* `gprs([apn: {str, bool}[, user: str, pass: str]])` (bool): activate (3 arguments), deactivate (`gprs(False)`) or obtain the status of GPRS (on/off) if no arguments supplied;
* `call()` (list[str], [str, None]): calls missed (1st output) and the incoming call number or `None` if no incoming calls at the moment (2nd output);

### `usocket` ###

*Alias: `socket`*

TCP/IP stack over GPRS based on lwIP.
See [micropython docs](https://docs.micropython.org/en/latest/library/usocket.html) for details.

* `AF_INET`, `AF_INET6`, `SOCK_STREAM`, `SOCK_DGRAM`, `SOCK_RAW`, `IPPROTO_TCP`, `IPPROTO_UDP`, `IPPROTO_IP`: lwIP constants;
* `socket(af: int, type: int, proto: int)`: socket class;
    * `close()`
    * `bind(address)` [not implemented]
    * `listen([backlog])` [not implemented]
    * `accept()` [not implemented]
    * `connect(address)`
    * `send(bytes)`
    * `sendall(bytes)`
    * `recv(bufsize)`
    * `sendto(bytes, address)`
    * `recvfrom(bufsize)`
    * `setsockopt(level, optname, value)` [not implemented]
    * `settimeout(value)` [not implemented]
    * `setblocking(flag)` [not implemented]
    * `makefile(mode, buffering)`
    * `read([size])`
    * `readinto(buf[, nbytes])`
    * `readline()`
    * `write(buf)`
* `get_local_ip()` (str): the local IP address;
* `getaddrinfo(host (str), port (str), af (int) = AF_INET, type (int) = SOCK_STREAM, proto (int) = IPPROTO_TCP, flags (int) = 0)` (tuple): translates host/port into arguments to socket constructor;
* `inet_ntop(af: int, bin_addr: bytearray)` (str) [not implemented]
* `inet_pton(af: int, txt_addr: str)` (bytearray) [Not implemented]
* `get_num_open()` (int): the number of open sockets (max 8);

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
This is only available in the A9G module where GPS is a separate chip connected via UART2.

* `GPSError(message: str)`
* `on()`: turns the GPS on;
* `off()`: turns the GPS off;
* `get_firmware_version()` (str): retrieves the firmware version;
* `get_location()` (latitude: float, longitude: float): retrieves the current GPS location;
* `get_last_location()` (latitude: float, longitude: float): retrieves the last known GPS location without polling the GPS module;
* `get_satellites()` (tracked: int, visible: int): the numbers of satellites in operation;

### `machine`

Provides power-related functions: power, watchdogs.

* `POWER_ON_CAUSE_ALARM`, `POWER_ON_CAUSE_CHARGE`, `POWER_ON_CAUSE_EXCEPTION`, `POWER_ON_CAUSE_KEY`, `POWER_ON_CAUSE_MAX`, `POWER_ON_CAUSE_RESET`: power-on flags;
* `reset()`: hard-resets the module;
* `off()`: powers the module down. **TODO**: By fact, hard-resets the module, at least when USB-powered. Figure out what's wrong;
* `idle()`: tunes the clock rate down and turns off peripherials;
* `get_input_voltage()` (float, float): the input voltage (mV) and the battery level (percents);
* `power_on_cause()` (int): the power-on flag, one of `POWER_ON_CAUSE_*`.  **TODO**: never saw anything except `POWER_ON_CAUSE_CHARGE` returned, needs investigation;
* `watchdog_on(timeout: int)`: arms the hardware watchdog with a timeout in seconds;
* `watchdog_off()`: disarms the hardware watchdog;
* `watchdog_reset()`: resets the timer on the hardware watchdog;
  
### `i2c`

Provides i2c functionality
(see [sdk docs](https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/function-api/iic.html))

* `I2C_DEFAULT_TIME_OUT`
* `I2CError(message)`
* `init(id: int, freq: int)`: inintializes i2c on the given port id and frequency;
* `close(id: int)`: closes i2c on the given port id;
* `receive(id: int, slave_address: int, data_length: int, timeout: int = I2C_DEFAULT_TIME_OUT)` (bytes): receives data;
* `transmit(id: int, slave_address: int, data: bytes, timeout: int = I2C_DEFAULT_TIME_OUT)`: transmits data;
* `mem_receive(id: int, slave_address: int, memory_address: int, memory_size: int, data_length: int, timeout: int = I2C_DEFAULT_TIME_OUT)` (bytes): reads memory data;
* `mem_transmit(id: int, slave_address: int, memory_address: int, memory_size: int, data: bytes, timeout: int = I2C_DEFAULT_TIME_OUT)` writes data to memory;

## Notes ##

* The module halts on fatal errors; create an empty file `.reboot_on_fatal` if a reboot is desired
* The size of micropython heap is roughly 512 Kb. 400k can be realistically allocated right after hard reset.

