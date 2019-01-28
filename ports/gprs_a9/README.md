
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
Retrieves IMEI number.
**Returns**: a string with IMEI number.

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
