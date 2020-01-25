# Micropython a9g example
# Source: https://github.com/pulkin/micropython
# Author: pulkin
# Demonstrates how to use mqtt for publishing GPS location

# Get online
import cellular
cellular.gprs("internet", "", "")

# Import mqtt (download client if necessary)
try:
    from umqtt import simple
except ImportError:
    import upip
    upip.install("micropython-umqtt.simple")
    from umqtt import simple

# Turn GPS on
import gps
gps.on()

# Report location
name = "a9g-micropython-board"
server = "test.mosquitto.org"
topic = "a9g-micropython-board-topic"
print("To track messages run, for example\n  mosquitto_sub -h {server} -t \"{topic}\" -v".format(server=server, topic=topic))
import json
client = simple.MQTTClient(name, server)
client.connect()
data = json.dumps(gps.get_last_location())
print("Publishing", data)
client.publish(topic, data)

