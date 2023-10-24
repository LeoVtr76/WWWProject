#include <ChainableLED.h>
#include <TimerOne.h>
#include <RTClib.h>
#include "Seeed_BME280.h"
#include <Wire.h>
#include <SdFat.h>
#include <SPI.h>
#include <EEPROM.h>

#define NUM_LEDS 1
#define RED_BUTTON 2
#define GREEN_BUTTON 3
#define LIGHT_SENSOR_PIN A0
#define BUTTON_CHECK_INTERVAL 100000  // 100ms in microseconds


//EEPROM 
#define ADDR_LOG_INTERVAL 0
#define ADDR_FILE_MAX_SIZE 4 
#define ADDR_TIMEOUT 8
#define ADDR_LUMIN 12
#define ADDR_LUMIN_LOW 14
#define ADDR_LUMIN_HIGH 16
#define ADDR_TEMP_AIR 18
#define ADDR_MIN_TEMP_AIR 20
#define ADDR_MAX_TEMP_AIR 22
#define ADDR_HYGR 24
#define ADDR_HYGR_MINT 26
#define ADDR_HYGR_MAXT 28
#define ADDR_PRESSURE 30
#define ADDR_PRESSURE_MIN 32
#define ADDR_PRESSURE_MAX 34
#define ADDR_LOG_INTERVAL_VALUE 36
#define ADDR_FILE_MAX_SIZE_VALUE 40
#define ADDR_TIMEOUT_VALUE 44

ChainableLED leds(5, 6, NUM_LEDS);
BME280 bme280;
RTC_DS1307 clock;
SdFat SD;
int recordCounter = 1;
unsigned long lastRecordTime = 0, configStartTime = 0, lastGetTime = 0, buttonPressTime = 0;
enum Mode {CONFIG, STANDARD, ECO, MAINTENANCE};
Mode currentMode;
Mode lastMode;
volatile bool isButtonPressed = false;
int logInterval = 30;

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
void flashLedError(int red, int green, int blue, int duration1, int duration2);
void checkError();
void printDateTime();
void handleSerialCommand(String command);
void resetToDefaults();
void writeEEPROMint(int address, int value);
int readEEPROMint(int address);

void setup() {
  Serial.begin(9600);
  if (EEPROM.read(ADDR_LOG_INTERVAL) == 0xFF) { 
    writeEEPROMint(ADDR_LUMIN, 1);
    writeEEPROMint(ADDR_LUMIN_LOW, 255);
    writeEEPROMint(ADDR_LUMIN_HIGH, 768);
    writeEEPROMint(ADDR_TEMP_AIR, 1);
    writeEEPROMint(ADDR_MIN_TEMP_AIR, -10);
    writeEEPROMint(ADDR_MAX_TEMP_AIR, 60);
    writeEEPROMint(ADDR_HYGR, 1);
    writeEEPROMint(ADDR_HYGR_MINT, 0);
    writeEEPROMint(ADDR_HYGR_MAXT, 50);
    writeEEPROMint(ADDR_PRESSURE, 1);
    writeEEPROMint(ADDR_PRESSURE_MIN, 850);
    writeEEPROMint(ADDR_PRESSURE_MAX, 1080);
    writeEEPROMint(ADDR_LOG_INTERVAL_VALUE, 10);
    writeEEPROMint(ADDR_FILE_MAX_SIZE_VALUE, 4096);
    writeEEPROMint(ADDR_TIMEOUT_VALUE, 30);
  }

  pinMode(RED_BUTTON, INPUT_PULLUP);
  pinMode(GREEN_BUTTON, INPUT_PULLUP);
  leds.setColorRGB(0, 0, 0, 0);
  while (!Serial && millis() > 5000);
  while (!clock.begin()) Serial.println("Couldn't find RTC");

  if (!clock.isrunning()) clock.adjust(DateTime(2023, 10, 23, 11, 48, 30));
  printDateTime();
  changeMode(!digitalRead(RED_BUTTON) ? CONFIG : STANDARD);
  attachInterrupt(digitalPinToInterrupt(RED_BUTTON), buttonPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(GREEN_BUTTON), buttonPressed, FALLING);
  Timer1.initialize(BUTTON_CHECK_INTERVAL);
  Timer1.attachInterrupt(checkButton);
}

