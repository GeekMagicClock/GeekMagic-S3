//#include <SD.h>
#include <LittleFS.h>
#include <AnimatedGIF.h>
#include <TFT_eSPI.h> 
#include "my_debug.h"

AnimatedGIF *gif = NULL;
extern TFT_eSPI tft;

// GIFDraw is called by AnimatedGIF library frame to screen
#define DISPLAY_WIDTH  tft.width()
#define DISPLAY_HEIGHT tft.height()
#define BUFFER_SIZE 240           // Optimum is >= GIF width or integral division of width

#define USE_DMA

#ifdef USE_DMA
  uint16_t usTemp[2][BUFFER_SIZE]; // Global to support DMA use
#else
  uint16_t usTemp[1][BUFFER_SIZE];    // Global to support DMA use
#endif
bool     dmaBuf = 0;

 
/* gif 图象的显示位置，只能显示一张 */
int x_offset, y_offset;
/* gif高度小于canvas高度时，用户canvas居中显示 */
int height_offset = 0;
int max_frame_height;

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

//TFT_eSprite spt = TFT_eSprite(&tft);
#if 0
// Draw a line of image directly on the LCD
static void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth, iCount;
  if(pDraw->iHeight > max_frame_height)
    max_frame_height = pDraw->iHeight;
  height_offset = (DISPLAY_WIDTH-max_frame_height)/2;
  // Displ;ay bounds chech and cropping
  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > DISPLAY_WIDTH)
    iWidth = DISPLAY_WIDTH - pDraw->iX;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || iWidth < 1){
    return;
  }

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
#if 0
        spt.setColorDepth(8);
        spt.createSprite(90, 1);//创建窗口
        spt.fillSprite(0x0000);   //填充率
        spt.setAddrWindow(pDraw->iX + x, 0, iCount, 1);
        spt.pushPixels(usTemp, iCount); 
        spt.pushSprite(160,160+y);
        spt.deleteSprite();
#else
//这里不能使用DMA，不知原因
        //tft.startWrite(); // The TFT chip slect is locked low
        tft.setAddrWindow(pDraw->iX + x+x_offset, y+y_offset, iCount, 1);
        tft.pushPixels(usTemp, iCount);
        //tft.endWrite(); // The TFT chip slect is released low
#endif
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
    tft.setAddrWindow(pDraw->iX + x_offset, y+y_offset, iWidth, 1);
    tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
    dmaBuf = !dmaBuf;
#else // 57.0 fps
#if 0
        spt.setColorDepth(8);
        spt.createSprite(80, 1);//创建窗口
        spt.fillSprite(0x0000);   //填充率
        spt.setAddrWindow(pDraw->iX + x, 0, iCount, 1);
        spt.pushPixels(&usTemp[0][0], iCount); 
        spt.pushSprite(160,160+y);
        spt.deleteSprite();
#else
    tft.startWrite(); // The TFT chip slect is locked low
    tft.setAddrWindow(pDraw->iX+x_offset, y+y_offset, iWidth, 1);
    tft.pushPixels(&usTemp[0][0], iCount);
    tft.endWrite(); // The TFT chip slect is released low
#endif    
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
#if 0
        spt.setColorDepth(8);
        spt.createSprite(80, 1);//创建窗口
        spt.fillSprite(0x0000);   //填充率
        spt.setAddrWindow(pDraw->iX + x, 0, iCount, 1);
        spt.pushPixels(&usTemp[0][0], iCount); 
        spt.pushSprite(160,160+y);
        spt.deleteSprite();
#else
      tft.startWrite(); // The TFT chip slect is locked low
      tft.pushPixels(&usTemp[0][0], iCount);
      tft.endWrite(); // The TFT chip slect is released low
#endif      
#endif
      iWidth -= iCount;
    }
  }
} /* GIFDraw() */
#endif

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
    //DBG_PTNf("gif open %s\n", fname);
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

