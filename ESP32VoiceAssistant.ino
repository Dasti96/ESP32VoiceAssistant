#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "driver/gpio.h"
#include <Arduino.h>
#include <ESP_I2S.h>

#define I2S_SCK GPIO_NUM_9
#define I2S_WS GPIO_NUM_8
#define I2S_SD GPIO_NUM_10
#define I2S_OUT GPIO_NUM_3
#define INT_PIN 0

I2SClass i2s;
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define TEXT_SIZE 2
#define MAX_MIC_LISTENING 5000
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiClient client;
const char* ssid = "ssid";
const char* password = "password";

uint8_t bufferSpeaker[2048];
int32_t bufferMic[512];
int16_t pcm16[512];
static uint8_t silence[512] = {0};

volatile bool recordMic = false;
volatile uint32_t deltaTime = 0;
volatile uint32_t stopMicTime = 0;

volatile uint32_t lastPress;
volatile uint32_t currentPress;
volatile bool canPress = true;
volatile uint32_t startMicTime;

void setupI2S() { 
  i2s.setPins(I2S_SCK, I2S_WS, I2S_OUT, I2S_SD); 
  if (!i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
    Serial.println("Errore nell'inizializzazione dell'I2S!");
    while (1);
  }    
}

void setup() {
  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(INT_PIN, recordingInterrupt, FALLING);
  // put your setup code here, to run once:
  Serial.begin(115200);

  if(!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  oled.clearDisplay();
  oled.setTextSize(TEXT_SIZE); // You can select text size ex- 1, 2, 3 and so on
  oled.setTextColor(WHITE); // This will display Bright character on Black background
  oled.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());  

  while(!client.connected()){
    client.connect("192.168.1.102", 1234);
    delay(300);
  }
  Serial.println("Server connected");

  setupI2S();
  startMicTime = millis();
}


void loop() {  
  currentPress = millis();
  if(recordMic){
    stopMicTime = millis();
    deltaTime = stopMicTime - startMicTime;
  }

  if(client.available()){  
    //speaker
    int n = client.read(bufferSpeaker, sizeof(bufferSpeaker));
    Serial.printf("Ricevuti %d\n", n);
    size_t written;
    if (n > 0)
      written = i2s.write(bufferSpeaker, n);     
  }else
    i2s.write(silence, sizeof(silence));


  //microfono
    
    size_t byteRead;
    byteRead = i2s.readBytes((char*)bufferMic, sizeof(bufferMic));
    int nSamples = byteRead/sizeof(int32_t);
    for(int i = 0; i < nSamples; i++){
      Serial.print(3000);
      Serial.print("  ");
      Serial.print(-3000);
      Serial.print("  ");
      Serial.println(pcm16[i]);
      pcm16[i] = (int16_t)(bufferMic[i] >> 16);     
    }  

    size_t bytesToSend = nSamples * sizeof(int16_t);   
  if(recordMic && deltaTime < MAX_MIC_LISTENING){   
    size_t sent = client.write((uint8_t*)pcm16,bytesToSend);  
  }
} 


void recordingInterrupt(){ 
  
  if(currentPress - lastPress > 20){    
    canPress = true;   
    lastPress = currentPress;
  }
  else 
    canPress = false;

  if(canPress) {
    if(deltaTime >= MAX_MIC_LISTENING)
      recordMic = false;

    recordMic=!recordMic;   
  }
  
  if(recordMic)
    startMicTime = millis();
}

void oledPrint(String text){
  oled.setCursor(0, 16);

  oled.clearDisplay();

  oled.print(text);

  oled.display();  
}







