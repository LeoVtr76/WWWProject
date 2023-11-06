#include "leds.h"

Leds::Leds(byte redPin, byte greenPin, byte bluePin) :
    _redPin(redPin), _greenPin(greenPin), _bluePin(bluePin) {
    pinMode(_redPin, OUTPUT);
    pinMode(_greenPin, OUTPUT);
    pinMode(_bluePin, OUTPUT);
}

void Leds::setColorRGB(byte red, byte green, byte blue) {
    analogWrite(_redPin, red);
    analogWrite(_greenPin, green);
    analogWrite(_bluePin, blue);
}