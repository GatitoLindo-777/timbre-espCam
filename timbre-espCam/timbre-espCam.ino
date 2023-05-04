// Incluimos las librerias
#include <WiFi.h> // WIFI para ESP32
#include <WiFiClientSecure.h> 
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

// credenciales de wifi
const char* ssid = "ORT-IoT";  // cambiar por red WIFI deseada
const char* password = "OrtIOTnew22$2";  // cambiar por la clave de WIFI

// Initialize Telegram BOT
#define BOTtoken "2057747784:AAGmapbuxScYVtSGqdl8LhQrsKdt1rbeaBs"  // cambiar por token 

#define CHAT_ID "-526457647" //  cambiar por el chat id del usuario 

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);


int botRequestDelay = 1000; /// intervalo de tiempo entre lectras del bot (en ms)
unsigned long lastTimeBotRan; /// ultimo vez que el bot leyo 

const int ledPin = 2; /// pin led 
bool ledState = LOW;

void setup() {
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);
  
  // Conecto el wifi Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
  Serial.println("Connecting to WiFi..");
  
  ///bloquea el programa si no se puede conectar a internet 
  while (WiFi.status() != WL_CONNECTED) {   
    delay(1000);
    Serial.println(".");
  }
  
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  bot.sendMessage(CHAT_ID, "Bot Hola mundo", "");
}

void loop() {

  if (millis() > lastTimeBotRan + botRequestDelay) { // si el tiempo que trancurrio es mayor al delay seteado leemos de vuelta
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("Veo los msj nuevos");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  
  }
}

// funcion que se comunica con telegram
void handleNewMessages(int numNewMessages) {
  Serial.println("Mensaje nuevo");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // inicio de verificacion
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){   ////si el id no corresponde da error . en caso de que no se quiera comprobar el id se debe sacar esta parte 
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    ///fin de verificacion

    /*// imprime el msj recibido 
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    /// si rebice /led on enciende el led 
    if (text == "/led_on") {  
      bot.sendMessage(chat_id, "LED state set to ON", "");
      ledState = HIGH;
      digitalWrite(ledPin, ledState);
    }
    
    if (text == "/led_off") {
      bot.sendMessage(chat_id, "LED state set to OFF", "");
      ledState = LOW;
      digitalWrite(ledPin, ledState);
    }
    
    if (text == "/state") {
      if (digitalRead(ledPin)){
        bot.sendMessage(chat_id, "LED is ON", "");
      }
      else{
        bot.sendMessage(chat_id, "LED is OFF", "");
      }
    }
  }*/


}