void loop() {
  checkError();
  if (millis() - lastRecordTime >= 4800000) saveDataToSD();  // 80 minutes : 4800000
    
  if(millis() - lastGetTime >= logInterval * 60000)readAndPrintSensors();

  switch(currentMode){
    case CONFIG : configMode(); break;
    case STANDARD : standardMode(); break;
    case ECO : ecoMode(); break;
    case MAINTENANCE : maintenanceMode(); break;
  }
}
void writeEEPROMint(int address, int value) {
    EEPROM.put(address, value);
}

int readEEPROMint(int address) {
    int value;
    EEPROM.get(address, value);
    return value;
}

void checkError(){
  bool errorFlag = false;
  if(!bme280.init()){
    Serial.println("Device error!");
    flashLedError(255, 0, 0, 1, 1);
    errorFlag = true;
  } 
  if(!clock.begin()){
    Serial.println("Couldn't find RTC");
    flashLedError(255, 0, 255, 1, 1);
    errorFlag = true;
  }
  if(!SD.begin(4)){
    flashLedError(255, 255, 255, 2, 1);
    Serial.println("Card failed or not present");
    errorFlag = true;
  }
  // if (carte sd pleine){
  //    flashLedError(255,255,255,1,1);
  //    errorFlag = true;
  // }
  float temperature = bme280.getTemperature();
  if (temperature < -50 || temperature > 50) { 
    Serial.println("Inconsistent temperature data!");
    flashLedError(255, 0, 0, 2, 1);
  }
  if (!errorFlag) changeMode(currentMode);
  
}
void printDateTime(){
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
        dataFile.sync();
        Serial.print("Data written to "); Serial.println(filename);
        
        recordCounter++;
        lastRecordTime = millis();
    } else {
        Serial.print("error opening "); Serial.println(filename);
    }
}


void flashLedError(int red, int green, int blue, int duration1, int duration2) {
  for (int i = 0; i < 5; i++) {  // 5 fois pour une durée totale d'environ 10 secondes
    leds.setColorRGB(0, red, green, blue);  
    delay(duration1 * 500);  
    leds.setColorRGB(0, 255, 0, 0);  // Rouge
    delay(duration2 * 500);
  }
  leds.setColorRGB(0, 0, 0, 0);  // Éteindre la LED après la séquence
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
  logInterval = logInterval * 2;
  //temps recup gps x 2
}
void standardMode(){
  logInterval = 10;
  //Enregistre données sur carte SD jusqu'a ce que le fichier soit full (2ko), il ecrit sur un nouveau fichier
  //Pas compris la 3e instruction
}
void configMode(){
  if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        handleSerialCommand(command);
    }
  //Modifier parametres EEPROM ?
  //Formatter disque dur ? en 4k TUA
  //Reinitialiser paramètres
  //Interface pour taper des commandes
  //Affiche version + numéro de lot

}
void handleSerialCommand(String command) {
    if (command.startsWith("LOG_INTERVALL=")) {
        logInterval = command.substring(13).toInt();
        writeEEPROMint(ADDR_LOG_INTERVAL_VALUE, logInterval);
        Serial.println("LOG_INTERVALL set to " + String(logInterval));
    }
    else if (command.startsWith("FILE_MAX_SIZE=")) {
        int fileSize = command.substring(13).toInt();
        writeEEPROMint(ADDR_FILE_MAX_SIZE_VALUE, fileSize);
        Serial.println("FILE_MAX_SIZE set to " + String(fileSize));
    }
    else if (command.startsWith("TIMEOUT=")) {
        int timeout = command.substring(8).toInt();
        writeEEPROMint(ADDR_TIMEOUT_VALUE, timeout);
        Serial.println("TIMEOUT set to " + String(timeout));
    }
    else if (command == "RESET") {
        resetToDefaults();
        Serial.println("All parameters reset to default values.");
    }
    else if (command == "VERSION") {
        Serial.println("Version: 1.0.0, Lot: 12345");
    }
    else {
        Serial.println("Unknown command.");
    }
}
void resetToDefaults() {
    writeEEPROMint(ADDR_LOG_INTERVAL_VALUE, 10);
    writeEEPROMint(ADDR_FILE_MAX_SIZE_VALUE, 4096);
    writeEEPROMint(ADDR_TIMEOUT_VALUE, 30);
    //... (réinitialisez d'autres valeurs par défaut si nécessaire)
}

void maintenanceMode(){
  //Pas de sauvegarde de carte sd
  //recup données via port série
  // permet de changer la carte sd
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
