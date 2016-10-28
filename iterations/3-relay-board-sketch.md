
# Relay board sketch

Breadboard prototype done at [02 Relay Board Prototype](iterations/2-relay-board-prototype.md)

Now is time for a bit of coding.

# How-To

Here is the first [Basic sketch](/sketches/esp8266-web-relay-wifi/esp8266-web-relay-wifi.ino)

Details of mathematics in next section.


Steps:
- Uploaded from Arduino IDE
- Open Arduino console
- This should log the URL with IP address to connect to Wifi device
- connect using browser to see the table where you can switch on and off the relays


# Mathematics

## Binary computation

We have wired 16 outputs.

This in turns can be mapped to an integer from 0 to 65535.

In Arduino code, this means [__unsigned int__](https://www.arduino.cc/en/Reference/UnsignedInt).

Let's see what this means from [Byte mathematics](http://playground.arduino.cc/Code/BitMath#quickref)

```js
unsigned int relayState = 0;

// state of each relay (switchId is relay number from 0 to 15):
boolean switchState = ((relayState >> switchId) & 1);

// Set relay to 0:
relayState &= ~(1 << relayNb);

// Set relat to 1:
relayState |= (1 << relayNb);
```

## 74HC595 - ShiftOut

The [74HC595 ShiftOut](https://www.arduino.cc/en/Reference/ShiftOut) states that it is 8 bit and requires two steps operation to shift bits.

```js
// Value is anything between 0 to 65535 representing 16 bits of data I/Os
void switchRelay(int value) 
{
   // take the latchPin low:
   digitalWrite(latchPin, LOW);

   // shift out the highbyte
   shiftOut(dataPin, clockPin, MSBFIRST, (value >> 8));
   // shift out the lowbyte
   shiftOut(dataPin, clockPin, MSBFIRST, value);

   //take the latch pin high so the LEDs will light up:
   digitalWrite(latchPin, HIGH);
}
```


# Resources


- [74HC595 ShiftOut](https://www.arduino.cc/en/Reference/ShiftOut)

- [Byte mathematics](http://playground.arduino.cc/Code/BitMath#quickref)