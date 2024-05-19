#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "my_debug.h"
#include "display.h"
#include "jpg.h"
#include "gif.h"
#include <AnimatedGIF.h>

#define TURBO_MODE

#include "media_player/media_player.h"
void setup(){
    Serial.begin(115200);
    if(!LittleFS.begin()){
        DBG_PTN("file system not mounted");
        yield();
        return;
    }
    delay(3000);
    init_screen();
    //init_jpg();
    //DBG_PTN("draw jpg");
    //draw_jpg("/starry.jpg");
    //drawGif("/space.gif",0,0,true); 
    media_player_init(); 
    DBG_PTN("boot done");
}
#include "gif.h"
#include <TFT_eSPI.h>
extern TFT_eSPI tft;
void loop(){
    media_player_process();

    tft.setTextColor(TFT_WHITE);
    //tft.drawString(String(hour()/10)+String(hour()%10)+":",100,180);
    tft.drawString("15:",100,180);
    //tft.drawString(String(minute()/10)+String(minute()%10),170,180);
    tft.drawString("36",170,180);
    return;
   int ret = drawGif("/space.gif",0,0,false); 
   if(ret == 1){
        drawGif("/space.gif",0,0,true); 
   }
}