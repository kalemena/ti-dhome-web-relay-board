/* 
  ti-dhome - esp32 base web relay board
  Copyright (c) 2021 Kalemena. All rights reserved.

  access the sample web page at http://iotrelays.local
  edit the page by going to http://iotrelays.local/edit
*/

#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

#include "time.h"

#include <ArduinoJson.h>

#include <ESPmDNS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// System
String chipId;
esp_chip_info_t chip_info;
unsigned long epochTime; 

// HTU21
#include <Wire.h>
#include "Adafruit_HTU21DF.h"
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
float lastTemperature = 0.0;
float lastHumidity = 0.0;
boolean isHTU21Found = false;

// Relays
// holds 16 bits
unsigned int relayState = 0;

// Teleinfo
#include <LibTeleinfo.h>
HardwareSerial TeleInfo(2);  // Teleinfo Serial UART1/Serial2 pins 16,17
TInfo          tinfo;        // Teleinfo object

#define DEBUG false
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

  delay(100);

  // ===== System
  chipId = String((uint32_t)ESP.getEfuseMac(), HEX);
  chipId.toUpperCase();
  esp_chip_info(&chip_info);

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  delay(100);

  // ===== File System
  if(!SPIFFS.begin(false)){
    Serial.println("SPIFFS Mount Failed");
    return;
  } else {
    Serial.println("SPIFFS Mount OK");
  }
  listDir(SPIFFS, "/", 3);

  delay(100);

  // ===== Teleinfo
  //TeleInfo.begin(1200);               // standard pins 16,17 
  TeleInfo.begin(1200, SERIAL_8N1, 18); // custom pins 18,17
  tinfo.init();
  tinfo.attachADPS(ADPSCallback);
  tinfo.attachData(DataCallback);
  tinfo.attachNewFrame(NewFrame);
  tinfo.attachUpdatedFrame(UpdatedFrame);

  delay(100);

  // ===== HTU21D
  int i = 0;
  while(i < 3) {
    i++;
    if(!isHTU21Found) {
      if(!htu.begin()) {
        Serial.println("Couldn't find sensor ...");
        delay(100);
      } else {
        isHTU21Found = true;
        Serial.println("HTU21D Sensor found !");
      }
    }
  }

  delay(100);

  // ===== Relays on 74HC595
  // Set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  // reset to 0
  operation_relay_set_internal(0);

  delay(100);

  // ===== WiFi
  Serial.println("===========");
  Serial.printf("Connecting to %s\n", ssid);
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
  server.on("/status", HTTP_GET, controller_status);
  server.on("/test", HTTP_GET, controller_test);
  server.on("/relays/set", HTTP_GET, controller_relay_set);
  server.onNotFound(controller_handleNotFound); 

  // ===== mDNS
  if(!MDNS.begin(host)) {
     Serial.println("Error starting mDNS");
     return;
  }
  // Add service to mDNS
  MDNS.addService("http", "tcp", 80);

  // ===== Listen
  webSocket.begin();
  webSocket.onEvent(websocket_event);
  server.begin();
}

void loop(){
  // Teleinfo
  static char c;
  if ( TeleInfo.available() ) {
    c = TeleInfo.read();
    tinfo.process(c);
  }
  
  server.handleClient();
  webSocket.loop(); 
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

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  else if(filename.endsWith(".svg")) return "image/svg+xml";
  return "text/plain";
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

String renderLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }

  char timeStringBuff[50];
  strftime (timeStringBuff, 50,"%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  String asString(timeStringBuff);
  return asString;
}

// ===== OPERATIONS

/**
 * Value is anything between 0 to 65535 representing 16 bits of data I/Os
 */
