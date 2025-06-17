//
// Big memory demo
//
// Demonstrates how to get better draw performance and simpler code when your
// MCU has enough RAM to hold the entire GIF image (canvas)
//
// The 'pattern' GIF used in this demo has a lot of small runs of transparent pixels
// This creates a performance problem when drawing on a SPI LCD since the
// LCD must be told to 'skip' those pixels when drawing each line. If your MCU has
// enough RAM (additional 16K in this case) to hold the entire GIF image in memory
// then the transparent pixels can be managed in memory and the draw function can
// just write continuous runs of pixels to the display. For this particular image
// running on my Nano33 BLE test rig, the 'big memory' way of drawing it runs 3x faster
// than the 'small memory' way.
//
// The pin numbers referenced below are for my custom Nano33 BLE test rig
//
#include <LittleFS.h>
#include <AnimatedGIF.h>
#include <TFT_eSPI.h>
#include "display.h"
#include "space.h"

#include <WiFi.h>

extern TFT_eSPI tft;

AnimatedGIF gif;

File f;
static void * GIFOpenFile(const char *fname, int32_t *pSize)
{
  //f = SPIFFS.open(fname, "r");
  f = LittleFS.open(fname, "r");
  //f = SD.open(fname, "r");
  //  digitalWrite(TFT_CS_PIN,HIGH);
  //  digitalWrite(SD_CS_PIN,LOW);
  //f = SD.open(fname);
  if (f) {
    Serial.printf("gif open %s\n", fname);
    *pSize = f.size();
    //DBG_PTNf("filesize [%d]\n", *pSize);
    if(f.size() == 0) return NULL;
    //DBG_PTN("open success");
    return (void *)&f;
  }
  //DBG_PTNf("%s open failed\r\n", fname);
  return NULL;
} /* GIFOpenFile() */

static void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  //  digitalWrite(TFT_CS_PIN,HIGH);
  //  digitalWrite(SD_CS_PIN,LOW);
  if (f != NULL){
     f->close();
  }
  //DBG_PTN("close success");
} /* GIFCloseFile() */

static int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
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

static int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
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
} /* GIFSeekFile() */


//
// Draw callback from GIF decoder
//
// called once for each line of the current frame
// MCUs with very little RAM would have to test for disposal methods, transparent pixels
// and translate the 8-bit pixels through the palette to generate the final output.
// The code for MCUs with enough RAM is much simpler because the AnimatedGIF library can
// generate "cooked" pixels that are ready to send to the display
//
#if 0
void GIFDraw(GIFDRAW *pDraw)
{
  if (pDraw->y == 0) { // set the memory window when the first line is rendered
    //spilcdSetPosition(&lcd, pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight, DRAW_TO_LCD);
    tft.setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight);
    //tft.setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, 1);
  }
  // For all other lines, just push the pixels to the display
    tft.pushPixels((uint8_t *)pDraw->pPixels, pDraw->iWidth);
} /* GIFDraw() */
#endif

#define DISPLAY_WIDTH  tft.width()
#define DISPLAY_HEIGHT tft.height()
#define BUFFER_SIZE 240           // Optimum is >= GIF width or integral division of width
#if 1

bool     dmaBuf = 0;
uint16_t usTemp[1][BUFFER_SIZE];    // Global to support DMA use
// Draw a line of image directly on the LCD
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth, iCount;

  // Display bounds check and cropping
  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > DISPLAY_WIDTH)
    iWidth = DISPLAY_WIDTH - pDraw->iX;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || iWidth < 1)
    return;

  // Old image disposal
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = &usTemp[0][0];
      while (c != ucTransparent && s < pEnd && iCount < BUFFER_SIZE )
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
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        // DMA would degrtade performance here due to short line segments
        tft.setAddrWindow(pDraw->iX + x, y, iCount, 1);
        tft.pushPixels(usTemp, iCount);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          x++;
        else
          s--;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;

    // Unroll the first pass to boost DMA performance
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    if (iWidth <= BUFFER_SIZE)
      for (iCount = 0; iCount < iWidth; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];
    else
      for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA // 71.6 fps (ST7796 84.5 fps)
    tft.dmaWait();
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
    dmaBuf = !dmaBuf;
#else // 57.0 fps
    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixels(&usTemp[0][0], iCount);
#endif

    iWidth -= iCount;
    // Loop if pixel buffer smaller than width
    while (iWidth > 0)
    {
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA
      tft.dmaWait();
      tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
      dmaBuf = !dmaBuf;
#else
      tft.pushPixels(&usTemp[0][0], iCount);
#endif
      iWidth -= iCount;
    }
  }
} /* GIFDraw() */
#endif

//
// The memory management functions are needed to keep operating system
// dependencies out of the core library code
//
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

void setup() {
  Serial.begin(115200);
//  while (!Serial);

#ifndef HAL_ESP32_HAL_H_
  spilcdSetTXBuffer(ucTXBuf, sizeof(ucTXBuf));
#endif

    if(!LittleFS.begin()){
        Serial.println("file system not mounted");
        yield();
    }
 
    init_screen();

    if(psramInit()){
        Serial.println("PSRAM is correctly initialized");
    }else{
        Serial.println("PSRAM not available");
    }
    gif.begin(BIG_ENDIAN_PIXELS);
} /* setup() */

void loop() {
  long lTime;
  int iFrames;
  //if (gif.open((uint8_t *)space, sizeof(space), GIFDraw))

  //if (gif.open("/pia.gif", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  if (gif.open("/ez.gif", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  //if (gif.open("/boot.gif", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    // Allocate an internal buffer to hold the full canvas size
    // In this case, 128x128 = 16k of RAM
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
} /* loop() */