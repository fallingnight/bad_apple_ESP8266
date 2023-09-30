/*
 * wifi传输版，不稳定，但是极限速度可以接近30帧。
 * 建议使用时在传输端（Fileserver.java)改大传输间隔。
 * 此程序并未对每帧绘制间隔作严格的限制，若要稳定帧数可以自行在此程序中添加延时。
 * 注意！！！windows用户记得把wifi的专用改成公用！！！不然板子找不到pc的server！！！
 * 
 */


#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>

#define FRAME 704
#define FRAME_LENGTH 88
#define FRAME_WIDTH 64

// digital pin 2 has a pushbutton attached to it. Give it a name:
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

const char* ssid = "abc"; //请改成你自己的wifi名
const char* password = "12345678"; //请改成你自己的wifi密码
short st = 0;
int lasttime=0;
short flag = 0;
int rest = 0;

//图片buffer
static unsigned char picbuf[FRAME] = {};
static unsigned char picbuf_2[FRAME] = {}; 

WiFiClient client;

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
  WiFi.begin(ssid, password);
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
  int nowtime=system_get_time()-lasttime;
}

//拼接图片
void arr1(int rest) {
  int i;
  int now_pic=FRAME-rest;
  for (i = 0; i < rest; i++) {
    picbuf[now_pic + i] = picbuf_2[i];
  }
}


// the loop routine runs over and over again forever:
void loop() {

  //连接wifi
  if (st == 0 && WiFi.status() == WL_CONNECTED) {
    const uint16_t httpPort = 12001; //请改成你传输程序里的端口
    const char* host = "192.168.43.26"; //请改成你电脑的ip
    display.clearDisplay();
    display.display();
    client.connect(host, httpPort);
    if (client.connected() == 1) {
      st = 1;
    }
    Serial.println(client.connected()); //没连上server就打印0
  }
  
  while (client.available()) {
    lasttime=system_get_time();
    //上次没读满一帧的情况
    if (flag == 1) {
      int info = client.read(picbuf_2, rest);
      //Serial.println(info);
      if (info == rest) {
        arr1(rest);
        drawpic();
      }
      flag = 0;
      rest = 0;
    } else {
      //初始情况，直接读一整帧
      int info = client.read(picbuf, FRAME);
      //Serial.println(info);
      if (info == FRAME) {
        flag = 0;
        drawpic();
      } else {
        if(info<FRAME){
        rest = FRAME - info;
        }
        flag = 1;
      }
    }
  }
  
  //重新连接
  if (client.connected() == 0) {
    st = 0;
    flag = 0;
  }
}
