/*
  Adapted from the Adafruit and Xark's PDQ graphicstest sketch.

  See end of file for original header text and MIT license info.
*/

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

/* OPTION 1: Uncomment a dev device in Arduino_GFX_dev_device.h */
#include "Arduino_GFX_dev_device.h"
#ifndef GFX_DEV_DEVICE
/* OPTION 2: Manual define hardware */

/* Step 1: Define pins in Arduino_GFX_databus.h */
//#include "Arduino_GFX_pins.h"

/* Step 2: Uncomment your databus in Arduino_GFX_databus.h */
//#include "Arduino_GFX_databus.h"

/* Step 3: Uncomment your display driver in Arduino_GFX_display.h */
//#include "Arduino_GFX_display.h"

#endif /* Manual define hardware */
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/


/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
#include <Seeed_FS.h>
#include <SD/Seeed_SD.h>
#elif defined(TARGET_RP2040) || defined(PICO_RP2350)
#include <LittleFS.h>
#include <SD.h>
#elif defined(ESP32)
//#include <FFat.h>
#include <LittleFS.h>
//#include <SPIFFS.h>
//#include <SD.h>
//#include <SD_MMC.h>
#elif defined(ESP8266)
#include <LittleFS.h>
#include <SD.h>
#else
#include <SD.h>
#endif

#include <AnimatedGIF.h>
AnimatedGIF gif;
File f;
int16_t display_width, display_height;

void *GIFOpenFile(const char *fname, int32_t *pSize)
{
  /* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
  f = SD.open(fname, "r");
#elif defined(TARGET_RP2040) || defined(PICO_RP2350)
  f = LittleFS.open(fname, "r");
  // f = SD.open(fname, "r");
#elif defined(ESP32)
  // f = FFat.open(fname, "r");
  f = LittleFS.open(fname, "r");
  // f = SPIFFS.open(fname, "r");
  // f = SD.open(fname, "r");
#elif defined(ESP8266)
  f = LittleFS.open(fname, "r");
  // f = SD.open(fname, "r");
#else
  f = SD.open(fname, FILE_READ);
#endif
  if (f)
  {
    *pSize = f.size();
    return (void *)&f;
  }
  return NULL;
} /* GIFOpenFile() */

void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
  {
    f->close();
  }
} /* GIFCloseFile() */

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
  int32_t iBytesRead;
  iBytesRead = iLen;
  File *f = static_cast<File *>(pFile->fHandle);
  // Note: If you read a file all the way to the last byte, seek() stops working
  if ((pFile->iSize - pFile->iPos) < iLen)
  {
    iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
  }
  if (iBytesRead <= 0)
  {
    return 0;
  }
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
} /* GIFReadFile() */

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
  //  Serial.printf("Seek time = %d us\n", i);
  return pFile->iPos;
} /* GIFSeekFile() */
#if 1
void GIFDraw(GIFDRAW *pDraw)
{
  gfx->draw16bitBeRGBBitmap(pDraw->iX, pDraw->iY+pDraw->y, (uint16_t*)pDraw->pPixels, pDraw->iWidth, 1);
  return;
  if (pDraw->y == 0) { // set the memory window when the first line is rendered
    //spilcdSetPosition(&lcd, pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight, DRAW_TO_LCD);
    //gfx->setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight);
    //tft.setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, 1);
  }
  // For all other lines, just push the pixels to the display
    //tft.pushPixels((uint8_t *)pDraw->pPixels, pDraw->iWidth);
    //gfx->writePixels((uint8_t *)pDraw->pPixels, pDraw->iWidth);
} /* GIFDraw() */
#endif

#if 0
// Draw a line of image directly on the LCD
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth;

  static uint16_t *usTemp = NULL;
  
  // Allocate buffer once and reuse
  if (usTemp == NULL) {
    usTemp = (uint16_t*)heap_caps_malloc(pDraw->iWidth * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!usTemp) {
      Serial.println("Failed to allocate PSRAM for GIF buffer");
      return;
    }
  }

  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > display_width)
  {
    iWidth = display_width - pDraw->iX;
  }
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= display_height || pDraw->iX >= display_width || iWidth < 1)
  {
    return;
  }
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
      {
        s[x] = pDraw->ucBackground;
      }
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      }           // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        gfx->draw16bitBeRGBBitmap(pDraw->iX + x, y, usTemp, iCount, 1);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
        {
          iCount++;
        }
        else
        {
          s--;
        }
      }
      if (iCount)
      {
        x += iCount; // skip these
        iCount = 0;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    for (x = 0; x < iWidth; x++)
    {
      usTemp[x] = usPalette[*s++];
    }
    gfx->draw16bitBeRGBBitmap(pDraw->iX, y, usTemp, iWidth, 1);
  }
} /* GIFDraw() */
#endif
//#define GIF_FILENAME "/960x360px.gif"
//#define GIF_FILENAME "/ccc.gif"
#define GIF_FILENAME "/boot.gif"

