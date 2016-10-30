
# Relay board sketch

[First sketch](/iterations/3-relay-board-sketch.md) is not very efficient.

It only touched simple ESP8266 capability.

This iteration is about going deeper into the topic:
- host static files such as images or CSS files
- use the SPIFFS ~3MB flash storage
- better HTML rendering
- web sockets


# Steps

## Un coup dans le 'SPIFFS'

To leverage most of the 4MB disk from ESP8266, install plugin:

[Arduino IDE SPIFFS plugin](https://github.com/esp8266/arduino-esp8266fs-plugin)


Usage is pretty simple:
- create subfolder from sketch folder named 'data'
- use Arduino IDE / Tools / ESP Sketch Data Upload
- this takes very long as it uploads ~ 3MB (no matter what)


# Sketch details





# Resources

