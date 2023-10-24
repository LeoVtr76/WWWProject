#include "eeprom_manager.h"

void writeEEPROMint(int address, int value) {
    EEPROM.put(address, value);
}

int readEEPROMint(int address) {
    int value;
    EEPROM.get(address, value);
    return value;
}
