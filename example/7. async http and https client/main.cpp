/****************************************************************************************************************************
  AsyncHTTP_HTTPSRequest_ESP.ino - Dead simple AsyncHTTPSRequest for ESP8266, ESP32 and currently STM32 with built-in LAN8742A Ethernet

  For ESP8266, ESP32 and STM32 with built-in LAN8742A Ethernet (Nucleo-144, DISCOVERY, etc)

  AsyncHTTPSRequest_Generic is a library for the ESP8266, ESP32 and currently STM32 run built-in Ethernet WebServer

  Based on and modified from AsyncHTTPRequest Library (https://github.com/boblemaire/AsyncHTTPRequest)

  Built by Khoi Hoang https://github.com/khoih-prog/AsyncHTTPSRequest_Generic
  Licensed under MIT license

  Copyright (C) <2018>  <Bob Lemaire, IoTaWatt, Inc.>
  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published bythe Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  You should have received a copy of the GNU General Public License along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *****************************************************************************************************************************/
//************************************************************************************************************
//
// There are scores of ways to use AsyncHTTPSRequest.  The important thing to keep in mind is that
// it is asynchronous and just like in JavaScript, everything is event driven.  You will have some
// reason to initiate an asynchronous HTTP request in your program, but then sending the request
// headers and payload, gathering the response headers and any payload, and processing
// of that response, can (and probably should) all be done asynchronously.
//
// In this example, a Ticker function is setup to fire every 300 seconds to initiate a request.
// Everything is handled in AsyncHTTPSRequest without blocking.
// The callback onReadyStateChange is made progressively and like most JS scripts, we look for
// readyState == 4 (complete) here.  At that time the response is retrieved and printed.
//
// Note that there is no code in loop().  A code entered into loop would run oblivious to
// the ongoing HTTP requests.  The Ticker could be removed and periodic calls to sendRequest()
// could be made in loop(), resulting in the same asynchronous handling.
//
// For demo purposes, debug is turned on for handling of the first request.  These are the
// events that are being handled in AsyncHTTPSRequest.  They all begin with Debug(nnn) where
// nnn is the elapsed time in milliseconds since the transaction was started.
//
//*************************************************************************************************************

#if !( defined(ESP8266) ||  defined(ESP32) )
  #error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif

#define ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN_TARGET       "AsyncHTTPRequest_Generic v1.10.2"
#define ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN              1010002

#define ASYNC_HTTPS_REQUEST_GENERIC_VERSION_MIN_TARGET      "AsyncHTTPSRequest_Generic v2.2.1"
#define ASYNC_HTTPS_REQUEST_GENERIC_VERSION_MIN             2002001

/////////////////////////////////////////////////////////

// Uncomment for certain HTTP site to optimize
//#define NOT_SEND_HEADER_AFTER_CONNECTED        true

// Level from 0-4
#define ASYNC_HTTPS_DEBUG_PORT      Serial

#define _ASYNC_TCP_SSL_LOGLEVEL_    1
#define _ASYNC_HTTPS_LOGLEVEL_      1

// 300s = 5 minutes to not flooding
#define HTTPS_REQUEST_INTERVAL      60

// 10s
#define HEARTBEAT_INTERVAL          10

int status;     // the Wifi radio's status

const char* ssid        = "1024";
const char* password    = "2048@@@@";

#if (ESP8266)
  #include <ESP8266WiFi.h>
#elif (ESP32)
  #include <WiFi.h>
#endif

// Use larger queue size if necessary for large data transfer. Default is 512 bytes if not defined here
//#define ASYNC_QUEUE_LENGTH     512

// Use larger priority if necessary. Default is 10 if not defined here. Must be > 4 or adjusted to 4
//#define CONFIG_ASYNC_TCP_PRIORITY   (12)

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
// If use both AsyncHTTPRequest_Generic and AsyncHTTPSRequest_Generic, include AsyncHTTPRequest_Generic first or error
// because many definitions of AsyncHTTPSRequest_Generic rely on those of AsyncHTTPRequest_Generic
#include <AsyncHTTPRequest_Generic.h>             // https://github.com/khoih-prog/AsyncHTTPRequest_Generic
#include <AsyncHTTPSRequest_Generic.h>            // https://github.com/khoih-prog/AsyncHTTPSRequest_Generic

#include <Ticker.h>

#define NUM_DIFFERENT_SITES     3

const char* addreses[][NUM_DIFFERENT_SITES] =
{
  {"https://query1.finance.yahoo.com/v8/finance/chart/BTC-USD?interval=1h&range=30h", "https://worldtimeapi.org/api/timezone/Europe/Prague.txt"},
  {"http://worldtimeapi.org/api/timezone/Europe/London.txt",    "http://worldtimeapi.org/api/timezone/America/Vancouver.txt"},
  {"http://www.myexternalip.com/raw"}
};

