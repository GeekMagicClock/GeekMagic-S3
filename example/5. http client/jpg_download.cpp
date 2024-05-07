#include "jpg_download.h"
#include "my_debug.h"
#include "WiFiClientSecure.h"

#if 1
//http://fubotv.f3322.net:4762/GIF1/G512.gif
//#define JPG_URL F("https://www.dmoe.cc/random.php")
#define JPG_URL "https://github.com/GeekMagicClock/smalltvpro/raw/main/images/img-smalltv-pro.jpg"
WiFiClientSecure wificlient;
String filename = "/demo.jpg";
bool download_jpg() {
  HTTPClient http;
  bool success = false;

  String url = JPG_URL ;
  wificlient.setInsecure();
  
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(wificlient, url);
  
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    File file = LittleFS.open(filename, "w");

    if (file) {
      WiFiClient* stream = http.getStreamPtr();
      uint8_t buffer[2096];
      size_t bytesRead = 0;
      size_t total = 0;
      while ((bytesRead = stream->readBytes(buffer, sizeof(buffer))) > 0) {
        //file.write(buffer, bytesRead);
        size_t bytesWritten = file.write(buffer, bytesRead);
        while (bytesWritten != bytesRead) {
          bytesWritten += file.write(buffer + bytesWritten, bytesRead - bytesWritten);
        }
        total += bytesRead;
      }
     // DBG_PTN(total);
      file.close();
      success = true;
      DBG_PTN(http.getSize());
      if (total == http.getSize()) {
        success = true;
        DBG_PTN("File downloaded and saved successfully.");
      } else {
        DBG_PTN("File write error. Data may be incomplete.");
      }
    } else {
      DBG_PTN("Failed to open file for writing.");
    }
  } else {
    DBG_PTN("HTTP request failed with error code: ");
    DBG_PTN(httpCode);
  }

  http.end();
  return success;
}
#endif