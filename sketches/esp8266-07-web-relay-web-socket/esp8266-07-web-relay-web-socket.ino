#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSockets4WebServer.h>
#include <Hash.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);
WebSockets4WebServer webSocket;

extern "C" {
  #include "user_interface.h"
}

#define USE_SERIAL Serial

// ===== CONFIGURATION

#include "settings.h"

// ===== WebSocket

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

				        // send message to client
				        webSocket.sendTXT(num, "Connected");
            }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

//            if(payload[0] == '#') {
//                // we get RGB data
//
//                // decode rgb data
//                uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);
//
//                analogWrite(LED_RED, ((rgb >> 16) & 0xFF));
//                analogWrite(LED_GREEN, ((rgb >> 8) & 0xFF));
//                analogWrite(LED_BLUE, ((rgb >> 0) & 0xFF));
//            }

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            USE_SERIAL.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }

}

// ===== SETUP

void setup() {
    USE_SERIAL.begin(115200);

    //Serial.setDebugOutput(true);
    USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println("Initializing WS2812 ...");
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    SPIFFS.begin();
    {
      USE_SERIAL.println("SPIFFS contents:");
  
      Dir dir = SPIFFS.openDir("/");
      while (dir.next()) {
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        USE_SERIAL.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
      }
      USE_SERIAL.printf("\n");
    }

    USE_SERIAL.println("WiFi initializing");
    WiFiMulti.addAP(ssid, password);

    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
        USE_SERIAL.print(".");
    }

    USE_SERIAL.println("Controller initializing");
    server.on("/", []() {
        // send index.html
        server.send(200, "text/html", "<html><head><script>var connection = new WebSocket('ws://'+location.hostname+':80/ws', ['arduino']);connection.onopen = function () {  connection.send('Connect ' + new Date()); }; connection.onerror = function (error) {    console.log('WebSocket Error ', error);};connection.onmessage = function (e) {  console.log('Server: ', e.data);};function sendRGB() {  var r = parseInt(document.getElementById('r').value).toString(16);  var g = parseInt(document.getElementById('g').value).toString(16);  var b = parseInt(document.getElementById('b').value).toString(16);  if(r.length < 2) { r = '0' + r; }   if(g.length < 2) { g = '0' + g; }   if(b.length < 2) { b = '0' + b; }   var rgb = '#'+r+g+b;    console.log('RGB: ' + rgb); connection.send(rgb); }</script></head><body>LED Control:<br/><br/>R: <input id=\"r\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>G: <input id=\"g\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>B: <input id=\"b\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/></body></html>");
    });
    server.on("/status", HTTP_GET, controllerStatus);

    // web socket
    server.addHook(webSocket.hookForWebserver("/ws", webSocketEvent));

    server.begin();
    
    USE_SERIAL.println();
    USE_SERIAL.print("Connected to ");
    USE_SERIAL.println(ssid);
    USE_SERIAL.print("IP address: ");
    USE_SERIAL.println(WiFi.localIP());

    USE_SERIAL.println("Connect as http://" + String(host) + ".local or http://" + WiFi.localIP().toString().c_str());

    if (MDNS.begin(host)) {
        USE_SERIAL.println("MDNS responder started");
    } else {
        USE_SERIAL.println("MDNS.begin failed");
    }

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
}

// ===== CONTROLER

void controllerStatus() {  
  String json = renderStatus("status");
  server.send(200, "application/json", json);
  json = String();
}

// ===== TASK

String renderStatus(String message) {
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
  if(message != "") {
    json += " \"message\": \"" + message + "\"\n";
  }
  json += "}\n";
  return json;
}

void loop() {
    server.handleClient();
    webSocket.loop();
    MDNS.update();
}
