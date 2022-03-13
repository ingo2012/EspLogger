/********** Multi-Temp-Logger 1.00 ********* 29.12.21
    I.Gerlach, D.Kleine

  WeMos D1 R32 ESP32
  Data logging shield v1.0
  DS18B20
  SH1106
  ArduinoBoard ESP32 WEMOS D1 MINI ESP32 Version 1.6.0 ESP-IDF version is: v3.3.5-1-g85c43024c


  Mehrfach Temperatur-Logger zur Temperaturaufzeichnug auf SD-Karte im CSV-Format.
  Fühler DS18B20 im Display SH1106.

  letzte Änderung:
  01.01.22 DK -SD einbindung

*/

// Include the correct library
// For a connection via I2C using Wire include
// #include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SH1106Wire.h>
#include <SH1106.h>
#include <OLEDDisplayUi.h>
#include "TimeLib.h"
#include <RTClib.h>
#include "DS18B20.h"
#include <FS.h>
#include <SD.h>
#include "SPI.h"


RTC_Millis rtc;
File logFile;

const int zeit = 10000; //Messintervall

long temperatureSensor01 ;
long temperatureSensor02 ;
long temperatureSensor03 ;

// Display Settings

const int I2C_DISPLAY_ADDRESS = 0x3c;
const int SDA_PIN = 21;
const int SDC_PIN = 22;

// Initialize the OLED display using Wire library
SH1106  display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
OLEDDisplayUi ui     ( &display );

// OneWire Temp
DS18B20 ds(26);
uint8_t ds18_sen01[] = {40, 170, 170, 173, 24, 19, 2, 93};
uint8_t ds18_sen02[] = {40, 170, 1,   4,   25, 19, 2, 143};
uint8_t ds18_sen03[] = {40, 170, 177, 151, 24, 19, 2, 2};
uint8_t selected;

float temp_sen01 = 0.0;
float temp_sen02 = 0.0;
float temp_sen03 = 0.0;

String timenow ;

// Nur Test  !!!
int cnt = 0;

//Display
void TitleFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->flipScreenVertically();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(5 + x, 20 + y, "DATA-LOGGER");
  //  display->drawString(30 + x, 34 + y, (timenow);
}
void drawFontFaceDemo1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Font Demo1
  // create more fonts at http://oleddisplay.squix.ch/
  //  display->flipScreenVertically();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0 + x, 10 + y, "Hello world");
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 20 + y, "Hello world");
  display->setFont(ArialMT_Plain_24);
  display->drawString(0 + x, 34 + y, "Hello world");
}
void TempFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0 + x, 5 + y, "Messung (10sec.) :" + String(cnt));
  display->drawString(0 + x, 17 + y, "Sensor 01: " + String(temp_sen01));
  display->drawString(0 + x, 31 + y, "Sensor 02: " + String(temp_sen02));
  display->drawString(0 + x, 43 + y, "Sensor 02: " + String(temp_sen03));
}

FrameCallback frames[] = {TitleFrame, drawFontFaceDemo1, TempFrame1};
int frameCount = 3;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));

  Serial.println();
  Serial.println();

  Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));              //v3.3.5-1-g85c43024c    ESP32 Ver. 1.0.6

  Serial.print("DS18B20 Devices : ");
  Serial.println(ds.getNumberOfDevices());

  // Initialising the UI will init the display too.

  ui.setTargetFPS(60);
  //  ui.setActiveSymbol(activeSymbol);
  //  ui.setInactiveSymbol(inactiveSymbol);
  //  ui.setOverlays(overlays, overlaysCount);
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, frameCount);
  ui.init();

  // SD-Card Setup

  if (!SD.begin()) {
    Serial.println("initialization SD failed!");
    while (1);
  }
  Serial.println("initialization SD done.");

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");                       //Test io
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);        //Test io

  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    Serial.println(filename);                               //Test io
    if (!SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logFile = SD.open(filename, FILE_WRITE);
      break;  // leave the loop!
    }
  }
  if (!logFile) {
    Serial.println("could not create file");
    Serial.println(logFile);                                //Test n.io --> 0
  }
  Serial.print("Start logging: ");
  Serial.println(filename);
  logFile.print ("Start logging");
  logFile.println();

}

float readTemp(uint8_t sensor[]) {
  selected = ds.select(sensor);
  if (selected) {
    return (ds.getTempC());
  }
  return(-1.0);
}

void loop() {

  int remainingTimeBudget = ui.update();

  cnt++;
  delay(zeit);

  temp_sen01 = readTemp(ds18_sen01);
  temp_sen02 = readTemp(ds18_sen02);
  temp_sen03 = readTemp(ds18_sen03);

  DateTime now = rtc.now();
  logFile.print(now.day(), DEC);
  logFile.print('/');
  logFile.print(now.month(), DEC);
  logFile.print('/');
  logFile.print(now.year(), DEC);
  logFile.print(',');
  logFile.print(' ');
  logFile.print(now.hour(), DEC);
  logFile.print(':');
  logFile.print(now.minute(), DEC);
  logFile.print(':');
  logFile.print(now.second(), DEC);
  logFile.print (",");
  logFile.print (" ");
  logFile.print (temp_sen01);
  logFile.print (",");
  logFile.print (" ");
  logFile.print (temp_sen02);
  logFile.print (",");
  logFile.print (" ");
  logFile.print (temp_sen03);
  logFile.println();
  logFile.flush();
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(',');
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.print (",");
  Serial.print (" ");
  Serial.print (temp_sen01);
  Serial.print (" Grad, Sensor 1");
  Serial.print (",");
  Serial.print (" ");
  Serial.print (temp_sen02);
  Serial.print (" Grad, Sensor 2");
  Serial.print (",");
  Serial.print (" ");
  Serial.print (temp_sen03);
  Serial.print (" Grad, Sensor 3");
  Serial.println();

}
