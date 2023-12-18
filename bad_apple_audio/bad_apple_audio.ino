/*
   wifi传输视频+闪存存放音频信息，可近似30帧，已添加用于稳定帧数的非阻塞延时/超时补偿。
   由于向显示屏写入的IIC过程是耗时大头，可能会出现音符时值不稳定的情况，但由于音符按照startTime播放，不会出现一直延迟，总体上是稳定的。
   可支持多通道音频输出，请使用无源蜂鸣器。每个通道（音轨）要求不能有时间上重叠的音符。
   注意！！！windows用户记得把wifi的专用改成公用！！！不然板子找不到pc的server！！！

*/


#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <FS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>

#define FRAME 704
#define FRAME_LENGTH 88
#define FRAME_WIDTH 64
#define BUFFERSIZE 100

// digital pin 2 has a pushbutton attached to it. Give it a name:
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define CENTER_X 20 //(128-88)/2

//音频信息文件放在闪存里
String file_name = "/output_audio.dat"; //filename in flash 主旋律音轨
String file_name_acc = "/output_audio_1.dat"; //accompany in flash 伴奏音轨

const char* ssid = "yourwifi"; //请改成你自己的wifi名
const char* password = "yourwifipswd"; //请改成你自己的wifi密码
short st = 0;

//用于定时触发
uint32_t pretimeStamp = 0;
const uint32_t period = 33; //每帧间隔33ms（30帧）
unsigned long audio_startTimeStamp = 0; //音乐正式开始播放的时间
unsigned long note_startTimeStamp = 0; //PWM实现的时候用的，为了多音轨改用tone的duration了
short audio_start_flag = 0;
short interval_flag = 0;
short noteflag=1;
int compensation_time =0; //超时补偿

//用于读取完整图片
short flag = 0;
int rest = FRAME;
int now_pic = 0;

//图片buffer
static unsigned char picbuf[FRAME] = {};
static unsigned char picbuf_2[FRAME] = {};

//音频buffer
static uint32_t audiobuf[BUFFERSIZE][3]={};
static uint32_t accAudiobuf[BUFFERSIZE][3]={};
//下一个要播放的音符
static uint32_t waitaudio[3]={0,0,0};
static uint32_t waitaccAudio[3]={0,0,0};

//循环队列的head和tail，每个音频buffer都有一个head&tail
int head = 0; 
int tail = 0;
int head_acc=0;
int tail_acc=0;
int count=0; //音符计数（调试用）

//音频文件
File audioFile;
File audioFile_accompany;

WiFiClient client;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  analogWriteRange(1023);
  // make the pushbutton's pin an input:
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  WiFi.begin(ssid, password);
  SPIFFS.begin();  //启动SPIFFS
  //检查文件是否存在
  if (SPIFFS.exists(file_name)&&(file_name_acc)) {
    Serial.print(file_name);
     Serial.print(file_name_acc);
    Serial.println(" FOUND.");
  } else {
    Serial.print(" NOT FOUND.");
  }
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();
  delay(1000);
}

//显示在显示屏上
void drawpic() {
  //非阻塞定时
  uint32_t timestamp = millis();
  //开始播放第一帧
  if(audio_start_flag==0){
      pretimeStamp=millis();
      timestamp = millis()+period;
    }
  //若时间间隔大等于33ms则播放
  uint32_t interval=timestamp - pretimeStamp;
  if (interval >= period) {
    pretimeStamp = timestamp;
    if(interval-period >0){
      compensation_time+=interval-period; //计算超时时间
      }
    display.clearDisplay();

    display.drawBitmap(CENTER_X, 0,
                       picbuf, FRAME_LENGTH, FRAME_WIDTH, WHITE);
    display.display();
    setAudiostart();

  } else {
    //超时补偿，刷新快的时候如果当前存在超时则补偿超时
    int tempcomp=period-interval;
    if(compensation_time>0 && compensation_time-tempcomp>-10){
        compensation_time-=tempcomp;
         pretimeStamp = timestamp;
         display.clearDisplay();

         display.drawBitmap(CENTER_X, 0,
                       picbuf, FRAME_LENGTH, FRAME_WIDTH, WHITE);
         display.display();
         return;
      }
    //否则不刷新
    interval_flag = 1;
    return;
  }
}

void setAudiostart(){
      //开始播放第一帧时，设置音频开始时间
      if(audio_start_flag==0){
       audio_start_flag=1;
       audio_startTimeStamp=system_get_time();
      }
}

//拼接图片
void arr1(int info) {
  int i;
  for (i = 0; i < info; i++) {
    picbuf[now_pic + i] = picbuf_2[i];
  }
}

//SPIFFS没有提供读取x个字节的API，因此手动读4个字节并拼成一个int
uint32_t readInt(File f){
  uint32_t result = 0;
    for (int i = 0; i < 4; i++) {
    if (f.available()) {
      uint8_t byteValue = f.read();
      result |= static_cast<uint32_t>(byteValue) << (8 * i);
    } else {
      return 0;
    }
  }
  return result;
}

//从文件f中读取主旋律音符信息放到主旋律buffer里
void readNote(File f) {
  for (int i = 0; i < 3; i++) {
    audiobuf[head][i] = readInt(f);
  }
   head = (head + 1) % BUFFERSIZE;  // 移动 head，并确保在缓冲区大小内循环
}
//从主旋律buffer里读取音符信息放到下一个要播放的音符里
uint32_t* readAudiobuf() {
  uint32_t *bufdata = new uint32_t[3];
  for (int i = 0; i < 3; i++) {
    bufdata[i]=audiobuf[tail][i];
  }
  tail = (tail + 1) % BUFFERSIZE;  // 移动 tail，并确保在缓冲区大小内循环
  return bufdata;
}

