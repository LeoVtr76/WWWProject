#include "eeprom_manager.h"

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
