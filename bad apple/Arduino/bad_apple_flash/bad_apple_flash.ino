/*
 * 闪存版本，直接从闪存中读取数据显示。优点是稳定，缺点是不能播放较长的视频且速度偏慢（帧率上限低）。
 * 如何将文件载入esp8266闪存请参照：http://www.taichi-maker.com/homepage/esp8266-nodemcu-iot/iot-c/spiffs/
 */

#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FS.h>

#define FRAME 704
#define FRAME_LENGTH 88
#define FRAME_WIDTH 64

// digital pin 2 has a pushbutton attached to it. Give it a name:
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

String file_name = "/ba.bin"; //filename in flash
unsigned char picbuf[704] = {};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  // make the pushbutton's pin an input:
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  SPIFFS.begin();  //启动SPIFFS
  if (SPIFFS.exists(file_name)) {
    Serial.print(file_name);
    Serial.println(" FOUND.");
  } else {
    Serial.print(file_name);
    Serial.print(" NOT FOUND.");
  }
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();
  delay(1000);
}

//显示在显示屏上
void drawpic() {
  display.clearDisplay();

  display.drawBitmap(0, 0,
                     picbuf, FRAME_LENGTH, FRAME_WIDTH, WHITE);
  display.display();
}

//从文件中读取一帧
void readFrame(File f) {
  for (int i = 0; i < FRAME; i++) {
    picbuf[i] = (unsigned char)f.read();
  }
}

// the loop routine runs over and over again forever:
void loop() {
  File dataFile = SPIFFS.open(file_name, "r");
  Serial.println(dataFile.size());
  for (int i = 0; i < dataFile.size() / FRAME; i++) {
    //int time_1=system_get_time();
    readFrame(dataFile);
    drawpic();
    //int time_delay=system_get_time()-time_1;
    //Serial.println(time_delay);
  }

  dataFile.close();
}
