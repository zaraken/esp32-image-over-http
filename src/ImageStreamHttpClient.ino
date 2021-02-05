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

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

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

          //Serial.println("Next chunk");
          //Serial.println(size);
          if (size)
          {
            // Assumption - buffer is big enough
            int c = stream->readBytes(&img_buff[data_len], size);

            data_len += c;
          }
        }

        Serial.print("Download time ");
        Serial.println(millis() - dt);
        Serial.print("Download size");
        Serial.println(data_len);

        dt = millis();

        //drawArrayJpeg(img_buff, data_len, 0, 0); // Draw a jpeg image stored in memory at x,y

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

void jpegInfo()
{
  Serial.println(F("==============="));
  Serial.println(F("JPEG image info"));
  Serial.println(F("==============="));
  Serial.print(F("Width      :"));
  Serial.println(JpegDec.width);
  Serial.print(F("Height     :"));
  Serial.println(JpegDec.height);
  Serial.print(F("Components :"));
  Serial.println(JpegDec.comps);
  Serial.print(F("MCU / row  :"));
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print(F("MCU / col  :"));
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print(F("Scan type  :"));
  Serial.println(JpegDec.scanType);
  Serial.print(F("MCU width  :"));
  Serial.println(JpegDec.MCUWidth);
  Serial.print(F("MCU height :"));
  Serial.println(JpegDec.MCUHeight);
  Serial.println(F("==============="));
}

//####################################################################################################
// Draw a JPEG on the TFT pulled from a program memory array
//####################################################################################################
void drawArrayJpeg(const uint8_t arrayname[], uint32_t array_size, int xpos, int ypos) {

  int x = xpos;
  int y = ypos;

  JpegDec.decodeArray(arrayname, array_size);

  //jpegInfo(); // Print information from the JPEG file (could comment this line out)
  Serial.print("renderJPEG\n");
  renderJPEG(x, y);

  Serial.println("#########################");
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void renderJPEG(int xpos, int ypos) {

  // retrieve infomration about the image
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
  uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while (JpegDec.read()) {

    // save a pointer to the image block
    pImg = JpegDec.pImage ;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    tft.startWrite();

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
    {

      // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
      tft.setAddrWindow(mcu_x, mcu_y, win_w, win_h);

      // Write all MCU pixels to the TFT window
      while (mcu_pixels--) {
        // Push each pixel to the TFT MCU area
        tft.pushColor(*pImg++);
      }

    }
    else if ( (mcu_y + win_h) >= tft.height()) JpegDec.abort(); // Image has run off bottom of screen so abort decoding

    tft.endWrite();
  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime;

  // print the results to the serial port
  Serial.print(F(  "Total render time was    : ")); Serial.print(drawTime); Serial.println(F(" ms"));
  Serial.println(F(""));
}
