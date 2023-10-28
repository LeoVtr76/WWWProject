#include <ChainableLED.h>
#include <TimerOne.h>
#include <RTClib.h>
#include <Wire.h>
#include <SdFat.h>
#include <EEPROM.h>
#include <Adafruit_BME280.h>
//#include <SPI.h>

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


#define NUM_LEDS 1
#define RED_BUTTON 2
#define GREEN_BUTTON 3
#define BUTTON_CHECK_INTERVAL 100000  // 100ms in microseconds
#define LIGHT_SENSOR_PIN A0
enum Mode {CONFIG, STANDARD, ECO, MAINTENANCE};
ChainableLED leds(5, 6, NUM_LEDS);
Adafruit_BME280 bme;
RTC_DS1307 clock;
SdFat SD;
byte recordCounter = 1;
unsigned long lastGetTime = 0, buttonPressTime = 0;
Mode currentMode;
Mode lastMode;
volatile bool isButtonPressed = false;
byte logInterval;
bool errorFlag = false;

//Prototype
void writeEEPROMint(int address, int value);
int readEEPROMint(int address);
void initializeEEPROMDefaults();
void ecoMode();
void standardMode();
void configMode();
void maintenanceMode();
void buttonPressed();
void checkButton();
void handleSerialCommand(String command);
void resetToDefaults();
void saveDataToSD();
void flashLedError(int red, int green, int blue, int duration1, int duration2);
void checkError();
void calculateDate(int* day, int* month, int* year);


void setup() {
    Serial.begin(9600);
    if (readEEPROMint(ADDR_LOG_INTERVAL_VALUE) == 0xFF) {
        initializeEEPROMDefaults();
        Serial.println("EEPROM value initiate");
    }

    pinMode(RED_BUTTON, INPUT_PULLUP);
    pinMode(GREEN_BUTTON, INPUT_PULLUP);
    
    while (!Serial && millis() > 5000);
    if (!clock.begin()) {
        Serial.println("Couldn't find RTC");
    } else {
        if (!clock.isrunning()) {
            clock.adjust(DateTime(2023, 10, 23, 11, 48, 30));
        }
        //printDateTime();
    }
    currentMode = !digitalRead(RED_BUTTON) ? CONFIG  : STANDARD;
    attachInterrupt(digitalPinToInterrupt(RED_BUTTON), buttonPressed, FALLING);
    attachInterrupt(digitalPinToInterrupt(GREEN_BUTTON), buttonPressed, FALLING);
    Timer1.initialize(BUTTON_CHECK_INTERVAL);
    Timer1.attachInterrupt(checkButton);
}

void loop() {
    checkError();
    if(currentMode != MAINTENANCE && currentMode != CONFIG){
        if (millis() - lastGetTime >= logInterval * 100) { //REMETTRE *60000 pour mettre des minutes en entrées
            saveDataToSD();
            lastGetTime = millis();
        }
    }

    switch (currentMode) {
        case CONFIG: configMode(); break;
        case STANDARD: standardMode(); break;
        case ECO: ecoMode(); break;
        case MAINTENANCE: maintenanceMode(); break;
    }
}
void buttonPressed() {
  buttonPressTime = millis();
  isButtonPressed = true;
  if (currentMode == ECO && digitalRead(GREEN_BUTTON) == LOW) currentMode = STANDARD;
}

void checkButton() {
    if (isButtonPressed && (millis() - buttonPressTime >= 5000)) {
        isButtonPressed = false;
        if (currentMode == MAINTENANCE && digitalRead(RED_BUTTON) == LOW) currentMode = lastMode;
        else if (digitalRead(RED_BUTTON) == LOW) {
            lastMode = currentMode;
            currentMode = MAINTENANCE;
        } else if (digitalRead(GREEN_BUTTON) == LOW && currentMode == STANDARD) {
            currentMode = ECO;
        }
    }
}


//gestion des modes
void ecoMode() {
    if(!errorFlag)leds.setColorRGB(0, 0, 0, 255);
    logInterval = readEEPROMint(ADDR_LOG_INTERVAL_VALUE);
    logInterval *= 2;
}

void standardMode() {
    if(!errorFlag)leds.setColorRGB(0, 0, 255, 0);
    logInterval = readEEPROMint(ADDR_LOG_INTERVAL_VALUE);
    // Enregistre données sur carte SD jusqu'à ce que le fichier soit full (2ko), il écrit sur un nouveau fichier
    // Pas compris la 3e instruction
}

void configMode() {
    if (!errorFlag) leds.setColorRGB(0, 255, 110, 0);
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        handleSerialCommand(command);
    }
    Serial.println("config");
    if (millis() >= 10000) currentMode = STANDARD; // Pour 30 min : 1800000
    // Modifier paramètres EEPROM ?
    // Formatter disque dur ? en 4k TUA
    // Réinitialiser paramètres
    // Interface pour taper des commandes
    // Affiche version + numéro de lot
}

void maintenanceMode() {
    if(!errorFlag)leds.setColorRGB(0, 255, 30, 0);
    // Pas de sauvegarde de carte sd
    // récup données via port série
    // permet de changer la carte sd
}

