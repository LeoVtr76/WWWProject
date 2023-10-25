#include "commands.h"
#include "eeprom_manager.h"

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

void resetToDefaults() {
    writeEEPROMint(ADDR_LOG_INTERVAL_VALUE, 10);
    writeEEPROMint(ADDR_FILE_MAX_SIZE_VALUE, 4096);
    writeEEPROMint(ADDR_TIMEOUT_VALUE, 30);
}
