
// 74HC595
//Pin connected to ST_CP of 74HC595
int latchPin = 15;
//Pin connected to SH_CP of 74HC595
int clockPin = 14;
////Pin connected to DS of 74HC595
int dataPin = 13;

void setup(void)
{
  Serial.begin(115200);

  // 74HC595
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
}
 
void loop(void)
{  
  int pause = 500;
  switchRelay(0); delay(pause);
  switchRelay(1); delay(pause);
  switchRelay(2); delay(pause);
  switchRelay(4); delay(pause);
  switchRelay(8); delay(pause);
  switchRelay(16); delay(pause);
  switchRelay(32); delay(pause);
  switchRelay(64); delay(pause);
  switchRelay(128); delay(pause);
  switchRelay(256); delay(pause);
  switchRelay(512); delay(pause);
  switchRelay(1024); delay(pause);
  switchRelay(2048); delay(pause);
  switchRelay(4096); delay(pause);
  switchRelay(8192); delay(pause);
  switchRelay(16384); delay(pause);
  switchRelay(32768); delay(pause);
} 

/**
Value is anything between 0 to 65535 representing 16 bits of data I/Os
*/
void switchRelay(int value) 
{
   // take the latchPin low so 
   // the LEDs don't change while you're sending in bits:
   digitalWrite(latchPin, LOW);
   
   // shift out the highbyte
   shiftOut(dataPin, clockPin, MSBFIRST, (value >> 8));
   // shift out the lowbyte
   shiftOut(dataPin, clockPin, MSBFIRST, value);
   
   //take the latch pin high so the LEDs will light up:
   digitalWrite(latchPin, HIGH);
}
