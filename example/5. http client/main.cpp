#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "my_debug.h"
#include "display.h"
#include "jpg.h"
#include "jpg_download.h"

String ssid = "1024";
String pwd = "your wifi pwd";

void setup(){
    Serial.begin(115200);
    init_screen();
    if(!LittleFS.begin()){
        DBG_PTN("file system not mounted");
        yield();
        return;
    }

    WiFi.begin(ssid, pwd);
    long start = millis();
    while(WiFi.status() != WL_CONNECTED) {
        DBG_PTN("...");
        if(millis()-start > 10000) break;
    }

    if(WiFi.status() != WL_CONNECTED)
        DBG_PTN("wifi failed.");
    else
        DBG_PTN("wifi connected.");

    DBG_PTN(WiFi.localIP());

    init_jpg();
}

unsigned long last_download_time = 0;
bool jpg_changed = true;

#define DOWNLOAD_INTERVAL 20*1000 //ms
void loop(){

    if ((millis() - last_download_time > DOWNLOAD_INTERVAL)) {
       last_download_time = millis();
       jpg_changed = true;
       download_jpg(); 
       DBG_PTN("download finish");
    }

    if(jpg_changed){
       jpg_changed = false;
       DBG_PTN("show jpg");
       draw_jpg("/demo.jpg");
    }
    
}