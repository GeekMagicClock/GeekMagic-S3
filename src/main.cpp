#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "my_debug.h"
#include "display.h"
#include "jpg.h"
#include "gif.h"
#include <AnimatedGIF.h>

//#define TURBO_MODE

#include <lvgl.h>
#include <lv_conf.h>
#include "media_player/media_player.h"

void *draw_buf_1;
unsigned long lastTickMillis = 0;
#define TFT_HOR_RES 240
#define TFT_VER_RES 240
static lv_display_t *disp;
//#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES * 2 *2)

lv_obj_t *img1 = NULL;
lv_obj_t *label =  NULL;
#include "mjpegclass.h"
static MjpegClass mjpeg;
static uint8_t *mjpeg_buf;
static uint16_t *imageData = nullptr;

#include <TFT_eSPI.h>
extern TFT_eSPI tft;

#define IMG_WIDTH 240
#define IMG_HEIGHT 240
lv_obj_t * canvas = NULL; 
static lv_color_t *canvas_buf = NULL;//[IMG_WIDTH * IMG_HEIGHT];

bool isBreak = false; //是否播放中断
JPEGDEC jpg;
File ff;
static void * openFile(const char *fname, int32_t *pSize) {
  ff = LittleFS.open(fname, "r");
  if (ff) {
    //Serial.printf("open %s\n", fname);
    *pSize = ff.size();
    //Serial.printf("filesize [%d]\n", *pSize);
    if(ff.size() == 0) return NULL;
    //DBG_PTN("open success");
    return (void *)&ff;
  }
  //DBG_PTNf("%s open failed\r\n", fname);
  return NULL;
}
static void closeFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  //  digitalWrite(TFT_CS_PIN,HIGH);
  //  digitalWrite(SD_CS_PIN,LOW);
  if (f != NULL){
     f->close();
  }
  //DBG_PTN("close success");
}
static int32_t readFile(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    if (iBytesRead <= 0){
       //DBG_PTNf("read 0 bytes\n");
       return 0;
    }
    //DBG_PTNf("1file[%s] available size %d\r\n", f->name(), f->available());
  //  digitalWrite(TFT_CS_PIN,HIGH);
  //  digitalWrite(SD_CS_PIN,LOW);
    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();
    //DBG_PTNf("2file[%s] available size %d\r\n", f->name(), f->available());
    //DBG_PTNf("read [%d] success\r\n", iBytesRead);
    return iBytesRead;
} /* GIFReadFile() */

static int32_t seekFile(JPEGFILE *pFile, int32_t iPosition)
{ 
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  //  digitalWrite(TFT_CS_PIN,HIGH);
  //  digitalWrite(SD_CS_PIN,LOW);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
  //DBG_PTNf("Seek time = %d us\n", i);
  return pFile->iPos;
}

void setup(){
    Serial.begin(115200);
    delay(1000);
    if(!LittleFS.begin()){
        DBG_PTN("file system not mounted");
        yield();
    }
    //delay(3000);
    init_screen();

    if(psramInit()){
        Serial.println("PSRAM is correctly initialized");
    }else{
        Serial.println("PSRAM not available");
    }

    lv_init();
    draw_buf_1 = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_SPIRAM);
    disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf_1, DRAW_BUF_SIZE);

    String LVGL_Arduino = "Hello Arduino!\n";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
    Serial.println(LVGL_Arduino);

#if 1
    imageData = (uint16_t *)heap_caps_malloc(tft.width()* tft.height()* sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!imageData) {
        Serial.println("内存分配失败！");
        return ;
    }
#endif
    
    //img_dsc.header = header;
    //img_dsc.data = (const uint8_t *)imageData;
    //img_dsc.data_size = tft.width()* tft.height()* sizeof(uint16_t);
     // 创建画布对象并设置缓冲区
     #if 0
        canvas_buf = (lv_color_t *)malloc(IMG_WIDTH * IMG_HEIGHT * sizeof(lv_color_t));
        memset(canvas_buf,0, sizeof(canvas_buf));
        if (canvas_buf == NULL) {
            Serial.println("Failed to allocate memory for canvas buffer");
            return;
        }
            Serial.println("success to allocate memory for canvas buffer");
    #endif    
