# Get online
import cellular
cellular.gprs("internet", "", "")

# Import mqtt (download client if necessary)
try:
    import agps
except ImportError:
    import upip
    upip.install("micropython-agps")
    import agps

token = "put-your-token-here"
print("AGPS location", agps.get_location_opencellid(cellular.agps_station_data(), token))

