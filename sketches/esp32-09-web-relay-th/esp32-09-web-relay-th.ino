/* 
  ti-dhome - esp32 base web relay board
  Copyright (c) 2021 Kalemena. All rights reserved.

  access the sample web page at http://iotrelays.local
  edit the page by going to http://iotrelays.local/edit
*/

#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// HTU21
#include <Wire.h>
#include "Adafruit_HTU21DF.h"
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
float lastTemperature = 0.0;
float lastHumidity = 0.0;

// Relays
// holds 16 bits
unsigned int relayState = 0;

#define DEBUG true
#define Serial if(DEBUG)Serial

// ===== CONFIGURATION

#include "settings.h"

// ===== SETUP

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  Serial.println();
  Serial.println("Initializing ...");
  Serial.println();

  // ===== File System
  delay(10);
  if(!SPIFFS.begin(false)){
    Serial.println("SPIFFS Mount Failed");
    return;
  } else {
    Serial.println("SPIFFS Mount OK");
  }
  listDir(SPIFFS, "/", 3);

  // ===== HTU21D
  while(!htu.begin()) {
    Serial.println("Couldn't find sensor ...");
    delay(100);
  }
  Serial.println("HTU21D Sensor found !");

  // ===== Relays on 74HC595
  // Set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  // reset to 0
  operation_relay_set_internal(0);

  // ===== WiFi
  delay(10);
  Serial.println("===========");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // ===== Controllers
  server.on("/", []() {
      // send index.html
      server.send(200, "text/html", "<html><head><meta http-equiv=\"refresh\" content=\"0; URL=/index.html\" /></head></html>");
  });
  server.onNotFound(controller_handleNotFound); 

  webSocket.begin();
  webSocket.onEvent(websocket_event);
  server.begin();
}

int value = 0;

void loop(){
  server.handleClient();
  webSocket.loop();
//  
//  delay(5000);
//  operation_read_TH();
//  
// WiFiClient client = server.available();   // listen for incoming clients
//
//  if (client) {                             // if you get a client,
//    Serial.println("New Client.");           // print a message out the serial port
//    String currentLine = "";                // make a String to hold incoming data from the client
//    while (client.connected()) {            // loop while the client's connected
//      if (client.available()) {             // if there's bytes to read from the client,
//        char c = client.read();             // read a byte, then
//        Serial.write(c);                    // print it out the serial monitor
//        if (c == '\n') {                    // if the byte is a newline character
//
//          // if the current line is blank, you got two newline characters in a row.
//          // that's the end of the client HTTP request, so send a response:
//          if (currentLine.length() == 0) {
//            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
//            // and a content-type so the client knows what's coming, then a blank line:
//            client.println("HTTP/1.1 200 OK");
//            client.println("Content-type:text/html");
//            client.println();
//
//            // the content of the HTTP response follows the header:
//            client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 5 on.<br>");
//            client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 5 off.<br>");
//
//            // The HTTP response ends with another blank line:
//            client.println();
//            // break out of the while loop:
//            break;
//          } else {    // if you got a newline, then clear currentLine:
//            currentLine = "";
//          }
//        } else if (c != '\r') {  // if you got anything else but a carriage return character,
//          currentLine += c;      // add it to the end of the currentLine
//        }
//
//        // Check to see if the client request was "GET /H" or "GET /L":
//        if (currentLine.endsWith("GET /H")) {
//          digitalWrite(5, HIGH);               // GET /H turns the LED on
//        }
//        if (currentLine.endsWith("GET /L")) {
//          digitalWrite(5, LOW);                // GET /L turns the LED off
//        }
//      }
//    }
//    // close the connection:
//    client.stop();
//    Serial.println("Client Disconnected.");
//  }
}

