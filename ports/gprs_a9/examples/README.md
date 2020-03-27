Examples for micropython on a9g
===============================

[example_01_blink.py](example_01_blink.py)

Blinks the built-in blue LED on the pudding board connected to GPIO 27.

Keywords: **gpio, machine, blink, hello world**

Modules: `machine`, `time`

[example_05_watchdog.py](example_05_watchdog.py)

Arms the hardware watchdog and sleeps until hard reset.

Keywords: **watchdog, hardware, reset**

Modules: `machine`, `time`

[example_20_gps.py](example_20_gps.py)

Displays current GPS location and visible satellites.

Keywords: **GPS**

Modules: `gps`

[example_21_agps.py](example_21_agps.py)

Displays current location determined by cell tower identifiers using [agps](https://github.com/pulkin/mpy-agps).

Keywords: **GPS, aGPS**

Modules: `cellular`, `agps` (external), `upip`

[example_30_network.py](example_30_network.py)

Tests connection to the internet by running a raw HTTP request.

Keywords: **internet, TCP, HTTP, GPRS, connection**

Modules: `cellular`, `socket`

[example_31_ssl.py](example_31_ssl.py)

Test an encrypted SSL HTTP request.

Keywords: **HTTPS, SSL, encryption**

Modules: `cellular`, `socket`, `ssl`

[example_32_http.py](example_32_http.py)

Performs a simple URL request using `urequests` from [micropython-lib](https://github.com/micropython/micropython-lib).

Keywords: **URL, requests, HTTP, HTTPS**

Modules: `cellular`, `urequests` (external), `upip`

[example_50_mqtt.py](example_50_mqtt.py)

Reports location to a test MQTT server via cellular connection using `umqtt` from [micropython-lib](https://github.com/micropython/micropython-lib).

Keywords: **MQTT, GPS, track**

Modules: `cellular`, `umqtt` (external), `gps`, `upip`

[example_51_traccar.py](example_51_traccar.py)

Reports location to [traccar](https://www.traccar.org/) online service using a simple POST request without external libraries.

Keywords: **GPS, track, traccar**

Modules: `cellular`, `gps`, `socket`

