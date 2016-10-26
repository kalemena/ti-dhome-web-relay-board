
# Web Relay Board

The idea is to create a Web Relay Board.

Specifications:
- 16 relays
- REST API
- Wifi

# BOM

Total ~ 20€

- [SainSmart 16 relays](http://www.sainsmart.com/relay-1/16-channel-12v-relay-module-for-pic-arm-avr-dsp-arduino-msp430-ttl-logic.html) (~ 12€)
- [ESP-12 8266](http://www.ebay.fr/itm/5119-ESP12-E-esp8266-module-wifi-sans-fils-ARDUINO-ESP8266-ESP12E-/191849920712) (~ 4€)
- [2x 74HC595](http://www.ebay.fr/itm/20-x-74HC595-8-bit-Shift-Register-IC-DIP-16-TEXAS-/260843227719) (~ 4€ x 20)
- [2x ULN2803](http://www.ebay.fr/itm/20x-ULN2803APG-ULN2803-DIP-18-Transistor-TOSHIBA-DARLINGTON-ARRAYS-Buffer-Driver-/350899601550) (~ 2.5€ x 20)

Prototyping:

- Dev Board: [NodeMCU](http://www.ebay.fr/itm/NodeMcu-V3-Lua-WeMos-WiFi-Wireless-Module-CH340-Development-Board-ESP8266-ESP12E-/322164935016) (~ 7€)

Production:

- Printed PCB: [Example at DFRobot](https://www.dfrobot.com/index.php?route=product/pcb&product_id=1351) (~ 10€ x 10)


# Prototyping iterations

Here are described step by step experiments to reach final product.

[01 Setup ESP8266 board in Arduino IDE](iterations/1-setup-arduino-ide-for-esp8266.md)


[02 Hello ESP8266 Wifi](iterations/1-wifi-esp8266)

http://www.esp8266.com/viewtopic.php?f=29&t=2153


[03 Understanding 74HC595](iterations/1-74HC595)

https://www.arduino.cc/en/Tutorial/ShftOut11


[04 Understanding ULN2803](iterations/1-ULN2803)


