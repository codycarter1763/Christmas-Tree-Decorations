#ifndef EFFECTS_H_
#define EFFECTS_H_

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN   4
#define LED_COUNT 100

// Defined in main.cpp, used by effects.cpp
extern Adafruit_NeoPixel strip;
extern uint32_t presetStartTime;

void fadeToBlack(int ledNo, byte fadeValue);
void meteorRain(byte red, byte green, byte blue, byte meteorSize,
                byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay);
void shootingStar();
void berryTree();
void Red();
void Blue();
void Green();
void lightGreen();
void White();
void warmWhite();
void candyCane();
void LSU();
void rainbow();
void SnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay);
void pastelSweep();
void colorSweep();
void retroBulbs();
void aurora();
void fireworks();

#endif
