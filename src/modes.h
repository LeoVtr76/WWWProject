#pragma once
#include <ChainableLED.h>

enum Mode {CONFIG, STANDARD, ECO, MAINTENANCE};

void changeMode(Mode newMode);
void ecoMode();
void standardMode();
void configMode();
void maintenanceMode();
void buttonPressed();
void checkButton();
