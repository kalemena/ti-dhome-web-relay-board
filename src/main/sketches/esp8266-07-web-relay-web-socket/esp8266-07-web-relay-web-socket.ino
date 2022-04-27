/* 
  ti-dhome - esp8266 base web relay board
  Copyright (c) 2022 Kalemena. All rights reserved.

  access the sample web page at http://iotrelays.local
  edit the page by going to http://iotrelays.local/edit
*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSockets4WebServer.h>
#include <Hash.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);
WebSockets4WebServer webSocket;

// holds the current upload
File fsUploadFile;
// holds 16 bits
unsigned int relayState = 0;

extern "C" {
  #include "user_interface.h"
}

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

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    // Setup file system
    if (LittleFS.begin()) {
      Serial.println(F("LittleFS system mounted with success"));
    } else {
      Serial.println(F("An Error has occurred while mounting LittleFS"));
    }
    
    // Get all information about LittleFS
    FSInfo fsInfo;
    LittleFS.info(fsInfo);
  
    Serial.println("------------------------------");
    Serial.println("File system info");
    Serial.println("------------------------------");
    Serial.printf("Total space:      %d byte\n", fsInfo.totalBytes);
    Serial.printf("Total space used: %d byte\n", fsInfo.usedBytes);
    Serial.printf("Block size:       %d byte\n", fsInfo.blockSize);
    Serial.printf("Page size:        %d byte\n", fsInfo.totalBytes);
    Serial.printf("Max open files:   %d\n", fsInfo.maxOpenFiles);
    Serial.printf("Max path lenght:  %d\n", fsInfo.maxPathLength);
  
    Serial.println("------------------------------");
    Serial.println("List files");
    Serial.println("------------------------------");
    Dir dirit = LittleFS.openDir("/");
    while (dirit.next()) {
      if (dirit.fileSize()) {
        String fileName = dirit.fileName();
        size_t fileSize = dirit.fileSize();
        Serial.printf(" - %s - %s byte\n", fileName.c_str(), String(fileSize).c_str());
      }
    }
    Serial.printf("\n");
    
    delay(10);

    // 74HC595
    // Set pins to output so you can control the shift register
    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);

    Serial.println("WiFi initializing");
    WiFiMulti.addAP(ssid, password);

    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }

    Serial.println("Controller initializing");
    // list directory
    server.on("/list", HTTP_GET, controller_file_list);
    // file uploads at that location
    server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, controller_file_upload);
    // file create
    server.on("/edit", HTTP_PUT, controller_file_create);
    // file delete
    server.on("/edit", HTTP_DELETE, controller_file_delete);
    // load file editor
    server.on("/edit", HTTP_GET, [](){
      if(!controller_file_read("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
    });

    // static files
    //server.serveStatic("/favicon.ico", LittleFS, "/favicon.ico");
    //server.serveStatic("/on.svg", LittleFS, "/on.svg");
    //server.serveStatic("/off.svg", LittleFS, "/off.svg");
    
    server.on("/", []() {
        // send index.html
        server.send(200, "text/html", "<html><head><meta http-equiv=\"refresh\" content=\"0; URL=/index.html\" /></head></html>");
    });
    server.on("/status", HTTP_GET, controller_status);
    server.on("/test", HTTP_GET, controller_test);
    server.on("/relays/set", HTTP_GET, controller_relay_set);

    //called when the url is not defined here
    //use it to load content from LittleFS
    server.onNotFound([](){
      if(!controller_file_read(server.uri()))
        server.send(404, "text/plain", "FileNotFound");
    });
    
    // web socket
    server.addHook(webSocket.hookForWebserver("/ws", websocket_event));

    server.begin();
    
    Serial.println();
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Connect as http://" + String(host) + ".local or http://" + WiFi.localIP().toString().c_str());
    Serial.print("Open http://"); Serial.print(host); Serial.println(" to see relays status");
    Serial.print("Open http://"); Serial.print(host); Serial.println(".local/edit to see the file browser");

    if (MDNS.begin(host)) {
        Serial.println("MDNS responder started");
    } else {
        Serial.println("MDNS.begin failed");
    }

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);

    delay(10);
    
    // reset to 0
    operation_relay_set_internal(0);
}

void loop() {
    server.handleClient();
    webSocket.loop();
    MDNS.update();
}

// ===== UTILITIES

String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
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

// Utility convert String to Char
char* StringToChar(String s) {
  unsigned int bufSize = s.length() + 1; //String length + null terminator
  char* ret = new char[bufSize];
  s.toCharArray(ret, bufSize);
  return ret;
}

// ===== OPERATIONS

/**
Value is anything between 0 to 65535 representing 16 bits of data I/Os
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

void operation_test() {
  int pause = 500;  
  for (int switchId = 15; switchId >= 0; switchId--) {
    operation_relay_set(switchId, -1); 
    delay(pause);
    operation_relay_set(switchId, -1);
    delay(pause);
  }
}

// ===== CONTROLLER - FILE

bool controller_file_read(String path){
  Serial.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(LittleFS.exists(pathWithGz) || LittleFS.exists(path)){
    if(LittleFS.exists(pathWithGz))
      path += ".gz";
    File file = LittleFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void controller_file_upload(){
  if(server.uri() != "/upload") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = LittleFS.open(filename, "w");
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
      fsUploadFile.close();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

void controller_file_delete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!LittleFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  LittleFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void controller_file_create(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(LittleFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = LittleFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void controller_file_list() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  Serial.println("handleFileList: " + path);
  Dir dir = LittleFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  server.send(200, "text/json", output);
}

// ===== CONTROLER - RELAYS

String render_status(String message) {
  String json = "{\n";
  json += " \"system\": {\n";
  json += "   \"heap\": " + String(ESP.getFreeHeap()) + ",\n";
  json += "   \"boot-version\": " + String(ESP.getBootVersion()) + ",\n";
  json += "   \"cpu-frequency\": " + String(system_get_cpu_freq()) + ",\n";
  json += "   \"sdk\": \"" + String(system_get_sdk_version()) + "\",\n";
  json += "   \"chip-id\": " + String(system_get_chip_id()) + ",\n";
  json += "   \"flash-id\": " + String(spi_flash_get_id()) + ",\n";
  json += "   \"flash-size\": " + String(ESP.getFlashChipRealSize()) + ",\n";
  json += "   \"vcc\": " + String(ESP.getVcc()) + ",\n";
  json += "   \"gpio\": " + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16))) + "\n";
  json += "  },\n";
  json += render_relays_status();  
  if(message != "") {
    json += ",\n \"message\": \"" + message + "\"\n";
  } else {
    json += "\n";
  }
  json += "}\n";
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

void controller_status() {  
  String json = render_status("status");
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

// ===== WEB-SOCKET

void websocket_broadcast(String message) {
  // send to all clients this update
  char* msgChar = StringToChar(message);
  webSocket.broadcastTXT(msgChar, strlen(msgChar));
  Serial.printf("Broadcast message [%s]\r\n", msgChar);
}

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
    }

}