//gestion des capteurs et des erreurs
void saveDataToSD() {
    Serial.println("Analyse");
    int day, month, year;
    calculateDate(&day, &month, &year);
    char filename[15];
    SdFile dataFile;
    bool fileOpened = false;
    unsigned long startTime;
    int lightLevel;
    float voltage;

    while (!fileOpened) {
        snprintf(filename, sizeof(filename), "%02d%02d%02d_%d.log", year, month, day, recordCounter);
        if(dataFile.open(filename, O_READ)) {
            int fileSize = dataFile.fileSize();
            dataFile.close();
            
            if(fileSize <= (2048 - 100)) { // 100 octets pour la nouvelle entrée (estimation)
                fileOpened = true;
            } else {
                recordCounter++; // passez au fichier suivant
            }
        } else {
            // le fichier n'existe pas, alors créez-le
            fileOpened = true;
        }
    }
    startTime = millis();
    float temperature, humidity, pressure;
    while ((millis() - startTime <= 3000)){
        temperature = bme.readTemperature();
        humidity = bme.readHumidity();
        pressure = bme.readPressure();
        lightLevel = analogRead(LIGHT_SENSOR_PIN);
        voltage = lightLevel * (5.0/1023.0);
        if(voltage > 0 && bme.begin(0x76)) break;
        delay(100);
    }
    if(millis() - startTime > 30000){
        if(!bme.begin(0x76)) temperature = humidity = pressure = NAN;
        Serial.println(pressure);
        if(voltage == 0){
            lightLevel = -1;
            voltage = NAN;
        }
    }
    Serial.print("Temperature: "); Serial.print(isnan(temperature) ? "Na" : String(temperature)); Serial.println("C");
    Serial.print("Humidity: "); Serial.print(isnan(humidity) ? "Na" : String(humidity)); Serial.println("%");
   Serial.print("Pressure: "); Serial.print(isnan(pressure) ? "Na" : String(pressure)); Serial.println("Pa");
    Serial.print("Lecture brute : "); Serial.print(lightLevel == -1 ? "Na" : String(lightLevel));

    if(dataFile.open(filename, O_WRITE | O_CREAT | O_AT_END)) {
        dataFile.print(clock.now().hour()); dataFile.print(":"); dataFile.print(clock.now().minute()); dataFile.print(" -> ");
        dataFile.print("Temperature: "); dataFile.print(isnan(temperature) ? "Na" : String(temperature)); dataFile.print("C; ");
        dataFile.print("Humidity: "); dataFile.print(isnan(humidity) ? "Na" : String(humidity)); dataFile.print("%; ");
        dataFile.print("Pressure: "); dataFile.print(isnan(pressure) ? "Na" : String(pressure)); dataFile.print("Pa; ");
        dataFile.print("Light Level: "); dataFile.print(lightLevel == -1 ? "Na" : String(lightLevel)); dataFile.println("; ");
        dataFile.close();
        
    } else {
        Serial.print("error opening "); Serial.println(filename);
    }
}

// void printDateTime() {
//     DateTime now = clock.now();
//     Serial.print(now.hour());
//     Serial.print(":");
//     Serial.print(now.minute());
//     Serial.print(":");
//     Serial.print(now.second());
//     Serial.print(" ");
//     Serial.print(now.month());
//     Serial.print("/");
//     Serial.print(now.day());
//     Serial.print("/");
//     Serial.println(now.year());
// }

void flashLedError(int red, int green, int blue, int duration1, int duration2) {
    for (byte i = 0; i < 5; i++) {
        leds.setColorRGB(0, red, green, blue);  
        delay(duration1 * 500);  
        leds.setColorRGB(0, 255, 0, 0);  // Rouge
        delay(duration2 * 500);
    }
    leds.setColorRGB(0, 0, 0, 0);  // Éteindre la LED après la séquence
}

void checkError() {
    errorFlag = false;
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
    if (!bme.begin(0x76)){
        flashLedError(0,255,255,1,1);
        Serial.println("BME dont work chef");
        errorFlag = true;
    }
    //Serial.println(SD.card()->cardSize());
    //  if (SD.vol()->sectorsPerCluster() *512L * SD.vol()->freeClusterCount() < 7 * 1024 * 1024 * 1024){
    //     Serial.println("Chef ya plus de place");
    //     flashLedError(255,255,255,1,1);
    //     errorFlag = true;
    //  }

    //if(!errorFlag)changeMode(currentMode);
}

void calculateDate(int* day, int* month, int* year) {
    DateTime now = clock.now();
    *day = now.day();
    *month = now.month();
    *year = now.year() % 100;
}

//gestion des commandes

void handleSerialCommand(String command) {
    if (command.startsWith("LOG_INTERVAL=")) {
        int logInterval = command.substring(13).toInt();
        writeEEPROMint(ADDR_LOG_INTERVAL_VALUE, logInterval);
        Serial.println("LOG_INTERVAL set to " + String(logInterval));
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
//gestion EEPROM
void resetToDefaults() {
    writeEEPROMint(ADDR_LOG_INTERVAL_VALUE, 10);
    writeEEPROMint(ADDR_FILE_MAX_SIZE_VALUE, 4096);
    writeEEPROMint(ADDR_TIMEOUT_VALUE, 30);
}


void writeEEPROMint(int address, int value) {
    EEPROM.put(address, value);
}

int readEEPROMint(int address) {
    int value;
    EEPROM.get(address, value);
    return value;
}
void initializeEEPROMDefaults() {
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
