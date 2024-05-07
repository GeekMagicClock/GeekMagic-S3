#include <Arduino.h>
#include <LittleFS.h>
#include "my_debug.h"

#include "display.h"
#include "WebServer.h"

WebServer server(80);

void handleRoot(){
    server.send(200, "text/plain", "Hello GeekMagic");
}

/////////////////////////////////////////////////////////////////
void setup(){
    Serial.begin(115200);
    DBG_PTN("Hello GeekMagic S3");
    init_screen();     

    WiFi.mode(WIFI_AP);
    WiFi.softAP("GeekMagic","");

    server.on("/", handleRoot);
    server.begin();
}

void loop(){
    server.handleClient();
}