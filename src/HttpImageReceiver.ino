/**
 * ImageStreamHttpClien.ino
 *
 *  Created on: 02.02.20201
 *
 */

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>

#include <TFT_eSPI.h>       // Display library by bodmer
#include <JPEGDecoder.h>    // by bodmer
#include <TFT_eFEX.h>       // Display library extension

#include <WiFiSecrets.h>    // N.B. Create this file for your application
// Contents
// const char * wifi_ssid = "your SSID";
// const char * wifi_pass = "your WiFi password";

#include <AppConf.h>        // contains confogiration settings such as
                            // server ip and port. Adjust as necessary

TFT_eSPI tft = TFT_eSPI();
TFT_eFEX tftx = TFT_eFEX(&tft);

WiFiMulti wifiMulti;

// Jpeg Image buffer size
// According to wikipedia
// https://en.wikipedia.org/wiki/JPEG#Sample_photographs
// For highest quality images (Q=100), about 8.25 bits per color pixel
// is required.
// (135 * 240)px * 8.25bit/px = 33.5kB
const uint16_t img_buff_len = 33500;
uint8_t img_buff[img_buff_len] = {0};
uint32_t dt;

// helper function for viewing available memory
void print_mem()
{
  Serial.print("ESP.getFreeHeap ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.print("kB; xPortGetFreeHeapSize ");
  Serial.print(xPortGetFreeHeapSize() / 1024);
  //Serial.print("kB; MALLOC_CAP_32BIT ");
  //Serial.print(heap_caps_get_free_size(MALLOC_CAP_32BIT) / 1024);
  //Serial.print("kB; MALLOC_CAP_SPIRAM ");
  //Serial.print(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
  //Serial.print("kB; MALLOC_CAP_DMA ");
  //Serial.print(heap_caps_get_free_size(MALLOC_CAP_DMA) / 1024);
  Serial.println("kB");
}

void setup() {

    Serial.begin(115200);

    Serial.println(); Serial.println(); Serial.println();

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    print_mem();

    // WiFi init
    wifiMulti.addAP(wifi_ssid, wifi_pass);

    // Display init
    tft.begin();
    tft.setRotation(1); // 0 & 2 Portrait. 1 & 3 landscape

    // Swap the colour byte order when rendering
    //tft.setSwapBytes(true);

    // I see a black screen and I want to paint it red
    tft.fillScreen(TFT_RED);
}

void loop() {
  print_mem();

  // wait for WiFi connection
  if ((wifiMulti.run() == WL_CONNECTED))
  {

    HTTPClient http;

    //Serial.print("[HTTP] begin...\n");

    // configure server and url
    http.begin(server_ip, server_port);

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {

        // get length of document (is -1 when Server sends no Content-Length header)
        int len = http.getSize();

        // get tcp stream
        WiFiClient *stream = http.getStreamPtr();

        // read all data from server
        uint16_t data_len = 0;
        dt = millis();
        while (http.connected() && (len > 0 || len == -1))
        {
          // get available data size
          size_t size = stream->available();

          //Serial.println(size);
          if (size)
          {
            // Assumption - buffer is big enough
            // TODO: do something if buffer is not big enough ...
            int c = stream->readBytes(&img_buff[data_len], size);

            data_len += c;
          }
        }

        Serial.print("Download time ");
        Serial.println(millis() - dt);
        Serial.print("Download size");
        Serial.println(data_len);

        dt = millis();

        // requires a fix, possibly in TFT_eFEX.cpp
        // jpeg.tft = this -> jpeg._tft;
        tftx.drawJpg(img_buff, data_len, 0, 0);

        Serial.print("Render time ");
        Serial.println(millis() - dt);

        delay(3000);

        Serial.println();
        Serial.print("[HTTP] connection closed or file end.\n");
      }
    }
    else
    {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    }

    delay(1);
}
