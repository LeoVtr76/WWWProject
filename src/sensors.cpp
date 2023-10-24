#include "sensors.h"
#include "modes.h"
#include "Seeed_BME280.h"
#include "eeprom_manager.h"
#include <SdFat.h>
#include <RTClib.h>

extern ChainableLED leds;
extern BME280 bme280;
extern RTC_DS1307 clock;
extern SdFat SD;
extern int recordCounter;
extern Mode currentMode;
extern const int LIGHT_SENSOR_PIN;

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
    } else {
        Serial.print("error opening "); Serial.println(filename);
    }
}

void printDateTime() {
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

void flashLedError(int red, int green, int blue, int duration1, int duration2) {
    for (int i = 0; i < 5; i++) {
        leds.setColorRGB(0, red, green, blue);  
        delay(duration1 * 500);  
        leds.setColorRGB(0, 255, 0, 0);  // Rouge
        delay(duration2 * 500);
    }
    leds.setColorRGB(0, 0, 0, 0);  // Éteindre la LED après la séquence
}

void checkError() {
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

void calculateDate(int* day, int* month, int* year) {
    DateTime now = clock.now();
    *day = now.day();
    *month = now.month();
    *year = now.year() % 100;
}