void operation_relay_set_internal(int value) {
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

  webSocket.broadcastTXT(String("relay~{ \"id\": ") + relayNb + String(", \"value\":") + String(relayStateResponse) + String(" }"));

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

/**
 * Reset all the relays to zero
 */
void operation_reset() {
  int pause = 500;  
  for (int switchId = 15; switchId >= 0; switchId--) {
    operation_relay_set(switchId, 0); 
    delay(pause);
  }
}

void operation_th() {
  lastTemperature = round(htu.readTemperature()*100)/100.0;
  lastHumidity = round(htu.readHumidity()*100)/100.0;
  Serial.printf("Temp=%.2f C° / Humidity=%.2f \%\n", lastTemperature, lastHumidity);
}

// ===== JSON

void json_system(JsonObject system) {
  system["chip-model"] = ESP.getChipModel();
  system["chip-revision"] = ESP.getChipRevision();
  system["chip-cores"] = ESP.getChipCores();
  String chipBL = String((chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "") + String((chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
  system["chip-bluetooth"] = chipBL;
  system["chip-id"] = chipId;
  system["heap"] = ESP.getFreeHeap();
  system["flash-size"] = spi_flash_get_chip_size()/(1024*1024);
  system["flash-type"] = String((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embeded" : "external");
  system["time-iso"] = renderLocalTime();
}

void json_sensors(JsonObject sensors) {
  operation_th();
  sensors["temperature"] = lastTemperature;
  sensors["humidity"] = lastHumidity;
}

void json_relay(JsonObject relay, int switchId) {
  int thisRelayState = (relayState >> switchId) & 1;
  
  relay["description"] = sensors[switchId];
  relay["id"] = switchId;
  relay["value"] = thisRelayState;
}

void json_relays(JsonArray relays) {
  for (int switchId = 0; switchId < 16; switchId++) {
    JsonObject relay = relays.createNestedObject();
    json_relay(relay, switchId);
  }
}

String json_status(String message) {

  StaticJsonDocument<2048> doc;

  JsonObject system = doc.createNestedObject("system");
  json_system(system);
  
  if(isHTU21Found) {
    JsonObject sensors = doc.createNestedObject("sensors");
    json_sensors(sensors);
  }

  JsonArray relays = doc.createNestedArray("relays");
  json_relays(relays);

  JsonObject teleinfo = doc.createNestedObject("teleinfo");
  json_teleinfo(teleinfo);

  if(message != "") {
    doc["message"] = message;
  }
  
  String json;
  serializeJson(doc, json);
  return json;
}

// ===== CONTROLLERS

void controller_handleNotFound() {
  if(!controller_file_read(server.uri()))
    server.send(404, "text/plain","404: Not found");
}

bool controller_file_read(String path){
  Serial.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void controller_status() {
  String json = json_status("status");
  server.send(200, "application/json", json);
  json = String();
}

void controller_test() {
  server.send(200, "text/plain", "");
  operation_test();
}

void controller_relay_set() {
  if(!server.hasArg("id")) {
    return;
  }

  int relayWantedState = -1;
  if(server.hasArg("state")) {
    relayWantedState = server.arg("state").toInt();
  }
    
  int relayNb = server.arg("id").toInt();
  int relayState = operation_relay_set(relayNb, relayWantedState);

  String json = "{\n";
  json += "  \"id\": " + String(relayNb) + ", \"value\": " + String(relayState) + "\n";
  json += "}";
  server.send(200, "text/plain", json);
}

// ===== TELEINFO

 /* ======================================================================
Function: printUptime 
Purpose : print pseudo uptime value
Input   : -
Output  : - 
Comments: compteur de secondes basique sans controle de dépassement
          En plus SoftwareSerial rend le compteur de millis() totalement
          A la rue, donc la precision de ce compteur de seconde n'est
          pas fiable du tout, dont acte !!!
====================================================================== */
void printUptime(void) {
  Serial.print(millis()/1000);
  Serial.print(F("s\t"));
}

/* ======================================================================
Function: ADPSCallback 
Purpose : called by library when we detected a ADPS on any phased
Input   : phase number 
            0 for ADPS (monophase)
            1 for ADIR1 triphase
            2 for ADIR2 triphase
            3 for ADIR3 triphase
Output  : - 
Comments: should have been initialised in the main sketch with a
          tinfo.attachADPSCallback(ADPSCallback())
====================================================================== */
void ADPSCallback(uint8_t phase) {
  printUptime();

  // Monophasé
  if (phase == 0 ) {
    Serial.println(F("ADPS"));
  }
  else {
    Serial.print(F("ADPS PHASE #"));
    Serial.println('0' + phase);
  }
}

/* ======================================================================
Function: DataCallback 
Purpose : callback when we detected new or modified data received
Input   : linked list pointer on the concerned data
          current flags value
Output  : - 
Comments: -
====================================================================== */
void DataCallback(ValueList * me, uint8_t  flags) {
  // Show our not accurate second counter
  printUptime();

  if (flags & TINFO_FLAGS_ADDED) 
    Serial.print(F("NEW -> "));

  if (flags & TINFO_FLAGS_UPDATED)
    Serial.print(F("MAJ -> "));

  // Display values
  Serial.print(me->name);
  Serial.print("=");
  Serial.println(me->value);

  webSocket.broadcastTXT(String("sensors/teleinfo~{ \"") + String(me->name) + String("\": \"") + String(me->value) + String("\" }"));
}

/* ======================================================================
Function: NewFrame 
Purpose : callback when we received a complete teleinfo frame
Input   : linked list pointer on the concerned data
Output  : - 
Comments: -
====================================================================== */
void NewFrame(ValueList * me) {
  // Show our not accurate second counter
  printUptime();
  Serial.println(F("FRAME -> SAME AS PREVIOUS"));
}

/* ======================================================================
Function: NewFrame 
Purpose : callback when we received a complete teleinfo frame
Input   : linked list pointer on the concerned data
Output  : - 
Comments: it's called only if one data in the frame is different than
          the previous frame
====================================================================== */
void UpdatedFrame(ValueList * me) {
  // Show our not accurate second counter
  printUptime();
  Serial.println(F("FRAME -> UPDATED"));
}

/* ======================================================================
Function: render_teleinfo 
Purpose : dump teleinfo values as JSON
Output  : JSON
Comments: -
====================================================================== */
void json_teleinfo(JsonObject teleinfo) {
  ValueList * me = tinfo.getList();

  if (me) {
    // Loop thru the node
    while (me->next) {
      // go to next node
      me = me->next;

      // we have at least something ?
      if (me->value && strlen(me->value)) {
        boolean isNumber = true;
        uint8_t c;
        char * p = me->value;

        // check if value is number
        while (*p && isNumber) {
          if ( *p < '0' || *p > '9' )
            isNumber = false;
          p++;
        }

        // this will add "" on not number values
        if (!isNumber) {
          teleinfo[me->name] = me->value;
        }
        // this will remove leading zero on numbers
        else
          teleinfo[me->name] = atol(me->value);
      }
    }
  }
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
                String json = json_status("");
                webSocket.sendTXT(num, String("status~") + json);
                json = String();
            }
            break;
        case WStype_TEXT:
            {
                Serial.printf("[%u] get Text: %s\n", num, payload);

                String payloadStr = String((const char *)payload);
                if(payloadStr.startsWith("status")) {
                  String json = json_status("");
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

                } else if(payloadStr.startsWith("relays/reset")) {
                  operation_reset();

                } else if(payloadStr.startsWith("system")) {
                  epochTime = getTime();
                  char buf[32];
                  sprintf(buf, "%d", epochTime);
                  Serial.println("Epoch Time: " + String(buf));
                                    
                  // String timeStr = renderLocalTime();
                  // Serial.println("Time=" + timeStr + "\"");
                  webSocket.sendTXT(num, String("system~{ \"time\": ") + String(buf) + String(" }"));
                 
                } else if(isHTU21Found && payloadStr.startsWith("sensors/th")) {
                  operation_th();
                  webSocket.sendTXT(num, String("sensors/th~{ \"t\": ") + String(lastTemperature) + String(", \"h\":") + String(lastHumidity) + String(" }"));
                  
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