void setup()
{
#ifdef DEV_DEVICE_INIT
  DEV_DEVICE_INIT();
#endif

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino_GFX Animated GIF Image Viewer example");
  Serial.println(ESP.getFlashChipSize());
    // Check PSRAM
    if (psramFound()) {
      Serial.println("PSRAM found and initialized");
      Serial.printf("Total PSRAM: %d bytes\n", ESP.getPsramSize());
      Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    } else {
      Serial.println("PSRAM not found or not initialized!");
    }
  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(RGB565_BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  display_width = gfx->width();
  display_height = gfx->height();

/* Wio Terminal */
#if defined(ARDUINO_ARCH_SAMD) && defined(SEEED_GROVE_UI_WIRELESS)
  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI, 4000000UL))
#elif defined(TARGET_RP2040) || defined(PICO_RP2350)
  if (!LittleFS.begin())
  // if (!SD.begin(SS))
#elif defined(ESP32)
  // if (!FFat.begin())
  if (!LittleFS.begin())
  //if (!LittleFS.begin(true, "/littlefs", 10, "littlefs"))
  // if (!SPIFFS.begin())
  // SPI.begin(12 /* CLK */, 13 /* D0/MISO */, 11 /* CMD/MOSI */);
  // if (!SD.begin(10 /* CS */, SPI))
  // pinMode(10 /* CS */, OUTPUT);
  // digitalWrite(SD_CS, HIGH);
  // SD_MMC.setPins(12 /* CLK */, 11 /* CMD/MOSI */, 13 /* D0/MISO */);
  // if (!SD_MMC.begin("/root", true /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_DEFAULT))
  // SD_MMC.setPins(12 /* CLK */, 11 /* CMD/MOSI */, 13 /* D0/MISO */, 14 /* D1 */, 15 /* D2 */, 10 /* D3/CS */);
  // if (!SD_MMC.begin("/root", false /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
#elif defined(ESP8266)
  if (!LittleFS.begin())
  // if (!SD.begin(SS))
#else
  if (!SD.begin())
#endif
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
    gfx->println(F("ERROR: File System Mount Failed!"));
  }

  gif.begin(BIG_ENDIAN_PIXELS);
}
// memory allocation callback function
void * GIFAlloc(uint32_t u32Size)
{
  //return heap_caps_malloc(u32Size, MALLOC_CAP_SPIRAM);
  return heap_caps_aligned_alloc(32, u32Size, MALLOC_CAP_SPIRAM);

//  return malloc(u32Size);
} /* GIFAlloc() */
// memory free callback function
void GIFFree(void *p)
{
  free(p);
} /* GIFFree() */

// 在全局区域添加这些变量
unsigned long lastFrameTime = 0;
unsigned long frameCount = 0;
float fps = 0;
void loop()
{
    // 增加帧计数
  
    // 每秒计算一次 FPS
    if (millis() - lastFrameTime >= 5000) {
      fps = frameCount / ((millis() - lastFrameTime) / 1000.0);
      Serial.printf("FPS: %.2f\n", fps);
      
      // 可以选择在屏幕上显示 FPS
      gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
      gfx->setCursor(10, 10);
      gfx->printf("FPS: %.2f", fps);
      
      frameCount = 0;
      lastFrameTime = millis();
      delay(1000);
    }
  //gfx->fillScreen(RGB565_BLACK);
  gfx->fillScreen(RGB565_RED);
    frameCount++;
  gfx->fillScreen(RGB565_GREEN);
    frameCount++;
  gfx->fillScreen(RGB565_BLUE);
    frameCount++;
  return;
  long lTime;
  int iFrames;
  if (gif.open(GIF_FILENAME, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    if (gif.allocFrameBuf(GIFAlloc) == GIF_SUCCESS)
    {
      gif.setDrawType(GIF_DRAW_COOKED); // we want the library to generate ready-made pixels
      lTime = micros();
      iFrames = 0;
      while (gif.playFrame(true, NULL))
      {
        iFrames++;
      }
      lTime = micros() - lTime;
      Serial.printf("total decode time for %d frames = %d us\n", iFrames, (int)lTime);
      gif.freeFrameBuf(GIFFree);
    }
    else
    {
      Serial.println("Insufficient memory!");
    }
    gif.close();
  }
  else
  {
    Serial.printf("Error opening file = %d\n", gif.getLastError());
    while (1)
    {
    };
  }
}