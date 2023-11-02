#include <ChainableLED.h>
#include <TimerOne.h>
#include <RTClib.h>
#include <Wire.h>
// #include <SdFat.h>
//#include <SDI>
#include <SD.h>
#include <EEPROM.h>
#include <Adafruit_BME280.h>

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
//SdFat SD;
SdVolume volume;
Sd2Card card;
byte recordCounter = 1;
unsigned long lastGetTime = 0, buttonPressTime = 0;
Mode currentMode;
Mode lastMode;
volatile bool isButtonPressed = false;
byte logInterval;
bool errorFlag = false;

//Prototype
void changeMode(Mode mode);
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
void printDateTime();


void setup() {
    Serial.begin(9600);
    if (readEEPROMint(ADDR_LOG_INTERVAL_VALUE) == 0xFF) {
        initializeEEPROMDefaults();
        //Serial.println(F("EEPROM value initiate"));
    }
    pinMode(RED_BUTTON, INPUT_PULLUP);
    pinMode(GREEN_BUTTON, INPUT_PULLUP);
    pinMode(LIGHT_SENSOR_PIN, INPUT);
    while (!Serial && millis() > 5000);
    if (!clock.begin()) {
        //Serial.println(F("Couldn't find RTC"));
    } else {
        if (!clock.isrunning()) {
            clock.adjust(DateTime(2023, 10, 23, 11, 48, 30));
        }
        printDateTime();
    }
    currentMode = !digitalRead(RED_BUTTON) ? CONFIG  : STANDARD;
    attachInterrupt(digitalPinToInterrupt(RED_BUTTON), buttonPressed, FALLING);
    attachInterrupt(digitalPinToInterrupt(GREEN_BUTTON), buttonPressed, FALLING);
    Timer1.initialize(BUTTON_CHECK_INTERVAL);
    Timer1.attachInterrupt(checkButton);
}
void printDateTime(){
    Serial.print(clock.now().day());
    Serial.print(F("/"));
    Serial.print(clock.now().month());
    Serial.print(F("/"));
    Serial.print(clock.now().year());
    Serial.print(F(" "));
    Serial.print(clock.now().hour());
    Serial.print(F(":"));
    Serial.print(clock.now().minute());
}
void loop() {
    switch (currentMode) {
        case CONFIG: configMode(); break;
        case STANDARD: standardMode(); break;
        case ECO: ecoMode(); break;
        case MAINTENANCE: maintenanceMode(); break;
    }
    checkError();
    if(currentMode != MAINTENANCE && currentMode != CONFIG){
        if (millis() - lastGetTime >= logInterval * 100) { //REMETTRE *60000 pour mettre des minutes en entrées
            saveDataToSD();
            lastGetTime = millis();
        }
    }
}
void buttonPressed() {
  buttonPressTime = millis();
  isButtonPressed = true;
  if (currentMode == ECO && digitalRead(GREEN_BUTTON) == LOW) changeMode(STANDARD);
}

void checkButton() {
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
    if (millis() >= 1800000) changeMode(STANDARD); // Pour 30 min : 1800000
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
    bool lumin = (readEEPROMint(ADDR_LUMIN));
    int day, month, year;
    calculateDate(&day, &month, &year);
    char filename[15];
    File dataFile;
    bool fileOpened = false;
    unsigned long startTime;
    int lightLevel;
    float voltage;
    String luminosityDescription;

    while (!fileOpened) {
        snprintf(filename, sizeof(filename), "%02d%02d%02d_%d.log", year, month, day, recordCounter);
        if (SD.exists(filename)) {
            dataFile = SD.open(filename, FILE_READ);
            int fileSize = dataFile.size();
            dataFile.close();
            
            if(fileSize <= (2048 - 100)) {
                fileOpened = true;
            } else {
                recordCounter++;
            }
        } else {
            fileOpened = true;
        }
    }
    startTime = millis();
    float temperature, humidity, pressure;
    while ((millis() - startTime <= 3000)){
        temperature = bme.readTemperature();
        humidity = bme.readHumidity();
        pressure = bme.readPressure();
        if(lumin){
            lightLevel = analogRead(LIGHT_SENSOR_PIN);
            voltage = lightLevel * (5.0/1023.0);
            if(voltage < 1000 && bme.begin(0x76)) break;
            delay(100);
        }
        else{
            lightLevel = -1;
            voltage = NAN;
        }

    }
    if(!bme.begin(0x76)) temperature = humidity = pressure = NAN;
    if(lumin){
        if(lightLevel > 1000){
        lightLevel = -1;
        voltage = NAN;
        }
        int luminLow = readEEPROMint(ADDR_LUMIN_LOW);
        int luminHigh = readEEPROMint(ADDR_LUMIN_HIGH);
        if (lightLevel < luminLow && lightLevel >= 0) {
            luminosityDescription = F("faible");
        } else if(lightLevel > luminHigh && lightLevel < 1000) {
            luminosityDescription = F("forte");
        }else {
            luminosityDescription = String(lightLevel);
        }
    }
    else {
        luminosityDescription = F("Na");
    }
    

    Serial.print(F("Temperature: ")); Serial.print(isnan(temperature) ? "Na" : String(temperature)); Serial.println("C");
    Serial.print(F("Humidity: ")); Serial.print(isnan(humidity) ? "Na" : String(humidity)); Serial.println("%");
    Serial.print(F("Pressure: ")); Serial.print(isnan(pressure) ? "Na" : String(pressure)); Serial.println("Pa");
    Serial.print(F("Luminosity: ")); Serial.println(luminosityDescription);
    dataFile = SD.open(filename, FILE_WRITE);
    if (dataFile) {
        dataFile.print(clock.now().hour()); dataFile.print(":"); dataFile.print(clock.now().minute()); dataFile.print(" -> ");
        dataFile.print(F("Temperature: ")); dataFile.print(isnan(temperature) ? "Na" : String(temperature)); dataFile.print("C; ");
        dataFile.print(F("Humidity: ")); dataFile.print(isnan(humidity) ? "Na" : String(humidity)); dataFile.print("%; ");
        dataFile.print(F("Pressure: ")); dataFile.print(isnan(pressure) ? "Na" : String(pressure)); dataFile.print("Pa; ");
        dataFile.print(F("Luminosity: ")); dataFile.println(luminosityDescription);
        Serial.println(F("Data written"));
        dataFile.close(); 
    } else {
        //Serial.print(F("error opening ")); Serial.println(filename);
    }
}