#if 1
        canvas = lv_canvas_create(lv_scr_act());
        //lv_canvas_set_buffer(canvas, NULL, IMG_WIDTH, IMG_HEIGHT, LV_COLOR_FORMAT_RGB565);
        // 设置画布对象的位置和大小
        lv_obj_set_pos(canvas, 0, 0);
        lv_obj_set_size(canvas, IMG_WIDTH, IMG_HEIGHT);
        lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

        img1 = lv_img_create(lv_scr_act());
#endif
#if 1
    label = lv_label_create(lv_scr_act());
    lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );
    static lv_style_t style;
    lv_style_init(&style); // 初始化样式
    lv_style_set_text_font(&style, &lv_font_montserrat_48);
    lv_style_set_bg_color(&style, lv_color_make(00,0xFF,00));
    lv_style_set_bg_opa(&style, LV_OPA_0);
    lv_style_set_border_width(&style, 0);
    lv_style_set_border_color(&style,lv_color_make(0,0,0xff));
    lv_style_set_border_opa(&style,LV_OPA_0);
    lv_style_set_text_color(&style, lv_color_make(0xff,0xff,0xff));
    lv_obj_add_style(label, &style, 0);
#endif

    // screenWidth * screenHeight / 2 大概是一帧JPG文件的大小，240x240差不多10-15kb一帧，这个分配了28.8kb可以说是很充足了
    #if 0
    mjpeg_buf = (uint8_t *)heap_caps_malloc(240 * 240, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!mjpeg_buf)
    {
        Serial.println(F("[MJPEG] Draw buff malloc failed!"));
        return ;
    }
    #endif

    //label = lv_label_create(img1);
    //init_jpg();
    //DBG_PTN("draw jpg");
    //draw_jpg("/starry.jpg");
    //drawGif("/space.gif",0,0,true); 
    media_player_init(); 
    DBG_PTN("boot done");
}
#include "gif.h"
File playFile;

// 块的偏移量
int blockOffsetX = 0;
int blockOffsetY = 0;
const int BLOCK_WIDTH = 128; // 根据实际解码块宽度调整
const int BLOCK_HEIGHT = 16; // 根据实际解码块高度调整
int frames = 0;
#define BUFFER_SIZE (IMG_WIDTH * IMG_HEIGHT * sizeof(uint16_t))

