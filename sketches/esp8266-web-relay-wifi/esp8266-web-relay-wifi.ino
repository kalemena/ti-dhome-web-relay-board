#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "<ssid>";
const char* password = "<passphrase>";
const int port = 80;

// 74HC595
int latchPin = 15;
int clockPin = 14;
int dataPin = 13;

unsigned int relayState = 0;

// Node MCU
ESP8266WebServer server(port);

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

void handle_root() {  
  String result = "<!DOCTYPE HTML>";
  result += "<html>";
  result += "<table style=\"width:100%\">";
  result += "<tr><th>Switch Id</th><th>Status</th><th>Switch</th></tr>";
  for (int switchId = 0; switchId < 16; switchId++) {
    boolean switchState = ((relayState >> switchId) & 1);
    result += "<tr>";
    result += "<td>" + String(switchId) + "</td>";
    result += "<td>" + String(switchState == 0 ? "Off" : "On") + "</td>";
    result += "<td><a href=\"/switch?id=" + String(switchId) + "\"><button>Turn "+ String(switchState == 1 ? "Off" : "On") +"</button></a></td>";
    result += "</tr>";
  }
  result += "</table>";
  result += "</html>";
    
  server.send(200, "text/html", result);  
  delay(100);
}

void handle_test() {  
  int pause = 500;  
  for (int switchId = 15; switchId >= 0; switchId--) {
    relayState = 0;
    relayState |= (1 << switchId);    
    switchRelay(relayState); delay(pause);
  }
  handle_root();
}

void handle_switch() {  
  
  int relayNb = server.arg("id").toInt();
  int thisRelayState = (relayState >> relayNb) & 1;
  
  Serial.println("Relay " + String(relayNb) + "=" + String(thisRelayState));
  
  if(thisRelayState == 0)
    relayState |= (1 << relayNb);
  else
    relayState &= ~(1 << relayNb);
  
  Serial.println("Relays = " + String(relayState));
  
  switchRelay(relayState); delay(500);
    
  handle_root();
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // 74HC595
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  // Connect to WiFi network
  Serial.print("\n\nConnecting to "); Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Prepare Web API
  server.on("/", handle_root);  
  server.on("/test", handle_test);
  server.on("/switch", handle_switch);

  // Start the server REST
  server.begin();
  Serial.println("HTTP Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  
  handle_test();
}
 
void loop() {
  server.handleClient();
}
