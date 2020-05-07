#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h" 
#include "soc/rtc_cntl_reg.h"  
#include "esp_http_server.h"

//Определения для подключения выводов камеры
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//Константы
const String strRTR = String("[FTP_DATA]-RTR-[ED]");
#define BUFERSIZE 1000

//Переменные параметров камеры
camera_fb_t * fb = NULL;
size_t _jpg_buf_len = 0;
uint8_t * _jpg_buf = NULL;
bool key = 0;

uint32_t offset0 = 0;
uint8_t curbyte=0, befbyte=0;

//переменные парамтров сервера
const char* ssid = "SSID of your WiFi point";
const char* password = "password of your WiFi";
String bufer;
unsigned char chbufer[100];
unsigned char packet[BUFERSIZE+5];
//uint32_t beforeEOF = 0;
//bool FTPresolution = 1;



uint32_t byteiter = 0;
int WriteSize = 0;

WiFiServer server(12345);//Service Port

void setup() {
  //camera setup
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  config.frame_size = FRAMESIZE_UXGA; //SVGA == 800x600   UXGA == 1600x1200
  config.jpeg_quality = 30; //<<--tested~~~~~~~~
  config.fb_count = 2;

  //camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("ERROR: Camera init failed with error 0x%x", err);
    return;
  }
  
  
  delay(5000);

  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this IP to connect: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    key = 1;
    return;
  }

  // Wait until the client sends some data
  Serial.println("New client");
  //while(!client.available()){
  //  delay(1);
  //  key = 1;
  //}
  Serial.println("Client available");

  if(key){
    fb = esp_camera_fb_get(); //делаем фото и сохраняем в буфер
    delay(5000);

    //formatting
    if (!fb){
    Serial.println("ERROR: Camera capture failed");
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){    //если формат картинки  не соответствует jpeg 
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len); //преобразуем файл картики в формат jpeg и сохраняем код картинки в _jpg_buf и ее размер в _jpg_buf_len
          esp_camera_fb_return(fb);           //???
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("ERROR: JPEG compression failed");
          }
          Serial.println("Parameters:");
          Serial.println("Standard = coverted jpeg");
          Serial.print("Buf_len = "); Serial.println(_jpg_buf_len);
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
          Serial.println("Parameters:");
          Serial.println("Standard = NO converted jpeg");
          Serial.print("Buf_len = "); Serial.println(_jpg_buf_len);
        }
      }  
    }
    Serial.println("Sending is started");

    
    //sending____________________________________
  offset0 = 0;
  //beforeEOF = uint32_t(_jpg_buf_len);
    while(true){
      bufer = client.readString();

      Serial.print("OPTIMIZATION >>>>> POINT 1 Time: "); //<<<< OPTIM
      Serial.println(millis());                          //<<<< OPTIM
      bufer.toCharArray((char*)chbufer, 100);
      Serial.print("OPTIMIZATION >>>>> POINT 2 Time: "); //<<<< OPTIM
      Serial.println(millis());                          //<<<< OPTIM
      Serial.println((char*)chbufer);
      
      
      //проверить на готовность принять данные
      if(!strcmp((char*)chbufer, "[FTP_DATA]-RTR-[ED]")){
        Serial.print("OPTIMIZATION >>>>> POINT 3 Time: "); //<<<< OPTIM
        Serial.println(millis());                          //<<<< OPTIM
        Serial.println("[JPEG] RTR detect data sending begin.   ");
        //если смещение стало больше чем размер файла, отправить команду завершения пересылки
        if(offset0 > _jpg_buf_len/*beforeEOF*/){
          Serial.println("[JPEG] Data sending ended");
          client.print("[FTP_DATA]-EOF-[ED]");
          client.print("[FTP_DATA]-EOP-[ED]");
          break;
        }
        Serial.print("OPTIMIZATION >>>>> POINT 4 Time: "); //<<<< OPTIM
        Serial.println(millis());                          //<<<< OPTIM
        
        //отправить данные побайтово <<<<<<<<<<<
        for(byteiter = 0; byteiter < BUFERSIZE; ++byteiter){
          WriteSize += client.write((byte)_jpg_buf[byteiter+offset0]);
          
          if((byteiter + offset0) >= (_jpg_buf_len - 1)){
            Serial.println("[JPEG] END of file detected");
            break;
          }
        }
        
        Serial.print("OPTIMIZATION >>>>> POINT 5 Time: "); //<<<< OPTIM
        Serial.println(millis());                          //<<<< OPTIM
        
        //завершить отправку пакета отправкой флага <<<
        client.print("[FTP_DATA]-EOP-[ED]");

        Serial.print("Sended: ");
        Serial.print(WriteSize,DEC);
        Serial.println(" byte");
        
        //сместить указатель на размер пакета
        offset0 += BUFERSIZE;
        //beforeEOF -= BUFERSIZE;
      }else{
        Serial.println("[ERROR] Data resend");
        //client.print("[FTP_DATA]-EOP-[ED]");
        //client.print((char*)packet);
      }
      
      //если смещение стало больше чем размер файла, отправить команду завершения пересылки
      
      
    }
    
    
    esp_camera_fb_return(fb);
    fb = NULL;
    free(_jpg_buf);
    _jpg_buf = NULL;

    Serial.println("Sending is ended");
    while(1) delay(100);
  }

  // Read the first line of the request
  String reseive = client.readStringUntil('\0');
  Serial.println(reseive);

// Match the request

if (reseive == "C") {
  Serial.println("Data reseived, \'C\' detected");
  //WriteSize = client.print("Data reseived? test successful"); //use client.print(string, BIN);
  client.write('C'); //use for one byte sell
  Serial.println("Data test sended");
} 


delay(1);
}
