#include <ChainableLED.h>
#include <TimerOne.h>
#include <RTClib.h>
#include "Seeed_BME280.h"
#include <Wire.h>
#include <SdFat.h>
#include <SPI.h>
#include <EEPROM.h>

#include "modes.h"
#include "commands.h"
#include "eeprom_manager.h"
#include "sensors.h"

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
unsigned long lastRecordTime = 0, configStartTime = 0, lastGetTime = 0, buttonPressTime = 0;
Mode currentMode;
Mode lastMode;
volatile bool isButtonPressed = false;
int logInterval = 30;

void setup() {
    Serial.begin(9600);
    if (readEEPROMint(ADDR_LOG_INTERVAL_VALUE) == 0xFF) {
        initializeEEPROMDefaults();
    }

    pinMode(RED_BUTTON, INPUT_PULLUP);
    pinMode(GREEN_BUTTON, INPUT_PULLUP);
    leds.setColorRGB(0, 0, 0, 0);
    
    while (!Serial && millis() > 5000);
    
    if (!clock.begin()) {
        Serial.println("Couldn't find RTC");
    } else {
        if (!clock.isrunning()) {
            clock.adjust(DateTime(2023, 10, 23, 11, 48, 30));
        }
        printDateTime();
    }

    changeMode(!digitalRead(RED_BUTTON) ? CONFIG : STANDARD);
    attachInterrupt(digitalPinToInterrupt(RED_BUTTON), buttonPressed, FALLING);
    attachInterrupt(digitalPinToInterrupt(GREEN_BUTTON), buttonPressed, FALLING);
    Timer1.initialize(BUTTON_CHECK_INTERVAL);
    Timer1.attachInterrupt(checkButton);
}

void loop() {
    checkError();

    if (millis() - lastRecordTime >= 4800000) {
        saveDataToSD();  // 80 minutes : 4800000
    }
    
    if (millis() - lastGetTime >= logInterval * 60000) {
        readAndPrintSensors();
    }

    switch (currentMode) {
        case CONFIG: configMode(); break;
        case STANDARD: standardMode(); break;
        case ECO: ecoMode(); break;
        case MAINTENANCE: maintenanceMode(); break;
    }
}

