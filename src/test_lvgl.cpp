#include <lvgl.h>
#include <lv_conf.h>
#include "demos/lv_demos.h"
//#include "demos/benchmark/lv_demo_benchmark.h"
#include <TFT_eSPI.h>
TFT_eSPI tft;
/* Change to your screen resolution */
void *draw_buf_1;
unsigned long lastTickMillis = 0;
#define TFT_HOR_RES 240
#define TFT_VER_RES 240
static lv_display_t *disp;
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char *file, uint32_t line, const char *dsc)
{
  Serial.printf("%s@%d->%s\r\n", file, line, dsc);
  Serial.flush();
}
#endif

void setup(){
    lv_init();
    delay(10);
    //tft.begin();        /* TFT init */
    //tft.setRotation(0); /* Landscape orientation */
    analogWrite(TFT_BL, 255 / 1.5); // values go from 0 to 255,

    draw_buf_1 = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf_1, DRAW_BUF_SIZE);

    String LVGL_Arduino = "Hello Arduino!\n";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
    Serial.println(LVGL_Arduino);
    /* Initialize the (dummy) input device driver */
    //******************************* MY CODE *******************************************
    lv_demo_benchmark();
}

void loop(){
      // LVGL Tick Interface
  unsigned int tickPeriod = millis() - lastTickMillis;
  lv_tick_inc(tickPeriod);
  lastTickMillis = millis();
  lv_task_handler();
//  delay(5);
}