#pragma once // pour le décarer 1 fois meme si appelé plusieurs fois
#include <EEPROM.h>

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

void writeEEPROMint(int address, int value);
int readEEPROMint(int address);
void initializeEEPROMDefaults();
