// Dete Test 1
// 29.12.21
// Vers. 0.2 , IGE 12.03.22 

// Board ESP32 Dev Module
// https://www.digikey.de/en/maker/projects/adafruit-data-logger-shield/997ad0fc7f894310b90a82d325a2e0f8
// Pin Belegung S. 4 https://www.fambach.net/wemos-d1-r32-esp32/

// Access-Point : https://randomnerdtutorials.com/esp32-access-point-ap-web-server/ 
// https://randomnerdtutorials.com/esp32-web-server-sent-events-sse/
// https://circuits4you.com/2018/11/20/web-server-on-esp32-how-to-update-and-display-sensor-values/

// Include the correct display library
 // For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SH1106Wire.h"
#include "SH1106.h"


// Wi-Fi library
#include <WiFi.h> 
#include <WebServer.h>
#include <ArduinoJson.h>
#include "index.h"  //Web page header file

// Temp
#include <DS18B20.h>

// SD
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// RTC
#include "RTClib.h"

// Include the UI lib
#include "OLEDDisplayUi.h"

// RTC
RTC_DS1307 rtc;

// Use the corresponding display class:
// Display Settings

const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = 21;
const int SDC_PIN = 22;

// Initialize the OLED display using Wire library
SH1106Wire  display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
const int x_pos = 3; // 3 Pixel vom linken Rand 
const int y_pos = 0; // 1. Zeile 
const int y_step = 12; // naechste Zeilen increment

// OneWire Temp
DS18B20 ds(26);
//uint8_t ds18_vorlauf[] = {40, 170, 170, 173, 24, 19, 2, 93}; // DETE
//uint8_t ds18_rlauf[]   = {40, 170, 177, 151, 24, 19, 2, 2,}; // DETE

uint8_t ds18_rlauf[]   = { 40, 64, 204, 118, 224, 1, 60, 215 };
uint8_t ds18_vorlauf[] = { 40, 41, 213, 118, 224, 1, 60, 13 };





uint8_t selected;
// Korrekturfaktoren, muessen noch genau bestimmt werden ! Vielfache von 0,0625 !
float temp_vorlauf_cor = 0.50;
float temp_rlauf_cor = -0.75;
float temp_vorlauf = 0.00;
float temp_rlauf = 0.00;


// Zaehler fuer Temp-Messung, nur alle 60 Sekunden messen
int counter = 60; // Auf 61 damit beim Start gleich gemessen wird , wird dann auf 0 gesetzt 
int counter_intervall = 59;

// File Setup
File dataFile;
char fileName[19];

// Wifi Setup
const char* ssid     = "Datalogger-D1";
const char* password = "start123";
String header;

// Set web server port number to 80
WebServer server(80);

void state() {
  Serial.println("/state (alive)");
  String s = MAIN_page; //Read HTML contents
  server.send(200, "text/html", s); //Send web page
}

void handleADC() {
  Serial.println("/handleADC (readADC)");
  char buf[150];
  char timeBuf[30];
  DateTime now = rtc.now();
  sprintf(timeBuf, "%.2d:%.2d:%.2d", now.hour(), now.minute(),now.second());
  DynamicJsonDocument doc(1024);
  doc["vorlauf"] = String(temp_vorlauf);
  doc["rlauf"] = String(temp_rlauf);
  doc["uhrzeit"]   = String(timeBuf);;
  serializeJson(doc, buf);
  Serial.println(buf);
  server.send(200, "application/json", buf);
}

void config_rest_server_routing() {
    server.on("/", HTTP_GET, []() {
        //server.send(200, "text/html",
        //<h1>Welcome to the ESP32 DataLogger-D1</h1>");
       // server.send_P(200, "text/html", index_html);     
       state();
    });
    server.on("/state", HTTP_GET, state);
    server.on("/readADC", handleADC);
}

