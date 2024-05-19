#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    // Stop further decoding as image is running off bottom of screen
    if (y >= tft.height())
        return 0;

    // This function will clip the image block rendering automatically at the TFT boundaries
    tft.pushImage(x, y, w, h, bitmap);

    // This might work instead if you adapt the sketch to use the Adafruit_GFX library
    // tft.drawRGBBitmap(x, y, bitmap, w, h);

    // Return 1 to decode next block
    return 1;
}

void init_jpg(){
    // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
    TJpgDec.setJpgScale(1);
    // The decoder must be given the exact name of the rendering function above
    TJpgDec.setCallback(tft_output);
}

void draw_jpg(String filename){
    tft.setSwapBytes(true); // We need to swap the colour bytes (endianess)
    TJpgDec.drawFsJpg(0, 0, filename, LittleFS);   
}