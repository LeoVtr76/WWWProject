#pragma once
#include "Seeed_BME280.h"

void readAndPrintSensors();
void saveDataToSD();
void printDateTime();
void flashLedError(int red, int green, int blue, int duration1, int duration2);
void checkError();
void calculateDate(int* day, int* month, int* year);
