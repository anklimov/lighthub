#!/usr/bin/env bash
mosquitto_pub -t 'domoticz/in' -h broker_url -p port -u user -P pass -m '{"command": "udevice", "idx": 20, "svalue": "15"}'
