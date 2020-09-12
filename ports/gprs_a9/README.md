[![Build Status](https://travis-ci.org/pulkin/micropython.svg?branch=master)](https://travis-ci.org/pulkin/micropython)
[![Build Status](https://dev.azure.com/gpulkin/micropython/_apis/build/status/pulkin.micropython?branchName=master)](https://dev.azure.com/gpulkin/micropython/_build/latest?definitionId=1&branchName=master)
[![Chat](https://img.shields.io/discord/725112070798049291)](https://discord.com/channels/725112070798049291)
[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2RZCNXCUCP4YG&source=url)

## Contribute

See [CONTRIBUTING.md](CONTRIBUTING.md) or use the donate button.

## The module

![A9G](https://raw.githubusercontent.com/Ai-Thinker-Open/GPRS_C_SDK/master/doc/assets/pudding_pin.png)

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
>>> import blink
blink.blink(1)
```

## Functionality

- [x] GPIO: `machine.Pin`
- [x] ADC: `machine.ADC`
- [ ] PWM: `machine.PWM`
- [x] UART: `machine.UART` (hw)
- [x] SPI: `machine.SPI` (sw)
- [x] RTC: `machine.RTC`
- [x] I2C: `i2c` (hw)
- [x] Cellular misc (IMEI, ICCID, ...): `cellular`
- [x] GPS: `gps`
- [x] time: `utime`
- [x] File system
- [x] GPRS, DNS: `cellular`, `socket`, `ssl`
- [x] Power: `machine`
- [x] Calls: `cellular`
- [x] SMS: `cellular.SMS`

Additional libraries: install via [`upip`](https://docs.micropython.org/en/latest/reference/packages.html) (built into the module).

## Examples

See [examples](examples) folder.

## API

Full module list
```python
>>> help('modules')
__main__          math              ubinascii         urandom
_boot             micropython       ucollections      ure
_uasyncio         network           ucryptolib        uselect
builtins          ntptime           uctypes           usocket
cellular          sys               uerrno            ussl
chip              uarray            uhashlib          ustruct
cmath             uasyncio/__init__ uheapq            utime
framebuf          uasyncio/core     uio               utimeq
gc                uasyncio/event    ujson             uzlib
gps               uasyncio/funcs    uos
i2c               uasyncio/lock     upip
machine           uasyncio/stream   upip_utarfile
```

Featured:

1. [`cellular`](#cellular), `network`: SMS, calls, connectivity
2. [`usocket`](#usocket): sockets over GPRS
3. [`ssl`](#ussl): SSL over sockets
4. [`gps`](#gps): everything related to GPS and assisted positioning
5. [`machine`](#machine): hardware and power control
6. [`i2c`](#i2c): i2c implementation
7. [Other modules](#Misc)
8. [Notes](#Notes)

### `cellular`

Provides cellular functionality.
As usual, the original API does not give access to radio-level and low-level functionality such as controlling the registration on the cellular network: these are performed in the background automatically.
The purpose of this module is to have an access to high-level networking (SMS, GPRS, calls) as well as to read the status of various components of cellular networking.

#### Constants

* `NETWORK_FREQ_BAND_GSM_900P`, `NETWORK_FREQ_BAND_GSM_900E`, `NETWORK_FREQ_BAND_GSM_850`, `NETWORK_FREQ_BAND_DCS_1800`, `NETWORK_FREQ_BAND_PCS_1900`, `NETWORK_FREQ_BANDS_ALL`: frequencies;
* `OPERATOR_STATUS_UNKNOWN`, `OPERATOR_STATUS_AVAILABLE`, `OPERATOR_STATUS_CURRENT`, `OPERATOR_STATUS_DISABLED`: operator statuses;
* `NETWORK_MODE_MANUAL`, `NETWORK_MODE_AUTO`, `NETWORK_MODE_MANUAL_AUTO`: network registration modes;
* `SMS_SENT`: constant for event handler `on_sms`;
* `ENOSIM`, `EREGD`, `ESMSSEND`, `ESMSDROP`, `ESIMDROP`, `EATTACHMENT`, `EACTIVATION`, `ENODIALTONE`, `EBUSY`, `ENOANSWER`, `ENOCARRIER`, `ECALLTIMEOUT`, `ECALLINPROGRESS`, `ECALLUNKNOWN`: extended codes for `OSError`s raised by the module;

#### Classes

* `SMS(phone_number: str, message: str[, pn_type: int, index: int, purpose: int])`: handles SMS messages;
  * `.phone_number` (str): phone number (sender or destination);
  * `.message` (str): message contents;
  * `.purpose` (int): integer with purpose/status bits;
  * `.is_inbox` (bool): indicates incoming message;
  * `.is_read` (bool): indicates message was previously read;
  * `.is_unread` (bool): indicates unread message;
  * `.is_unsent` (bool): indicates unsent message;
  * `.send(timeout: int)`: sends a message;
  * `.withdraw()`: withdraws SMS from SIM storage;
  * `.list()` (list) [staticmethod]: all SMS from the SIM card;
  * `.get_storage_size()` (int, int) [staticmethod]: number of active SMS records and total storage size;
  * ~~`.poll()` (int) [staticmethod]: the number of new SMS received~~ use `on_sms` instead;
* ~~`CellularError(message: str)`~~ `OSError`, `ValueError`, `RuntimeError` are used instead;

#### Methods

* `get_imei()` (str): the International Mobile Equipment Identity (IMEI) number;
* `get_iccid()` (str): the Integrated Circuit Card ID (ICCID) number of the inserted SIM card;
* `get_imsi()` (str): the International Mobile Subscriber Identity (IMSI) number of the inserted SIM card;
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
* `agps_station_data()` (int, int, list): a convenience function returning `(mcc, mnc, [(lac, cell_id, signal_strength), ...])` for use in agps location: all ints;
* `reset()`: resets network settings to defaults. Disconnects GPRS;
* `gprs([apn: {str, bool}[, user: str, pass: str[, timeout: int]]])` (bool): activate (3 or 4 arguments), deactivate (`gprs(False)`) or obtain the status of GPRS (on/off) if no arguments supplied;
* `dial(tn: {str, bool})`: dial a telephone number if string is supplied or hang up a call if `False`;
* `ussd(code: str[, timeout: int])` (int, str): USSD request. Unless zero timeout specified, returns USSD response option code and the response text;
* `on_status_event(callback: Callable)`: sets a callback `function(status: int)` for network status change;
* `on_sms(callback: Callable)`: sets a callback `function(sms_or_status)` on SMS sent or received;;
* `on_call(callback: Callable)`: sets a callback `function(number_or_hangup)` on call events (incoming, hangup, etc.);
* ~~`network_status_changed()` (bool): indicates whether the network status changed since the last check~~ use `on_status_event` instead;
* ~~`call()` (list[str], [str, None]): calls missed (1st output) and the incoming call number or `None` if no incoming calls at the moment (2nd output)~~ use `on_call` instead;

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

* ~~`GPSError(message: str)`~~ OSError used instead
* `on()`: turns the GPS on;
* `off()`: turns the GPS off;
* `get_firmware_version()` (str): retrieves the firmware version;
* `get_location()` (longitude: float, latitude: float): retrieves the current GPS location;
* `get_last_location()` (longitude: float, latitude: float): retrieves the last known GPS location without polling the GPS module;
* `get_satellites()` (tracked: int, visible: int): the numbers of satellites in operation;
* `time()` (int): the number of seconds since the epoch (2000). Use `time.localtime` for converting it into date/time values (this conversion may result in `OverflowError` until the GPS module starts reading meaningful satellite data);
* `nmea_data()` (tuple): all NMEA data parsed: `(rmc, (gsa[0], ...), gga, gll, gst, (gsv[0], ...), vtg, zda)`:
  - RMC: `(time: int, valid: bool, latitude, longitude, speed, course, variation: float)`;
  - GSA: `(mode: int, fix_type: int, satellite_prn: bytearray, pdop, hdop, vdop: float)`;
  - GGA: `(time_of_day: int, latitude, longitude: float, fix_quality, satellites_tracked: int, hdop, altitude: float, altitude_units: int, height: float, height_units: int, dgps_age: float)`;
  - GLL: `(latitude, longitude: float, time_of_day, status, mode: int)`;
  - GST: `(time_of_day: int, rms_deviation, ...: float)`;
  - GSV: `(total_messages, message_nr, total_satellites: int, satellite_info[4]: (nr, elevation, azimuth, snr: int))`;
  - VTG: `(true_track_degrees, magnetic_track_degrees, speed_knots, speed_kph: float, faa_mode: int)`;
  - ZDAL `(time, hour_offset, minute_offset: int)`.

  Latitudes and longitudes are in degrees `x100`.
  Time is given in seconds since the epoch or since `00:00` today.
  Status flags `mode`, `status` are ASCII indexes.
  For more info (units, etc) please consult the [minmea](https://github.com/kosma/minmea) project.

### `machine`

Provides power-related functions: power, watchdogs.

#### Constants

* `POWER_ON_CAUSE_ALARM`, `POWER_ON_CAUSE_CHARGE`, `POWER_ON_CAUSE_EXCEPTION`, `POWER_ON_CAUSE_KEY`, `POWER_ON_CAUSE_MAX`, `POWER_ON_CAUSE_RESET`: power-on flags.

#### Methods

* `reset()`: hard-resets the module;
* `off()`: powers the module down. **TODO**: By fact, hard-resets the module, at least when USB-powered. Figure out what's wrong;
* `idle()`: tunes the clock rate down and turns off peripherials;
* `get_input_voltage()` (float, float): the input voltage (mV) and the battery level (percents);
* `power_on_cause()` (int): the power-on flag, one of `POWER_ON_CAUSE_*`.  **TODO**: never saw anything except `POWER_ON_CAUSE_CHARGE` returned, needs investigation;
* `watchdog_on(timeout: int)`: arms the hardware watchdog with a timeout in seconds;
* `watchdog_off()`: disarms the hardware watchdog;
* `watchdog_reset()`: resets the timer on the hardware watchdog;
* `on_power_key(callback: Callable)`: sets a callback `function(is_power_key_down: bool)` on power key events.
  
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

## Misc ##

Built into the firmware:

* [`upip`](https://docs.micropython.org/en/latest/reference/packages.html): package manager over GPRS connection;

Available platform-specific modules (not installed by default):

* [`agps`](https://github.com/pulkin/mpy-agps): assisted GPS services.

## Notes ##

* The module halts on fatal errors; create an empty file `.reboot_on_fatal` if a reboot is desired
* The size of micropython heap is roughly 512 Kb. 400k can be realistically allocated right after hard reset.
* The external memory card is [mounted under `/t`](https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/function-api/file-system.html).

