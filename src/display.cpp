
#include <TFT_eSPI.h>

TFT_eSPI tft;
void init_screen();
void set_tft_brt(int brt);
void set_tft_brt(int brt);

#define LCD_BL_PWM_CHANNEL 0

void set_tft_brt(int brt){
    //digitalWrite(TFT_BL, brt);
    ledcSetup(LCD_BL_PWM_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_BL, LCD_BL_PWM_CHANNEL);
    ledcWrite(LCD_BL_PWM_CHANNEL, brt);
}

void init_screen(){
    set_tft_brt(100);
    tft.begin();

    bool ret = tft.initDMA();
    if(ret){
        Serial.println("tft dma init ok");
    }else{
        Serial.println("tft dma init fail");
    }

    tft.fillScreen(TFT_GREEN);
    tft.loadFont("AF40");
    tft.setTextColor(TFT_BLACK, TFT_RED);
    tft.setTextSize(4);
    tft.drawString("GeekMagic", 10,100);

}