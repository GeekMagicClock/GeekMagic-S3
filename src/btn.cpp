#include <Button2.h>
#include "my_debug.h"
#include <TFT_eSPI.h>
#include "font/larabiefont-free32.h"

extern TFT_eSPI tft;
#define TOUCH_PIN T9 // Must declare the touch assignment, not the pin.

int threshold = 1500;   // ESP32S3 
bool touchdetected = false; 
byte buttonState = HIGH;// HIGH is for unpressed, pressed = LOW
/////////////////////////////////////////////////////////////////

Button2 button;

/////////////////////////////////////////////////////////////////
void gotTouch() {
  touchdetected = true;
}

byte capStateHandler() {
    return buttonState;
}

int idx = 0;
void click(Button2& btn) {
    int touch_val = touchRead(TOUCH_PIN);
    DBG_PTN("touch_val= " + String(touch_val));
    Serial.println("click\n");
    tft.fillScreen(TFT_BLACK);
    tft.loadFont(larabiefont_free32);
    tft.setTextColor(TFT_GOLD);
    tft.drawString("one Click "+String(idx++), 20, 100);
}

void doubleClick(Button2& btn) {
    int touch_val = touchRead(TOUCH_PIN);
    DBG_PTN("touch_val= " + String(touch_val));
    Serial.println("double click\n");
    tft.fillScreen(TFT_BLACK);
    tft.loadFont(larabiefont_free32);
    tft.setTextColor(TFT_GOLD);
    tft.drawString("double Click "+String(idx++), 0, 100);
}

void longClick(Button2& btn) {
    tft.fillScreen(TFT_BLACK);
    Serial.println("long click\n");
    tft.fillScreen(TFT_BLACK);
    tft.loadFont(larabiefont_free32);
    tft.setTextColor(TFT_GOLD);
    tft.drawString("long Click "+String(idx++), 20, 100);
}

void btn_init(){
    Serial.println("\n\nCapacitive Touch Demo");
    touchAttachInterrupt(TOUCH_PIN, gotTouch, threshold); 
    button.setButtonStateFunction(capStateHandler);
    button.setClickHandler(click);
    button.setDoubleClickHandler(doubleClick);
    button.setLongClickTime(1000);
    button.setLongClickHandler(longClick);
    button.begin(BTN_VIRTUAL_PIN);
}

void btn_update(){
  button.loop();
  if (touchdetected) {
    touchdetected = false;
    if (touchInterruptGetLastStatus(TOUCH_PIN)) {
      buttonState = LOW;
    } else {
      buttonState = HIGH;
    }
  }
}