//同上，但读取的是伴奏音符信息并放到伴奏buffer里
void readaccNote(File f) {
  for (int i = 0; i < 3; i++) {
    accAudiobuf[head_acc][i] = readInt(f);
  }
   head_acc = (head_acc + 1) % BUFFERSIZE;  // 移动 head，并确保在缓冲区大小内循环
}
uint32_t* readaccAudiobuf() {
  uint32_t *bufdata = new uint32_t[3];
  for (int i = 0; i < 3; i++) {
    bufdata[i]=accAudiobuf[tail_acc][i];
  }
  tail_acc = (tail_acc + 1) % BUFFERSIZE;  // 移动 tail，并确保在缓冲区大小内循环
  return bufdata;
}

//当前时间（系统时间-音频开始时间）大于下一个要播放的音符的起始时间则播放下一个音符
void checkAudiotoPlay(File f){
  if(audio_start_flag==1){
  unsigned long nowTimeStamp=system_get_time();
  if(static_cast<uint32_t>(nowTimeStamp-audio_startTimeStamp)>=waitaudio[2]){
         if(waitaudio[2]!=0){ //starttime初始化时为0，此时不应发出声音
          tone(D4,waitaudio[0],waitaudio[1]/1000-50); //音符信息中的时间单位为us，但让某引脚按特定频率输出的函数tone的单位为ms
          /*
          Serial.print(nowTimeStamp-audio_startTimeStamp);
          Serial.print("freq: ");
          Serial.print(waitaudio[0]);
          Serial.print(" ,dura: ");
          Serial.print(waitaudio[1]);
          Serial.print(" ,start: ");
          Serial.print(waitaudio[2]);
          Serial.println();
          */
          }
         note_startTimeStamp=nowTimeStamp-audio_startTimeStamp;
         count+=1;
         //播放后，从buffer中取出下一个音符，并更新buffer
         uint32_t* temp=new uint32_t[3];
         temp=readAudiobuf();
         for (int i = 0; i < 3; i++) {
            waitaudio[i]=temp[i];
           }
         readNote(f);
         delete temp;
    }
  }
}
//同上
void checkAccAudiotoPlay(File f){
  if(audio_start_flag==1){
  unsigned long nowTimeStamp=system_get_time();
  if(static_cast<uint32_t>(nowTimeStamp-audio_startTimeStamp)>=waitaccAudio[2]){
         if(waitaccAudio[2]!=0){
          tone(D5,waitaccAudio[0],waitaccAudio[1]/1000-50);
          /*
          Serial.print("freq: ");
          Serial.print(waitaccAudio[0]);
          Serial.print(" ,dura: ");
          Serial.print(waitaccAudio[1]);
          Serial.print(" ,start: ");
          Serial.print(waitaccAudio[2]);
          Serial.println();
          */
          }
         uint32_t* temp=new uint32_t[3];
         temp=readaccAudiobuf();
         for (int i = 0; i < 3; i++) {
            waitaccAudio[i]=temp[i];
           }
         readaccNote(f);
         delete temp;
    }
  }
}
//PWM时使用的停止播放函数，但只能单音轨（所有引脚共用一个频率），已废弃
void playNoteOff(){
  unsigned long nowTimeStamp=system_get_time()-audio_startTimeStamp;
  if(static_cast<uint32_t>(nowTimeStamp-note_startTimeStamp)>=waitaudio[1]-50000){
        analogWrite(D4,0);
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
      audioFile = SPIFFS.open(file_name, "r");
      audioFile_accompany=SPIFFS.open(file_name_acc, "r");
      st = 1;
      Serial.println(audioFile.size());
      count=0;
      head=0;
      head_acc=0;
      //初始化时先读满缓存
      for (int i = 0; i < BUFFERSIZE; i++) {
          // uint32_t* temp=new uint32_t[3];
          readNote(audioFile);
          readaccNote(audioFile_accompany);
          // temp=readAudiobuf();
          // delete temp;
      }
      tail=0;
      tail_acc=0;
      audio_start_flag=0;
      compensation_time=0;
      waitaudio[2]=0;
      waitaccAudio[2]=0;
      audio_startTimeStamp = 0;
      note_startTimeStamp = 0;
    }
    Serial.println(client.connected()); //没连上server就打印0
  }
  while (client.available()) {
    //每次循环检查下一个音符是否到了播放时间
    checkAudiotoPlay(audioFile);
    checkAccAudiotoPlay(audioFile_accompany);
    if (interval_flag == 1) {
      interval_flag = 0;
      drawpic();
    } else {
      //上次没读满一帧的情况
      if (flag == 1) {
        int info = client.read(picbuf_2, rest);
        if (info == rest) {
          arr1(info);
          drawpic();
          flag = 0;
        } else if (info < rest && info > 0) {
          now_pic = FRAME - rest;
          arr1(info);
          rest -= info;
        } else {
          rest = 0;
          now_pic = 0;
          flag = 0;
        }

      } else {
        //初始情况，直接读一整帧
        int info = client.read(picbuf, FRAME);
        if (info == FRAME) {
          flag = 0;
          drawpic();
        } else if (info < FRAME && info != 0) {
          rest = FRAME - info;
          now_pic = info;
          flag = 1;
        }
      }
    }
  }
  //重新连接
  if (client.connected() == 0) {
    st = 0;
    flag = 0;
  }
}