#define NUM_ENTRIES_SITE_0        2
#define NUM_ENTRIES_SITE_1        2
#define NUM_ENTRIES_SITE_2        1

byte reqCount[]  = { NUM_ENTRIES_SITE_0, NUM_ENTRIES_SITE_1, NUM_ENTRIES_SITE_2 };
bool readySend[] = { true, true, true };

typedef enum
{
  HTTP_REQUEST    = 0,
  HTTPS_REQUEST   = 1,
} HTTP_Type;

AsyncHTTPSRequest request0;
AsyncHTTPRequest  request1;
AsyncHTTPRequest  request2;

typedef struct _AsyncHTTPRequestData
{
  void*       request;      // (void*) for AsyncHTTPRequest* or AsyncHTTPSRequest*
  HTTP_Type   httpType;
} AsyncHTTPRequestData;

AsyncHTTPRequestData myAsyncHTTPRequestData[] =
{
  { (void*) &request0, HTTPS_REQUEST },
  { (void*) &request1, HTTP_REQUEST  },
  { (void*) &request2, HTTP_REQUEST  }
};

// This is for HTTPS and must use AsyncHTTPSRequest
void requestCB0(void* optParm, AsyncHTTPSRequest* thisRequest, int readyState);

// This is for HTTP and must use AsyncHTTPRequest
void requestCB1(void* optParm, AsyncHTTPRequest*  thisRequest, int readyState);

// This is for HTTP and must use AsyncHTTPRequest
void requestCB2(void* optParm, AsyncHTTPRequest*  thisRequest, int readyState);

void sendRequest0();
void sendRequest1();
void sendRequest2();

typedef void (*requestCallback0)(void* optParm, AsyncHTTPSRequest* thisRequest, int readyState);
typedef void (*requestCallback1)(void* optParm, AsyncHTTPRequest*  thisRequest, int readyState);

typedef void (*sendCallback)();

void*         requestCB     [] = { (void*) requestCB0,   (void*) requestCB1, (void*) requestCB2   };
sendCallback  sendRequestCB [] = { sendRequest0, sendRequest1, sendRequest2 };

Ticker ticker;
Ticker ticker1;

void heartBeatPrint()
{
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    Serial.print(F("H"));        // H means connected to WiFi
  else
    Serial.print(F("F"));        // F means not connected to WiFi

  if (num == 80)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(F(" "));
  }
}

void sendRequest(uint16_t index)
{
  static bool requestOpenResult;

  reqCount[index]--;
  readySend[index] = false;

  if ( myAsyncHTTPRequestData[index].httpType == HTTPS_REQUEST )
  {
    requestOpenResult = ((AsyncHTTPSRequest *) myAsyncHTTPRequestData[index].request)->open("GET",
                                                                                            addreses[index][reqCount[index]]);

    if (requestOpenResult)
    {
      // Only send() if open() returns true, or crash
      Serial.print("\nSending HTTPS request: ");
      ((AsyncHTTPSRequest *) myAsyncHTTPRequestData[index].request)->send();
    }
    else
    {
      Serial.print("\nCan't send bad HTTPS request : ");
    }
  }
  else if ( myAsyncHTTPRequestData[index].httpType == HTTP_REQUEST )
  {
    requestOpenResult = ((AsyncHTTPRequest *) myAsyncHTTPRequestData[index].request)->open("GET",
                                                                                           addreses[index][reqCount[index]]);

    if (requestOpenResult)
    {
      // Only send() if open() returns true, or crash
      Serial.print("\nSending HTTP request: ");
      ((AsyncHTTPRequest *) myAsyncHTTPRequestData[index].request)->send();
    }
    else
    {
      Serial.print("\nCan't send bad HTTP request : ");
    }
  }

  Serial.println(addreses[index][reqCount[index]]);
}

void sendRequest0()
{
  sendRequest(0);
}

void sendRequest1()
{
  sendRequest(1);
}

void sendRequest2()
{
  sendRequest(2);
}

void sendRequests()
{
  for (int index = 0; index < NUM_DIFFERENT_SITES; index++)
  {
    reqCount[index] = 2;
  }

  reqCount[0] = NUM_ENTRIES_SITE_0;
  reqCount[1] = NUM_ENTRIES_SITE_1;
  reqCount[2] = NUM_ENTRIES_SITE_2;
}

// This is for HTTPS and must use AsyncHTTPSRequest
void requestCB0(void *optParm, AsyncHTTPSRequest *thisRequest, int readyState)
{
  (void) optParm;

  if (readyState == readyStateDone)
  {
    AHTTPS_LOGDEBUG0(F("\n**************************************\n"));
    AHTTPS_LOGDEBUG1(F("Response Code = "), thisRequest->responseHTTPString());

    if (thisRequest->responseHTTPcode() == 200)
    {
      Serial.println(F("\n**************************************"));
      Serial.println(thisRequest->responseText());
      Serial.println(F("**************************************"));
    }

    thisRequest->setDebug(false);
    readySend[0] = true;
  }
}

