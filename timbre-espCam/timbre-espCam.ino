
//String BOTtoken = "6068641172:AAF_9siLYwZ-JFoX51Evfuh9RXk-jSB9UlI";  // Remplazar por el BotToken
//String CHAT_ID = "5388508527"; // Remplazar por el ID del usuario
// inclimos librerias
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// inicializamos el wifi
const char* ssid = "BJ2 5.8Ghz";
const char* password = "Benitojuarez";

// inicializamos el BOT de telegram
String BOTtoken = "6068641172:AAF_9siLYwZ-JFoX51Evfuh9RXk-jSB9UlI";  // Remplazar con el BOT TOKEN
String CHAT_ID = "5388508527"; // Remplazar con el chatID del usuario

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

// setemas las variables para la maquina de estado de antirrebote del boton

#define BUTTON_PIN 14
int buttonState;
#define BUTTON_STATE_READ 0
#define BUTTON_STATE_CHECK 1
#define BUTTON_STATE_REALEASE 2
#define BUTTON_PRESSED 0
#define BUTTON_NOT_PRESSED 1
#define YES_PRESSED 0
#define NOT_PRESSED 1

// VAriables para el timbre
#define DOORBELL_PIN 6
bool buttonFlag;
int doorbellTimer;
int actualTime;
int buttonTimer;

//CAMERA_MODEL_AI_THINKER
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

//funcion que inicializa la camara
void configInitCamera(){
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

  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  // Drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_CIF);  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
}


// Funcion para mandar la foto a telegram
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  
  Serial.println("Connect to " + String(myDomain));


  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + CHAT_ID + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    //Serial.println(getBody);
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  return getBody;
}

void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  // Init Serial Monitor
  Serial.begin(115200);

  // Configuracion del boton y el timbre
  pinMode(BUTTON_PIN, INPUT);
  pinMode(DOORBELL_PIN, OUTPUT);

  // Config e init de la camara
  configInitCamera();

  // Conectamos el Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP()); 
}

void loop() {
  /*readButtonState(); // Leemos el boton
  if (buttonFlag == BUTTON_PRESSED) { //si esta presionado mandamos la foto  y hacemos sonar el timbre
    Serial.println("Preparing photo");
    sendPhotoTelegram(); 
    //doorbellTimer = millis();
    //ringDoorbell();
    buttonFlag = BUTTON_NOT_PRESSED;
  }*/
  if (digitalRead(BUTTON_PIN == 0)){
    Serial.println("Preparing photo");
    sendPhotoTelegram(); 
  }
  Serial.println(digitalRead(BUTTON_PIN));
}

//maquina de estado antirrebote del boton
void readButtonState(){
  switch (buttonState){
    case BUTTON_STATE_READ:
      if (digitalRead(BUTTON_PIN == YES_PRESSED)){
        buttonState = BUTTON_STATE_CHECK;
        actualTime = millis();   
      }
      break;
    case BUTTON_STATE_CHECK:
      buttonTimer = millis() - actualTime;
      if (digitalRead(BUTTON_PIN) == YES_PRESSED && buttonTimer >= 200){ //queremos quue el boton alla estado presionado por 200ms
        buttonState = BUTTON_STATE_REALEASE;
      }
      if (digitalRead(BUTTON_PIN) == NOT_PRESSED && buttonTimer >= 200){
        buttonState = BUTTON_STATE_READ;
      }
      break;
     case BUTTON_STATE_REALEASE:
      if (digitalRead(BUTTON_PIN) == NOT_PRESSED){
        buttonFlag = BUTTON_PRESSED;
        buttonState = BUTTON_STATE_READ;
      }
      break;
  }
}

// hacemos sonar l timbre por medio segundo
void ringDoorbell(){
  if (millis() - doorbellTimer < 500){
    digitalWrite(DOORBELL_PIN, HIGH);
  }else if (millis() - doorbellTimer > 500){
    digitalWrite(DOORBELL_PIN, LOW);
  }
}
