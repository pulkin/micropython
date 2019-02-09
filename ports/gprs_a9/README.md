
## install GPRS_C_SDK toolchain

* [installation doc](https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/installation_linux.html)
* Or [docker](https://github.com/Neutree/gprs_build)


```
git clone git@github.com:Neutree/micropython.git
make -C mpy-cross
cd micropython/ports/gprs_a9
chmod +x build.sh
./build.sh
```

and burn the final lod file in the `hex` folder.

## Platform-specific modules

### `cellular`

Provides cellular functionality.

* `get_imei()`

  Retrieves the International Mobile Equipment Identity (IMEI) number.

  **Returns**: a string with IMEI number.

* `get_iccid()`

  Retrieves the Integrated Circuit Card ID (ICCID) number of the inserted SIM card.

  **Returns**: a string with ICCID number.

  **Raises**: `ValueError` if no ICCID number can be retrieved.

* `get_imsi()`

  Retrieves the International Mobile Subscriber Identity (IMSI) number of the inserted SIM card.

  **Returns**: a string with IMSI number.

  **Raises**: `ValueError` if no IMSI number can be retrieved.

* `is_sim_present()`

  Checks whether the SIM card is present and ICCID can be retrieved.

  **Returns**: True if SIM card is present.

* `is_network_registered()`

  Checks whether registered on the cellular network.

  **Returns**: True if registered.

* `is_roaming()`

  Checks whether registered on the roaming network.

  **Returns**: True if roaming.

  **Raises** `ValueError` if not registered ar all.

* `sms_send(destination, message)`

  Sends SMS.

  **Args**:

    * destination (str): telephone number;
    * message (str): message contents;

  **Raises**: `ValueError` if failed to set up/send SMS.

### `gps`

Provides the GPS functionality

* `on()`

  Turns the GPS on. Blocks until the GPS module responds.

  **Raises**: `ValueError` if the GPS module does not respond within 10 seconds.

* `off()`

  Turns the GPS off.

* `get_firmware_version()`

  Retrieves the firmware version.

  **Returns**: the firmware version as a string.

  **Raises**: `ValueError` if the GPS module fails to respond.

* `get_location()`

  Retrieves the current GPS location.

  **Returns**: latitude and longitude in degrees.

  **Raises**: `ValueError` if the GPS module never responded or is off.

* `get_last_location()`

  Retrieves the last GPS location.

  **Returns**: latitude and longitude in degrees.

  **Raises**: `ValueError` if the GPS module never responded.

* `get_satellites()`

  Retrieves the number of satellites visible.

  **Returns**: the number of satellites tracked and the number of visible satellites.

  **Raises**: `ValueError` if the GPS module never responded.

### `machine`

Provides power-related functions.

* `reset()`

  Hard-resets the module.

* `off()`

  Powers the module down.
  **Note**: By fact, hard-resets the module, at least when USB-powered.

* `idle()`

  Reduces clock rates of the module keeping functionality.

* `get_input_voltage()`

  Retrieves the input voltage and the battery percentage.

  **Returns**: two numbers: the voltage in `mV` and the estimated percentage (Lithium battery discharge controller).
  **Note**: also works when USB-powered (the second number returned is irrelevant).

* `power_on_cause()`

  Retrieves the reason for powering the module on.

  **Returns**: one of `POWER_ON_CAUSE_ALARM`, `POWER_ON_CAUSE_CHARGE`, `POWER_ON_CAUSE_EXCEPTION`, `POWER_ON_CAUSE_KEY`, `POWER_ON_CAUSE_MAX`, `POWER_ON_CAUSE_RESET`.
  **Note**: never saw anything except `POWER_ON_CAUSE_CHARGE` returned (needs further investigation).

* `watchdog_on(timeout)`

  Arms the hardware watchdog.

  **Args**:

    * timeout (int): timeout in seconds;

* `watchdog_off()`

  Disarms the hardware watchdog.

* `watchdog_reset()`

  Resets the timer on the hardware watchdog.

