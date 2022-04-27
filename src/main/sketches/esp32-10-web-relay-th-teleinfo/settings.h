// Webserver Config
const char *ssid = "<ssid>";
const char *password = "<password>";
const char* host = "ioteleinfo";

// NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

//// PINS (74HC595) - ESP8266
//int latchPin = 15;
//int clockPin = 12;
//int dataPin = 13;

// PINS (74HC595) - ESP32
int latchPin = 5;
int clockPin = 19;
int dataPin = 23;

// #define ENABLE_HTU21D 0

// Relay Map
#define ARRAYSIZE 16
String sensors[ARRAYSIZE] = { "Ch. Jeux", "Bureau", "Ch. Lena", "Ch. Damien", "SdB Haut", "Mezanine", "Ch. Bas", "SdB Bas", "Portail", "NA", "NA", "NA", "NA", "NA", "Salon", "Hall" };
