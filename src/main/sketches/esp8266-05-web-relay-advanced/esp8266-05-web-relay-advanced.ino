/* 
  ti-dhome - esp8266 base web relay board
  Copyright (c) 2016 Kalemena. All rights reserved.
   
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  upload the contents of the data folder with MkSPIFFS Tool ("ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE)
  or you can upload the contents of a folder if you CD in that folder and run the following command:
  for file in `ls -A1`; do curl -F "file=@$PWD/$file" esp8266fs.local/edit; done
  
  access the sample web page at http://iotrelays.local
  edit the page by going to http://iotrelays.local/edit
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#define DEBUG true
#define Serial if(DEBUG)Serial

const char* ssid = "<ssid>";
const char* password = "<password>";
const char* host = "iotrelays";

// 74HC595
int latchPin = 15;
int clockPin = 12;
int dataPin = 13;

#define ARRAYSIZE 16
String sensors[ARRAYSIZE] = { "Ch. Jeux", "Bureau", "Ch. Léna", "Ch. Damien", "SdB Haut", "Mezanine", "Ch. Bas", "SdB Bas", "NA", "NA", "NA", "NA", "NA", "NA", "Salon", "Hall" };

ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;
// holds 16 bits
unsigned int relayState = 0;

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

//format bytes
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

bool handleFileRead(String path){
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

void handleFileUpload(){
  if(server.uri() != "/upload") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
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

void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate(){
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}
  
  String path = server.arg("dir");
  Serial.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
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

void handle_root() {  
  String result = 
    "<!DOCTYPE HTML> \
     <html>\
      <head>\
       <meta http-equiv=\"Content-type\" content=\"text/html\"; charset=utf-8\">\
       <title>Web relays</title>\
       <link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\
      </head>\
      <table>\
       <tr>\
        <th>Description</th>\
        <th>Status</th>\
       </tr>";
  for (int switchId = 0; switchId < 16; switchId++) {
    boolean switchState = ((relayState >> switchId) & 1);
    result += "<tr>";
    result += "<td>" + String(sensors[switchId]) + " (" + String(switchId) + ")" + "</td>";
    result += "<td><a href=\"/switch?id=" + String(switchId) + "\"><button>" + String(switchState == 0 ? "<img src=\"/off.svg\"" : "<img src=\"/on.svg\"") + " height=\"40\" width=\"32\"/></button></a></td>";
    result += "</tr>";
  }
  result += "</table>";
  result += "</html>";
    
  server.send(200, "text/html", result);  
  delay(100);
}

void handle_test() {
  server.send(200, "text/plain", "");
  
  int pause = 500;  
  for (int switchId = 15; switchId >= 0; switchId--) {
    relayState = 0;
    relayState |= (1 << switchId);    
    switchRelay(relayState); delay(pause);
  }
}

void handle_status() {  
  String json = "{\n";
  json += " \"heap\":" + String(ESP.getFreeHeap()) + ",\n";
  json += " \"relays\":" + String(analogRead(A0)) + "],\n";
  json += " \"gpio\":"+String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16))) + ",\n";
  json += "}";
  server.send(200, "application/json", json);
  json = String();
}

void handle_gpios_status() {  
  String json = "[\n";
  for (int switchId = 0; switchId < 16; switchId++) {
    boolean switchState = ((relayState >> switchId) & 1);
    if(switchId != 0)
      json += ",\n";
    json += " { \"description\": \"" + String(sensors[switchId]) + "\", \"switch\":" + String(switchId) + ", \"value\":" + String(switchState) + " }";    
  }
  json += "\n]";  
  server.send(200, "application/json", json);
  json = String();
}

void handle_switch() {
  if(!server.hasArg("id")) {
    handle_root();
    return;
  }
    
  int relayNb = server.arg("id").toInt();
  int thisRelayState = (relayState >> relayNb) & 1;
  
  Serial.println("Relay " + String(relayNb) + "=" + String(thisRelayState));
  
  if(thisRelayState == 0)
    relayState |= (1 << relayNb);
  else
    relayState &= ~(1 << relayNb);
  
  Serial.println("Relays = " + String(relayState));  
  switchRelay(relayState);
   
  // reload page
  handle_root();
}

void handle_gpios_switch() {
  if(!server.hasArg("id")) {    
    server.send(200, "text/plain", "Bad Parameter");
    return;
  }
  
  int relayNb = server.arg("id").toInt();
  int thisRelayState = (relayState >> relayNb) & 1;
  
  Serial.println("Relay " + String(relayNb) + "=" + String(thisRelayState));
  
  if(thisRelayState == 0)
    relayState |= (1 << relayNb);
  else
    relayState &= ~(1 << relayNb);
  
  Serial.println("Relays = " + String(relayState));  
  switchRelay(relayState);
   
  server.send(200, "text/plain", "");  
}

void setup(void){
  Serial.begin(115200);
  Serial.print("\n");
  Serial.setDebugOutput(DEBUG);
  
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }

  delay(10);

  // 74HC595
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  // Connect to WiFi network
  Serial.printf("\nConnecting to %s\n", ssid);
  if (String(WiFi.SSID()) != String(ssid)) {
    WiFi.begin(ssid, password);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  
  MDNS.begin(host);
  Serial.print("Open http://"); Serial.print(host); Serial.println(" to see relays status");
  Serial.print("Open http://"); Serial.print(host); Serial.println(".local/edit to see the file browser");
      
  // Initialize Controllers
  // list directory
  server.on("/list", HTTP_GET, handleFileList);
  // file uploads at that location
  server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);
  // file create
  server.on("/edit", HTTP_PUT, handleFileCreate);
  // file delete
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  // load file editor
  server.on("/edit", HTTP_GET, [](){
    if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  server.on("/", handle_root);
  server.on("/status", HTTP_GET, handle_status);
  server.on("/test", HTTP_GET, handle_test);
  server.on("/switch", HTTP_GET, handle_switch);
  server.on("/gpios/status", HTTP_GET, handle_gpios_status);
  server.on("/gpios/switch", HTTP_GET, handle_gpios_switch);

  // static files
  server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
  server.serveStatic("/on.svg", SPIFFS, "/on.svg");
  server.serveStatic("/off.svg", SPIFFS, "/off.svg");
  
  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });
 
  server.begin();
  Serial.println("HTTP server started");

  delay(10);
  switchRelay(0);
}
 
void loop(void){
  server.handleClient();
}
