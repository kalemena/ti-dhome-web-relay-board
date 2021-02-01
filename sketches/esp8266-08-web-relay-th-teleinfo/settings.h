// Webserver Config
const char *ssid = "<ssid>";
const char *password = "<password>";
const char* host = "<host>";

// PINS (74HC595)
int latchPin = 15;
int clockPin = 12;
int dataPin = 13;

// Relay Map

#define ARRAYSIZE 16
String sensors[ARRAYSIZE] = { "Ch. Jeux", "Bureau", "Ch. Lena", "Ch. Damien", "SdB Haut", "Mezanine", "Ch. Bas", "SdB Bas", "Portail", "NA", "NA", "NA", "NA", "NA", "Salon", "Hall" };