void setup() {
  //Seriell - Debug aktivieren
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  /* Nur zum holen der Adresse eines neuen Fuehlers */
  while (ds.selectNext()) {
    uint8_t address[8];
    ds.getAddress(address);
    Serial.print("Get DS18B20 Adresses : ");  
    for (uint8_t i = 0; i < 8; i++) {
        Serial.print(" ");
        Serial.print(address[i]);
        Serial.print(",");
      }
    Serial.println();  
  }  
  
  /* Anzeige DS18B20 */
  Serial.print("DS18B20 Devices : ");
  Serial.println(ds.getNumberOfDevices());
  Serial.print("Power Mode: ");
  if (ds.getPowerMode()) {
    Serial.println("External");
  } else {
    Serial.println("Parasite");
  }
    
  // RTC Setup , RTC ansprechbar ? 
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  // Falls Batteriewechsel , Uhr neu stellen beim kompilieren. Sehr ungenau !!!!
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Initialising the UI will init the display too.
  display.init();

  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  
  display.clear();
  // Kann weg nur Demo !Oder als Startbildschirm anpassen
  drawFontFaceDemo();
  display.display();

  // SD-Card Setup
  // Karte vorhanden und zugreifbar ? 
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // FileNameSetup
  DateTime now = rtc.now();
  sprintf(fileName, "dat_%.2d_%.2d_%.2d_%.2d.csv", now.day(),now.month(),now.hour(), now.minute());
  dataFile = SD.open("/"+String(fileName), "w");
  if (!dataFile) {
     Serial.println("Error opening file for writing");
     return;
  }
  // Header schreiben
  dataFile.println("Timestamp;Date;VL;RL");
  dataFile.close();

  // Wifi-Setup
  Serial.print("Setting AP (Access Point)…");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  config_rest_server_routing();
  server.begin();
  delay(3000);
  // Start 
  
}

void loop() {
  server.handleClient();
  temp_vorlauf = readTemp(ds18_vorlauf) + temp_vorlauf_cor;
  temp_rlauf = readTemp(ds18_rlauf) + temp_rlauf_cor;
  counter++;
  if (counter > counter_intervall) {
    counter = 0 ;
    writeDataToFile(temp_vorlauf,temp_rlauf);
  }
  
  showDisplay(counter,temp_vorlauf,temp_rlauf);
  delay(1000);
      
}

// Oeffnet File im AppendMode und schreibt die Daten
// Format (csv) TimeStamp;Datum-Zeit;Vorlauf;Ruecklauf

void writeDataToFile(float vl, float rl) {
  static char buf[100];
  DateTime now = rtc.now();
  sprintf(buf, "%.0d;%.2d.%.2d_%.2d:%.2d;%5.2f;%5.2f", now.unixtime(),now.day(),now.month(),now.hour(), now.minute(),vl,rl);
  dataFile = SD.open("/"+String(fileName), "a");
  if (!dataFile) {
     Serial.println("Error opening file for writing");
     return;
  }
  int  bytesWritten = dataFile.println(String(buf));
  if (bytesWritten > 0) {
        Serial.print("Bytes written: ");
        Serial.println(bytesWritten);
        Serial.println(buf);
    } else {
        Serial.println("File write failed");
  }
  dataFile.close();
}

void drawFontFaceDemo() {
  // Font Demo1
  // create more fonts at http://oleddisplay.squix.ch/
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Hello world");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "Hello world");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 26, "Hello world");
}

// Auslesen der Tempfuehler 
float readTemp(uint8_t sensor[]) {
  selected = ds.select(sensor);
  if (selected) {
   return(ds.getTempC());
  }  
  return(-100.0);
}
/* Anzeige der aktuellen Temperaturen */
void showDisplay(int cnt, float vl, float rl) {
  DateTime now = rtc.now();
  static char buf[10];
  sprintf(buf, "%.2d:%.2d:%.2d", now.hour(), now.minute(), now.second());
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  //display.drawString(x_pos, y_pos, "Hello world :"+String(cnt));
  display.drawString(x_pos, y_pos, "Uhrzeit: "+String(buf));
  display.drawString(x_pos, y_pos+(1*y_step), "Vorlauf: "+String(vl));
  display.drawString(x_pos, (2*y_step), "Rlauf: "+String(rl));
  display.drawString(x_pos, (3*y_step), "File:"+ String(fileName));
  display.display();
}




  
