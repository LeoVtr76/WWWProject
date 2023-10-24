#include "modes.h"
#include "commands.h"
#include "eeprom_manager.h"
#include "sensors.h"
#include "globals.h"

extern Mode currentMode;
extern unsigned long configStartTime; // Ajoutez cette ligne pour initialiser la variable

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

void ecoMode() {
    Serial.println("eco");
    int logInterval = readEEPROMint(ADDR_LOG_INTERVAL_VALUE);
    logInterval *= 2;
    writeEEPROMint(ADDR_LOG_INTERVAL_VALUE, logInterval);
    // temps recup gps x 2
}

void standardMode() {
    Serial.println("standard");
    int logInterval = 10;
    writeEEPROMint(ADDR_LOG_INTERVAL_VALUE, logInterval);
    // Enregistre données sur carte SD jusqu'à ce que le fichier soit full (2ko), il écrit sur un nouveau fichier
    // Pas compris la 3e instruction
}

void configMode() {
    Serial.println("config");
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        handleSerialCommand(command);
    }
    // Modifier paramètres EEPROM ?
    // Formatter disque dur ? en 4k TUA
    // Réinitialiser paramètres
    // Interface pour taper des commandes
    // Affiche version + numéro de lot
}

void maintenanceMode() {
    Serial.println("maintenance");
    // Pas de sauvegarde de carte sd
    // récup données via port série
    // permet de changer la carte sd
}