// ===== TOOLS

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
  const uint8_t* src = (const uint8_t*) mem;
  Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
  for(uint32_t i = 0; i < len; i++) {
    if(i % cols == 0) {
      Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    Serial.printf("%02X ", *src);
    src++;
  }
  Serial.printf("\n");
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

// ===== OPERATIONS

void operation_read_TH() {
    lastTemperature = round(htu.readTemperature()*100)/100.0;
    lastHumidity = round(htu.readHumidity()*100)/100.0;
    Serial.printf("Temp=%.2f C° / Humidity=%.2f \%\n", lastTemperature, lastHumidity);

    // webSocket.broadcastTXT(String("sensors~{ \"t\": ") + String(lastTemperature) + String(", \"h\":") + String(lastHumidity) + String(" }"));
}

/**
 * Value is anything between 0 to 65535 representing 16 bits of data I/Os
 */
void operation_relay_set_internal(int value) 
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

/**
 * Set Relay state to specified value 0, 1 or -1 for switch
 */
int operation_relay_set(int relayNb, int relayStateWanted) {
  // int relayStateWanted = -1;
  int thisRelayState = (relayState >> relayNb) & 1;
  int relayStateResponse = thisRelayState;
  
  Serial.println("Relay " + String(relayNb) + "(" + relayStateWanted + ") => " + String(thisRelayState));

  // switch on
  if(thisRelayState == 0 && relayStateWanted != 0) {
    relayState |= (1 << relayNb);
    relayStateResponse = 1;
  }

  // switch off
  if(thisRelayState == 1 && relayStateWanted != 1) {
    relayState &= ~(1 << relayNb);
    relayStateResponse = 0;
  }
  
  Serial.println("Relays = " + String(relayState));  
  operation_relay_set_internal(relayState);

  // webSocket.broadcastTXT(String("relay~{ \"id\": ") + relayNb + String(", \"value\":") + String(relayStateResponse) + String(" }"));

  return relayStateResponse;
}

/**
 * Tests all the relays, one by one with on/off
 */
void operation_test() {
  int pause = 500;  
  for (int switchId = 15; switchId >= 0; switchId--) {
    operation_relay_set(switchId, -1); 
    delay(pause);
    operation_relay_set(switchId, -1);
    delay(pause);
  }
}

// ===== CONTROLLERS

String render_status(String message) {
  String json = "{\n";
  json += " \"system\": {\n";
  json += "   \"heap\": " + String(ESP.getFreeHeap()) + ",\n";
  // json += "   \"boot-version\": " + String(ESP.getBootVersion()) + ",\n";
  // json += "   \"cpu-frequency\": " + String(system_get_cpu_freq()) + ",\n";
  json += "   \"sdk\": \"" + String(system_get_sdk_version()) + "\"\n";
  // json += "   \"chip-id\": " + String(system_get_chip_id()) + ",\n";
  // json += "   \"flash-id\": " + String(spi_flash_get_id()) + ",\n";
  // json += "   \"flash-size\": " + String(ESP.getFlashChipRealSize()) + ",\n";
  // json += "   \"vcc\": " + String(ESP.getVcc()) + ",\n";
  // json += "   \"gpio\": " + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16))) + "\n";
  json += "  },\n";
  json += render_TH();
  json += ",\n";
  json += render_relays_status();
  if(message != "") {
    json += ",\n \"message\": \"" + message + "\"\n";
  } else {
    json += "\n";
  }
  json += "}\n";
  return json;
}

String render_TH() {
  operation_read_TH();
  
  String json = " \"sensors\": {\n";
  json += "   \"temperature\": " + String(lastTemperature) + ",\n";
  json += "   \"humidity\": " + String(lastHumidity) + "\n";
  json += "  }";
  return json;
}

String render_relays_status_body() {
  String json = "{\n";
  json += render_relays_status() + "\n";
  json += "}\n";
  return json;
}

String render_relays_status() {
  String json = " \"relays\": [\n";
  for (int switchId = 0; switchId < 16; switchId++) {
    int thisRelayState = (relayState >> switchId) & 1;
    json += "    { \"description\": \"" + String(sensors[switchId]) + "\", \"id\": " + String(switchId) + ", \"value\":" + String(thisRelayState) + " }";
    if(switchId < 15)
      json += ",";
    json += "\n";
  }
  json += "  ]";
  return json;
}

// ===== WEBSOCKET

void websocket_event(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

                // send message to client
                // webSocket.sendTXT(num, "Connected");
                String json = render_status("");
                webSocket.sendTXT(num, String("status~") + json);
                json = String();
            }
            break;
        case WStype_TEXT:
            {
                Serial.printf("[%u] get Text: %s\n", num, payload);

                String payloadStr = String((const char *)payload);
                if(payloadStr.startsWith("/status")) {
                  String json = render_status("");
                  webSocket.sendTXT(num, String("status~") + json);
                  json = String();
                  
                } else if(payloadStr.startsWith("relays/set?")) {
                  int idxEqual = payloadStr.indexOf("=");
                  if(idxEqual > 0 && idxEqual < payloadStr.length()) {
                    String switchId = payloadStr.substring(11, idxEqual);
                    String switchValue = payloadStr.substring(idxEqual+1);
                    int relayState = operation_relay_set(switchId.toInt(),switchValue.toInt());
                  }
                  
                } else if(payloadStr.startsWith("relays/test")) {
                  operation_test();
                  
                } else if(payloadStr.startsWith("sensors")) {
                  operation_read_TH();
                  
                } else {
                  Serial.println("Unknown command");
                }
            }
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    case WStype_ERROR:      
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
    }
}

// ===== CONTROLLERS

void controller_handleNotFound() {
  // if(!controller_file_read(server.uri()))
    server.send(404, "text/plain","404: Not found");
}
