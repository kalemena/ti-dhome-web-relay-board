
# Breadboard relay board

Using Node MCU for Wifi, there are few remaining constraints:

- GPIOs are 3.3v and we need 5v: __ULN2803__ to the rescue
> ULN2803 is a Darlington array which can be used to translate from 3.3v digital outputs to 5v

- Not enough GPIOs: __74HC595__ to the rescue
> This components enables use of 3 GPIOs to be used in serial protocol to handle in our case the 16 outputs requires for relay board


Photo (:bell: only relay 8 to 15):

![Breadboard NodeMCU](/res/breadboard-nodemcu.png)


Fritzing picture:

![Breadboard NodeMCU](/res/web-relay-board-nodemcu.png)



# Procedure

Few more details

## 74HC595

![74HC595 pins](/res/74HC595-pins.png)


We will use 3 pins from Node MCU:

| NodeMCU       | 74HC595       | Comment  |
| ------------- |:-------------:|:--------:|
| gpio13 (D7)   | pin 14        | data     |
| gpio14 (D5)   | pin 11        | clock (both 74HC595)   |
| gpio15 (D8)   | pin 12        | latch (both 74HC595)   |


:bell: Ground and 3.3v plugged obviously.

:bell: 1st 74HC595 is connected to 2nd 74HC595 from pin 9 to pin 14.


__We now have 16 I/Os !__


## ULN2803

![ULN2803 pins](/res/ULN2803-pins.jpg)


| 74HC595       | ULN2803       | Comment  |
| ------------- |:-------------:|:--------:|
| 8    | 9         | Ground   |
| 5v (from power supply)   | 10        | 5v :bell: not the 3.3v here!  |
| Q0 to Q7   | 1 to 8        | Outputs   |


# Resources

- [ESP8266 8x8 matrix](http://www.instructables.com/id/NODEMCU-LUA-ESP8266-With-74HC595-LED-and-Matrix-Dr/step2/ESP8266-driving-dual-595s-with-8-x-8-Matrix/)