int jpegDrawCallback(JPEGDRAW *pDraw)
{
// 计算解码块数据在缓冲区中的位置并复制数据
    for (int y = 0; y < pDraw->iHeight; y++) {
        int srcIndex = y * pDraw->iWidth;
        int dstIndex = (pDraw->y + y) * IMG_WIDTH + pDraw->x;
        memcpy(&imageData[dstIndex], &pDraw->pPixels[srcIndex], pDraw->iWidth * sizeof(uint16_t));
        //memcpy(&canvas_buf[dstIndex], &pDraw->pPixels[srcIndex], pDraw->iWidth * sizeof(lv_color_t));
    }
    //Serial.printf("x,y(%d,%d)\r\n", pDraw->x, pDraw->y);
    if(pDraw->x == BLOCK_WIDTH && pDraw->y == tft.width()-BLOCK_HEIGHT )//最后一个数据块 
    {
        lv_label_set_text_fmt(label, "fx %d", frames++);
        //Serial.printf("draw (%d) frame\r\n", frames++);
        #if 0
        //if(frames == 1) 
        //tft.dmaWait();
        //jpg 正常，mjpeg 解码的时候会出现断言
        //tft.pushImageDMA(0,0,tft.width(),tft.height(),(uint16_t*)imageData);
        //不使用 dma 则mjpeg正常
        tft.pushImage(0,0,tft.width(),tft.height(),imageData);
        #else 
        //if(frames != 2)
        {
        #if 0
        //可以正常显示一张 jpg的代码片段
        lv_image_dsc_t img_dsc;
        img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
        img_dsc.data = (uint8_t*)imageData;
        img_dsc.data_size = 240*240*2;
        img_dsc.header.w = 240;
        img_dsc.header.h = 240;
        img_dsc.header.cf =  LV_COLOR_FORMAT_RGB565;
        img_dsc.header.flags = 0;
        img_dsc.header.stride = 0;
        img_dsc.header.reserved_2 = 0;
        lv_img_set_src(img1, &img_dsc);
        #endif

        //lv_canvas_set_buffer(canvas, imageData, IMG_WIDTH, IMG_HEIGHT, LV_COLOR_FORMAT_RGB565);
        #if 0
        lv_draw_buf_t img_dsc;
        img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
        img_dsc.data = (uint8_t*)imageData;
        img_dsc.data_size = 240*240*2;
        img_dsc.header.w = 240;
        img_dsc.header.h = 240;
        img_dsc.header.cf =  LV_COLOR_FORMAT_RGB565;
        img_dsc.header.flags = 0;
        img_dsc.header.stride = 0;
        img_dsc.header.reserved_2 = 0;
        
        //lv_canvas_set_draw_buf(canvas, &img_dsc);
        #endif
        //draw_buf.unaligned_data = buf;
        //label = lv_label_create(lv_scr_act());
        //lv_label_set_text_fmt(label, "framex %d", frames);
        }
        //lv_task_handler();
        //lv_obj_invalidate(canvas);
        #endif
        // 清除旧的图像数据
        //memset(imageData, 0, IMG_WIDTH * IMG_HEIGHT * sizeof(lv_color_t));
    }
    //if(blockOffsetY>=tft.height()){
    return 1; 
}


void loop(){
    #if 0
    int ret = jpg.open("/starry.jpg", openFile, closeFile, readFile, seekFile, jpegDrawCallback); 
    if(ret == 0){
        Serial.printf("err: %d", jpg.getLastError());
        Serial.println("jpg open failed");
        return;
    }else{
        Serial.println("jpg open ok");
    }
    jpg.setPixelType(RGB565_BIG_ENDIAN);
    jpg.decode(0,0,0);
    jpg.close();
    //ff.close();
    while(1);
    return;
    #else
    //draw_jpg("/starry.jpg");
    media_player_process();
    lv_canvas_set_buffer(canvas, imageData, IMG_WIDTH, IMG_HEIGHT, LV_COLOR_FORMAT_RGB565);
    Serial.println(frames);
    unsigned int tickPeriod = millis() - lastTickMillis;
    lv_tick_inc(tickPeriod);
    lastTickMillis = millis();
    lv_task_handler();
    //delay(1000);
    #endif
    return;

        Serial.println(F("[MJPEG] Opening Video start"));
        playFile = LittleFS.open("/dragon_240x240_20fps.mjpeg", FILE_READ);
        if(!playFile) 
            Serial.println(F("[MJPEG] file open error"));

        mjpeg.setup(
            &playFile, mjpeg_buf, jpegDrawCallback, true /* useBigEndian */,
            0 /* x */, 0 /* y */, tft.width() /* widthLimit */,tft.height() /* heightLimit */);
        Serial.println(F("[MJPEG] mjpeg setup"));
        while (playFile.available() && mjpeg.readMjpegBuf())
        {
            mjpeg.drawJpg();
            Serial.println(F("[MJPEG] drawjpg"));
            if (isBreak)
            {
                Serial.println(F("[MJPEG] Opening Video break;"));
                break;
            }
        }
        Serial.println(F("[MJPEG] Opening Video end"));
        playFile.close();
  
  lv_task_handler();
// 
return;
#if 0
    tft.setTextColor(TFT_WHITE);
    //tft.drawString(String(hour()/10)+String(hour()%10)+":",100,180);
    tft.drawString("15:",100,180);
    //tft.drawString(String(minute()/10)+String(minute()%10),170,180);
    tft.drawString("36",170,180);
    return;
   ret = drawGif("/space.gif",0,0,false); 
   if(ret == 1){
        drawGif("/space.gif",0,0,true); 
   }
#endif
}