// #include <TimerOne.h>
// #include <ChainableLED.h>

// #define NUM_LEDS 1
// ChainableLED leds(5, 6, NUM_LEDS);
// const int buttonPin = 2;
// volatile unsigned long buttonPressTime = 0;
// volatile bool buttonPressed = false;

// void button_ISR() {
//   if (digitalRead(buttonPin) == LOW) {  // Si le bouton est pressé
//     if (buttonPressTime == 0) {  // Si c'est le début de la pression
//       buttonPressTime = millis();
//     }
//   } else {  // Si le bouton est relâché
//     buttonPressTime = 0;
//   }
// }

// void checkButton() {
//   if (buttonPressTime > 0 && (millis() - buttonPressTime >= 5000)) {
//     leds.setColorRGB(0, 0, 0, 255);  // Allumer la LED
//     buttonPressTime = 0;  // Réinitialiser le temps de pression
//   }
// }

// void setup() {
//   pinMode(buttonPin, INPUT_PULLUP);
//   leds.setColorRGB(0, 0, 0, 0);  // Éteindre la LED au démarrage
//   attachInterrupt(digitalPinToInterrupt(buttonPin), button_ISR, CHANGE);  // Attacher l'interruption
//   Timer1.initialize(100000);  // Configurer le timer pour vérifier le bouton toutes les 100 ms
//   Timer1.attachInterrupt(checkButton);  // Attacher la fonction checkButton au timer
// }

// void loop() {
//   delay(10000);
//   leds.setColorRGB(0, 0, 0, 0);
//   delay(10000);
// }
