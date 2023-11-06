#ifndef __Leds_h__
#define __Leds_h__

#include "Arduino.h"

class Leds {
public:
  Leds(byte redPin, byte greenPin, byte bluePin);
  void setColorRGB(byte red, byte green, byte blue);

private:
  byte _redPin, _greenPin, _bluePin;
};

#endif