// This is for HTTP and must use AsyncHTTPRequest
void requestCB1(void *optParm, AsyncHTTPRequest *thisRequest, int readyState)
{
  (void) optParm;

  if (readyState == readyStateDone)
  {
    AHTTPS_LOGDEBUG0(F("\n**************************************\n"));
    AHTTPS_LOGDEBUG1(F("Response Code = "), thisRequest->responseHTTPString());

    if (thisRequest->responseHTTPcode() == 200)
    {
      Serial.println(F("\n**************************************"));
      Serial.println(thisRequest->responseText());
      Serial.println(F("**************************************"));
    }

    thisRequest->setDebug(false);
    readySend[1] = true;
  }
}

// This is for HTTP and must use AsyncHTTPRequest
void requestCB2(void *optParm, AsyncHTTPRequest *thisRequest, int readyState)
{
  (void) optParm;

  if (readyState == readyStateDone)
  {
    AHTTPS_LOGDEBUG0(F("\n**************************************\n"));
    AHTTPS_LOGDEBUG1(F("Response Code = "), thisRequest->responseHTTPString());

    if (thisRequest->responseHTTPcode() == 200)
    {
      Serial.println(F("\n**************************************"));
      Serial.println(thisRequest->responseText());
      Serial.println(F("**************************************"));
    }

    thisRequest->setDebug(false);
    readySend[2] = true;
  }
}
#include <esp_task_wdt.h>
void btnTask(void *pvParameters) {
  while (1) {
    //btn.tick();
    //不延迟会造成整个系统卡顿
    esp_task_wdt_reset();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  while (!Serial && millis() < 5000);

  delay(200);

  Serial.print("\nStarting AsyncHTTP_HTTPSRequest_ESP on ");
  Serial.println(ARDUINO_BOARD);

#if defined(ESP32)
  Serial.println(ASYNC_TCP_SSL_VERSION);
#else
  //Serial.println(ESPASYNC_TCP_SSL_VERSION);
#endif

  Serial.println(ASYNC_HTTPS_REQUEST_GENERIC_VERSION);
  Serial.println(ASYNC_HTTP_REQUEST_GENERIC_VERSION);

#if defined(ASYNC_HTTPS_REQUEST_GENERIC_VERSION_MIN)

  if (ASYNC_HTTPS_REQUEST_GENERIC_VERSION_INT < ASYNC_HTTPS_REQUEST_GENERIC_VERSION_MIN)
  {
    Serial.print(F("Warning. Must use this example on Version equal or later than : "));
    Serial.println(ASYNC_HTTPS_REQUEST_GENERIC_VERSION_MIN_TARGET);
  }

#endif

#if defined(ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN)

  if (ASYNC_HTTP_REQUEST_GENERIC_VERSION_INT < ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN)
  {
    Serial.print(F("Warning. Must use this example on Version equal or later than : "));
    Serial.println(ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN_TARGET);
  }

#endif

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi SSID: " + String(ssid));

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.print(F("\nAsyncHTTPSRequest @ IP : "));
  Serial.println(WiFi.localIP());

  for (int index = 0; index < NUM_DIFFERENT_SITES; index++)
  {
    if ( myAsyncHTTPRequestData[index].httpType == HTTPS_REQUEST )
    {
      ((AsyncHTTPSRequest *) myAsyncHTTPRequestData[index].request)->setDebug(false);
      ((AsyncHTTPSRequest *) myAsyncHTTPRequestData[index].request)->onReadyStateChange( (requestCallback0) requestCB[index]);
    }
    else if ( myAsyncHTTPRequestData[index].httpType == HTTP_REQUEST )
    {
      ((AsyncHTTPRequest *) myAsyncHTTPRequestData[index].request)->setDebug(false);
      ((AsyncHTTPRequest *) myAsyncHTTPRequestData[index].request)->onReadyStateChange((requestCallback1) requestCB[index]);
    }
  }

  ticker.attach(HTTPS_REQUEST_INTERVAL, sendRequests);

  ticker1.attach(HEARTBEAT_INTERVAL, heartBeatPrint);
    esp_task_wdt_init(5, true);//看门狗
    xTaskCreatePinnedToCore(
        btnTask,     // 任务函数
        "btnTask",   // 任务名称
        10000,          // 任务堆栈大小（字节）
        NULL,           // 任务参数
        1,              // 任务优先级
        NULL,           // 任务句柄
        0               // 分配给第二个内核的内核编号（1表示第二个内核）
    );
}

void loop()
{
  for (int index = 0; index < NUM_DIFFERENT_SITES; index++)
  {
    if ((reqCount[index] > 0) && readySend[index])
    {
      sendRequestCB[index]();

      // Don't reduce this or possible crash. TLS needs long time to work.
      delay(100);
    }
  }
}