
# Breadboard relay board

Using Node MCU for Wifi, there are few remaining constraints:

- GPIOs are 3.3v and we need 5v: __ULN2803__ to the rescue
 ULN2803 is a Darlington array which can be used to translate from 3.3v digital outputs to 5v

- Not enough GPIOs: __74HC595__ to the rescue
 This components enables use of 3 GPIOs to be used in serial protocol to handle in our case the 16 outputs requires for relay board


# Procedure

Few more details

## 74HC595

![74HC595 pins](/res/74HC595-pins.png)


## ULN2803

![ULN2803 pins](/res/ULN2803-pins.png)


# Wiring

![Breadboard NodeMCU](/res/web-relay-board-nodemcu.png)


# Sketch