void flashLedError(int red, int green, int blue, int duration1, int duration2) {
    for (byte i = 0; i < 5; i++) {
        leds.setColorRGB(0, red, green, blue);  
        delay(duration1 * 500);  
        leds.setColorRGB(0, 255, 0, 0);  // Rouge
        delay(duration2 * 500);
    }
    leds.setColorRGB(0, 0, 0, 0);  // Éteindre la LED après la séquence
}
void changeMode(Mode newMode) {
  currentMode = newMode;
  switch (currentMode) {
    case CONFIG: leds.setColorRGB(0, 255, 110, 0); break;
    case STANDARD: leds.setColorRGB(0, 0, 255, 0); break;
    case ECO: leds.setColorRGB(0, 0, 0, 255); break;
    case MAINTENANCE: leds.setColorRGB(0, 255, 30, 0); break;
  }
}
void checkError() {
    errorFlag = false;
    if(!clock.begin()){
        //Serial.println("Couldn't find RTC");
        flashLedError(255, 0, 255, 1, 1);
        errorFlag = true;
    }
    if(!SD.begin(4)){
        flashLedError(255, 255, 255, 2, 1);
        //Serial.println("Card failed or not present");
        errorFlag = true;
    }
    if (!bme.begin(0x76)){
        flashLedError(0,255,255,1,1);
        //Serial.println("BME dont work chef");
        errorFlag = true;
    }
    Serial.println(volume.blocksPerCluster()*volume.clusterCount() - volume.clusterCount() * 512);
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
        //Serial.println("LOG_INTERVAL set to " + String(logInterval));
    }
    else if (command.startsWith("LUMIN=")){
        int luminValue = command.substring(6).toInt();
        if(luminValue == 0 || luminValue == 1){
            writeEEPROMint(ADDR_LUMIN, luminValue);
        }
    }
    else if (command.startsWith("FILE_MAX_SIZE=")) {
        int fileSize = command.substring(13).toInt();
        writeEEPROMint(ADDR_FILE_MAX_SIZE_VALUE, fileSize);
        //Serial.println("FILE_MAX_SIZE set to " + String(fileSize));
    }
    else if (command.startsWith("TIMEOUT=")) {
        int timeout = command.substring(8).toInt();
        writeEEPROMint(ADDR_TIMEOUT_VALUE, timeout);
        //Serial.println("TIMEOUT set to " + String(timeout));
    }
    else if (command.startsWith("LUMIN_LOW=")){
        int lumin_low = command.substring(10).toInt();
        writeEEPROMint(ADDR_LUMIN_LOW, lumin_low);
        //Serial.println("LUMIN_LOW set to " + String(lumin_low));
    }
    else if (command.startsWith("LUMIN_HIGH=")){
        int lumin_high = command.substring(11).toInt();
        writeEEPROMint(ADDR_LUMIN_HIGH, lumin_high);
        //Serial.println("LUMIN_HIGH set to " + String(lumin_high));
    }
    else if (command.startsWith("CLOCK=")){
        String time = command.substring(6);
        int hour = time.substring(0,2).toInt();
        int min = time.substring(3,5).toInt();
        int sec = time.substring(6,8).toInt();
        int currentYear, currentMonth, currentDay;
        calculateDate(&currentDay, &currentMonth, &currentYear);
        clock.adjust(DateTime(currentYear+2000, currentMonth, currentDay, hour, min, sec));
    }
    else if (command.startsWith("DATE=")){
        String date = command.substring(5);
        int day = date.substring(0,2).toInt();
        int month = date.substring(3,5).toInt();
        int year = date.substring(6,10).toInt();
        int currentHour, currentMinute, currentSecond;
        currentHour = clock.now().hour();
        currentMinute = clock.now().minute();
        currentSecond = clock.now().second();
        clock.adjust(DateTime(year, month, day, currentHour, currentMinute, currentSecond));
    }
    else if (command == "RESET") {
        resetToDefaults();
        //Serial.println("All parameters reset to default values.");
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
    writeEEPROMint(ADDR_LUMIN_LOW, 200);
    writeEEPROMint(ADDR_LUMIN_HIGH, 700);
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