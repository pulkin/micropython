# Micropython a9g example
# Source: https://github.com/pulkin/micropython
# Author: pulkin
# Demonstrates how to retrieve location (latitude and longitude) from GSM base stations (AGPS)

# Get online
import cellular
cellular.gprs("internet", "", "")

# Import agps (download client if necessary)
try:
    import agps
except ImportError:
    import upip
    upip.install("micropython-agps")
    import agps

# Get your token from https://unwiredlabs.com
token = "put-your-token-here"
print("AGPS location", agps.get_location_opencellid(cellular.agps_station_data(), token))

