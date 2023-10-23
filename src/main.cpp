#include <ChainableLED.h>
#include <TimerOne.h>
#include <RTClib.h>
#include "Seeed_BME280.h"
#include <Wire.h>
#include <SdFat.h>
#include <SPI.h>

#define NUM_LEDS 1
#define RED_BUTTON 2
#define GREEN_BUTTON 3
#define LIGHT_SENSOR_PIN A0
#define BUTTON_CHECK_INTERVAL 100000  // 100ms in microseconds

ChainableLED leds(5, 6, NUM_LEDS);
BME280 bme280;
RTC_DS1307 clock;
SdFat SD;
int recordCounter = 1;
unsigned long lastRecordTime = 0;

enum Mode {CONFIG, STANDARD, ECO, MAINTENANCE};
Mode currentMode;
Mode lastMode;

volatile unsigned long buttonPressTime = 0;
volatile bool isButtonPressed = false;
unsigned long configStartTime = 0;
int interval = 30;
unsigned long lastGetTime = 0;
void changeMode(Mode newMode);
void buttonPressed();
void checkButton();
void calculateDate(int* day, int* month, int* year);
void saveDataToSD();
void readAndPrintSensors();
void ecoMode();
void standardMode();
void configMode();
void maintenanceMode();

void setup() {
  Serial.begin(9600);
  while (!Serial && millis() > 5000);

  while (!bme280.init()) Serial.println("Device error!");

  while (!clock.begin()) Serial.println("Couldn't find RTC");

  if (!clock.isrunning()) clock.adjust(DateTime(2023, 10, 23, 11, 48, 30));

  while(!SD.begin(4)) Serial.println("Card failed or not present");

  DateTime now = clock.now();
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print(" ");
  Serial.print(now.month());
  Serial.print("/");
  Serial.print(now.day());
  Serial.print("/");
  Serial.println(now.year());

  pinMode(RED_BUTTON, INPUT_PULLUP);
  pinMode(GREEN_BUTTON, INPUT_PULLUP);
  leds.setColorRGB(0, 0, 0, 0);

  changeMode(!digitalRead(RED_BUTTON) ? CONFIG : STANDARD);

  attachInterrupt(digitalPinToInterrupt(RED_BUTTON), buttonPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(GREEN_BUTTON), buttonPressed, FALLING);

  Timer1.initialize(BUTTON_CHECK_INTERVAL);
  Timer1.attachInterrupt(checkButton);
}

void loop() {
  
  if (millis() - lastRecordTime >= 5000) {  // 80 minutes : 4800000
    saveDataToSD();
  }
  if(millis() - lastGetTime >= interval * 1000){
      readAndPrintSensors();
  }

  switch(currentMode){
    case CONFIG :
      configMode();
      break;
    case STANDARD :
      standardMode();
      break;
    case ECO :
      ecoMode();
      break;
    case MAINTENANCE :
      maintenanceMode();
      break;
  }
}
void saveDataToSD() {
    int day, month, year;
    calculateDate(&day, &month, &year);
    char filename[15];
    snprintf(filename, sizeof(filename), "%02d%02d%02d_%d.log", year, month, day, recordCounter);
    
    SdFile dataFile;
    if (dataFile.open(filename, O_WRITE | O_CREAT | O_AT_END)) {
        dataFile.print("Temperature: "); dataFile.print(bme280.getTemperature()); dataFile.println("C");
        dataFile.print("Humidity: "); dataFile.print(bme280.getHumidity()); dataFile.println("%");
        dataFile.print("Pressure: "); dataFile.print(bme280.getPressure()); dataFile.println("Pa");
        dataFile.print("Light Level: "); dataFile.println(analogRead(LIGHT_SENSOR_PIN));
        dataFile.close();

        Serial.print("Data written to "); Serial.println(filename);
        
        recordCounter++;
        lastRecordTime = millis();
    } else {
        Serial.print("error opening "); Serial.println(filename);
    }
}

void readAndPrintSensors() {
    float temperature = bme280.getTemperature();
    float humidity = bme280.getHumidity();
    float pressure = bme280.getPressure();
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println("C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println("%");
    Serial.print("Pressure: "); Serial.print(pressure); Serial.println("Pa");

    int lightLevel = analogRead(LIGHT_SENSOR_PIN);
    float voltage = lightLevel * (5.0 / 1023.0);
    Serial.print("Lecture brute : "); Serial.print(lightLevel);
    Serial.print("\tTension : "); Serial.println(voltage);
    lastGetTime = millis();
}
void ecoMode(){

}
void standardMode(){

}
void configMode(){

}
void maintenanceMode(){
  
}

void buttonPressed() {
  buttonPressTime = millis();
  isButtonPressed = true;
  if (currentMode == ECO && digitalRead(GREEN_BUTTON) == LOW) changeMode(STANDARD);
}

void checkButton() {
    if (currentMode == CONFIG && (millis() - configStartTime >= 1800000)) changeMode(STANDARD);
    if (isButtonPressed && (millis() - buttonPressTime >= 5000)) {
        isButtonPressed = false;
        if (currentMode == MAINTENANCE && digitalRead(RED_BUTTON) == LOW) changeMode(lastMode);
        else if (digitalRead(RED_BUTTON) == LOW) {
            lastMode = currentMode;
            changeMode(MAINTENANCE);
        } else if (digitalRead(GREEN_BUTTON) == LOW && currentMode == STANDARD) {
            changeMode(ECO);
        }
    }
}

void changeMode(Mode newMode) {
  currentMode = newMode;
  if (currentMode == CONFIG) configStartTime = millis();
  switch (currentMode) {
    case CONFIG: leds.setColorRGB(0, 255, 110, 0); break;
    case STANDARD: leds.setColorRGB(0, 0, 255, 0); break;
    case ECO: leds.setColorRGB(0, 0, 0, 255); break;
    case MAINTENANCE: leds.setColorRGB(0, 255, 30, 0); break;
  }
}

void calculateDate(int* day, int* month, int* year) {
  DateTime now = clock.now();
  *day = now.day();
  *month = now.month();
  *year = now.year() % 100;
}
