## Install GPRS_C_SDK toolchain

* [installation doc](https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/installation_linux.html)
* Or [docker](https://github.com/Neutree/gprs_build)


```bash
git clone git@github.com:Neutree/micropython.git
make -C mpy-cross
cd micropython/ports/gprs_a9
chmod +x build.sh
./build.sh
```

and burn the final `.lod` file in the `hex` folder.

## Platform-specific modules

### `cellular`

Provides cellular functionality.
As usual, the original API does not give access to radio-level and low-level functionality such as controlling the registration on the cellular network: these are performed in the background automatically.
The purpose of this module is to have an access to high-level networking (SMS, GPRS, calls) as well as to read the status of various components of cellular networking.

#### Classes ####

* `SMS(phone_number, message)`

  A class for handling SMS.

  **Attrs**:

    * phone_number (str): phone number (sender or destination);
    * message (str): message contents;
    * status (int): an integer with status bits;
    * inbox (bool): incoming message if `True`, outgoing message if `False` or unknown status if `None`;
    * unread (bool): unread message if `True`, previously read message if `False` or unknown status if `None`;
    * sent (bool): sent message if `True`, not sent message if `False` or unknown status if `None`;
    * `send()`
    
      Sends the message.

      **Raises**: `CelularError` if not registered on the network or failed to set up/send SMS.

    * `withdraw()`

      Withdraws SMS from SIM storage. Resets status and index of this object to zero.

      **Raises**: `CellularError` if failed to withdraw.

#### Exception classes ####

* `CellularError(message)`

#### Methods ####

* `get_imei()`

  Retrieves the International Mobile Equipment Identity (IMEI) number.

  **Returns**: a string with IMEI number.

* `get_iccid()`

  Retrieves the Integrated Circuit Card ID (ICCID) number of the inserted SIM card.

  **Returns**: a string with ICCID number.

  **Raises**: `CellularError` if no ICCID number can be retrieved.

* `get_imsi()`

  Retrieves the International Mobile Subscriber Identity (IMSI) number of the inserted SIM card.

  **Returns**: a string with IMSI number.

  **Raises**: `CellularError` if no IMSI number can be retrieved.

* `network_status_changed()`

  Checks whether the network status was changed since the last check.

  **Returns**: `True` if it was changed.

* `get_network_status()`

  Retrieves the network status as an integer.

  **Returns**: `int` representing cellular network status.

  **TODO**: Provide bit-wise specs.

* `get_network_exception()`

  Retrieves the network exception.

  **Returns**: `int` representing the last network exception. Returns zero if no exception occurred since the last check.

* `is_sim_present()`

  Checks whether the SIM card is present and ICCID can be retrieved.

  **Returns**: True if SIM card is present.

* `is_network_registered()`

  Checks whether registered on the cellular network.

  **Returns**: True if registered.

* `is_roaming()`

  Checks whether registered on the roaming network.

  **Returns**: True if roaming.

  **Raises** `CellularError` if not registered on the network.

* `get_signal_quality()`

  Retrieves signal quality.

  **Returns**: Two integers, the signal quality (0-31) and RXQUAL. These are replaced by `None` if no signal quality information is available.
  **Note**: The RXQUAL output is always `None`. Its meaning is unknown.

* `sms_list()`

  Retrieves SMS from the SIM card.
  
  **Returns**: a list of SMS messages.

  **Raises**: `CellularError` if not registered on the network. This is because network registration process interfers with most other SIM-related operations.

### `gps`

Provides the GPS functionality.
This is only available in the A9G module where GPS is a separate chip connected via UART2.

#### Methods ####

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

Provides power-related functions: power, watchdogs.

#### Methods ####

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