static int init_gif = 0;
// return 0; playing 
// return 1; play done
static int ShowGIF(const char *name){
  long lTime = micros();
  int iFrames = 0;

  //未初始化gif，停止播放
  if (init_gif == 0){
      if(gif != NULL){gif->close(); delete gif; gif = NULL; }
      return 1;
  }
  //DBG_PTNf("debug gif draw2\r\n");

  //DBG_PTNf("Start show gif  [%s]\r\n",name); 
  {
    //DBG_PTNf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    tft.startWrite(); // The TFT chip slect is locked low
    //DBG_PTNf(" tft size = %d x %d\n",tft.width(), tft.height());
    if(gif->playFrame(true, NULL))
    {
      iFrames++;
      //yield();
      //Serial.printf(" frames [%d]\n",iFrames);

      //tft.loadFont("AF40");
      tft.setTextColor(TFT_WHITE);
      //tft.drawString(String(hour()/10)+String(hour()%10)+":",100,180);
      tft.drawString("15:",100,180);
      //tft.drawString(String(minute()/10)+String(minute()%10),170,180);
      tft.drawString("36",170,180);
      //tft.unloadFont();

    }else //no more frames
    {
        
        if(gif != NULL){gif->close(); delete gif; gif = NULL; }
        // open next gif file;
        init_gif = 0;
        return 1;
        // reopen to play 
        //gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw);
    }
      
    //tft.endWrite(); // Release TFT chip select for other SPI devices
    lTime = micros() - lTime;
    //DBG_PTN(iFrames / (lTime / 1000000.0));
    //DBG_PTN(" fps");
  }
  return 0;
}
static char last_gif_name[64]={0};
#include "space.h"
void gifInit(const char *name, bool force){
  //force true 为强制重新播放该gif
  // 文件名更换，需要重新初始化
  if(!strcmp(last_gif_name, name) && force == false){ 
    return;
  }
  snprintf(last_gif_name, sizeof(last_gif_name), "%s", name);
  //关闭前一个gif
  if(gif != NULL) gif->close(); 

  if(gif == NULL)
    gif = new AnimatedGIF; 

#ifdef TURBO_MODE
uint8_t pTurboBuffer[TURBO_BUFFER_SIZE + 256 + (160 * 120)];
#endif
  //打开一个新的gif文件
  
  gif->begin(GIF_PALETTE_RGB565_BE);
  //gif.open("/badapple.gif", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw);
  //gif->open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw);
  gif->open((uint8_t *)space, sizeof(space), GIFDraw);

#ifdef TURBO_MODE
    gif.setTurboBuf(pTurboBuffer);
#endif
  init_gif = 1;
}

void gifDeinit(){

  init_gif = 0;
  if(gif != NULL){ delete gif; gif = NULL; }
} 

int drawGif(const char *filename, int xpos, int ypos, bool force){
  //DBG_PTNf("debug gif draw1\r\n");
  x_offset = xpos;
  y_offset = ypos;

  /* 和Tjpg_decoder 冲突 */
  tft.setSwapBytes(false); // We need to swap the colour bytes (endianess)

  gifInit(filename, force);
  return ShowGIF(filename);
}

void playGif(const char *filename, int xpos, int ypos){
  tft.fillScreen(TFT_BLACK);
  if(gif == NULL)
  	gif = new AnimatedGIF; 
  gif->begin(GIF_PALETTE_RGB565_BE);
  /* 和Tjpg_decoder 冲突 */
  tft.setSwapBytes(false); // We need to swap the colour bytes (endianess)
  x_offset = xpos;
  y_offset = ypos;
  if (gif->open(filename, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
    //DBG_PTNf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    //tft.startWrite(); // The TFT chip slect is locked low
    //DBG_PTNf(" tft size = %d x %d\n",tft.width(), tft.height());
    while (gif->playFrame(true, NULL))
    {
      //yield();
      //DBG_PTNf(" frames [%d]\n",iFrames);
    }
    if(gif != NULL){gif->close(); delete gif; gif = NULL; }
    //tft.endWrite(); // Release TFT chip select for other SPI devices
  }
  else{
    //DBG_PTNf("failed show gif  [%s]\r\n",filename); 
  }
  return;
}