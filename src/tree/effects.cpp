#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "effects.h"

// NOTE: color arguments below are in (G, R, B) order on purpose -- the original
// code passes green first to setPixelColor(). Kept as-is so colors look the same.

// Fades pixel to nothing over time, used in meteorRain
void fadeToBlack(int ledNo, byte fadeValue) {
  uint32_t oldColor;
  uint8_t r, g, b;

  oldColor = strip.getPixelColor(ledNo);
  r = (oldColor & 0x00ff0000UL) >> 16;
  g = (oldColor & 0x0000ff00UL) >> 8;
  b = (oldColor & 0x000000ffUL);

  r = (r <= 10) ? 0 : (int) r - (r * fadeValue / 256);
  g = (g <= 10) ? 0 : (int) g - (g * fadeValue / 256);
  b = (b <= 10) ? 0 : (int) b - (b * fadeValue / 256);

  strip.setPixelColor(ledNo, r, g, b);
}

// Meteor rain effect (non-blocking version of the classic Tweaking4All effect).
// Called every frame by the LED task; advances one step every SpeedDelay ms.
void meteorRain(byte red, byte green, byte blue, byte meteorSize,
                byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {
  static int      step       = 0;
  static uint32_t lastStep   = 0;
  static uint32_t lastPreset = 0xFFFFFFFF;

  // Restart cleanly whenever this preset is (re)selected
  if (presetStartTime != lastPreset) {
    lastPreset = presetStartTime;
    step = 0;
    lastStep = millis();
    strip.clear();
    strip.show();
    return;
  }

  if (millis() - lastStep < (uint32_t)SpeedDelay) return;   // not time yet
  lastStep = millis();

  const int n = strip.numPixels();

  // Fade brightness of all LEDs one step
  for (int j = 0; j < n; j++) {
    if (!meteorRandomDecay || random(10) > 5) {
      fadeToBlack(j, meteorTrailDecay);
    }
  }

  // Draw meteor
  for (int j = 0; j < meteorSize; j++) {
    int pos = step - j;
    if (pos >= 0 && pos < n) {
      // (G, R, B) order to match the rest of this file's colors
      strip.setPixelColor(pos, green, red, blue);
    }
  }

  strip.show();

  // Original runs i from 0 to 2*NUM_LEDS so the trail fades out before repeating
  if (++step >= n + n) {
    step = 0;
    strip.clear();
  }
}

// Shooting star effect
int shootingStarPos = 0;
long shootingStarFirstPixelHue = 0;
unsigned long lastShootingStarUpdate = 0;
int shootingStarWait = 20;   // adjust speed
int shootingStarTailLength = 40;
int shootingStarBrightness = 255;
void shootingStar() {
  if (millis() - lastShootingStarUpdate >= (unsigned long)shootingStarWait) {
    lastShootingStarUpdate = millis();

    strip.clear();
    for (int j = 0; j < shootingStarTailLength; j++) { // tail pixels
      int pos = shootingStarPos - j;
      if (pos >= 0 && pos < (int)strip.numPixels()) {
        int hue = shootingStarFirstPixelHue + pos * 65536L / strip.numPixels();
        int tailBrightness = shootingStarBrightness - (shootingStarBrightness / shootingStarTailLength) * j;
        uint32_t color = strip.gamma32(strip.ColorHSV(hue, 255, tailBrightness));
        strip.setPixelColor(pos, color);
      }
    }

    strip.show();

    shootingStarPos++;

    if (shootingStarPos >= (int)strip.numPixels() + shootingStarTailLength) {
      shootingStarPos = 0;
      shootingStarFirstPixelHue += 65536 / 90;
      if (shootingStarFirstPixelHue >= 65536) {
        shootingStarFirstPixelHue = 0;
      }
    }
  }
}

// Solid colors: fill every pixel, then push to the strip once
static void fillSolid(uint8_t a, uint8_t b, uint8_t c) {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, a, b, c);
  }
  strip.show();
}

void Red()       { fillSolid(0, 255, 0); }
void Blue()      { fillSolid(0, 0, 255); }
void Green()     { fillSolid(255, 0, 0); }
void lightGreen()     { fillSolid(255, 50, 50); }
void White()     { fillSolid(255, 255, 255); }
void warmWhite() { fillSolid(140, 255, 30); }

// Pastel hue gradient that travels across all trees as one long strip
const uint32_t pastelCycleMs = 10000;      // time for one full trip around the color wheel
const uint16_t pastelSpread  = 65536 / 2;  // how much of the wheel spans all trees at once
const uint8_t  pastelSat     = 190;         // low saturation = pastel (0-255)
const uint8_t  pastelVal     = 255;        // brightness

void pastelSweep() {
  const uint32_t total = 3 * LED_COUNT;
  uint32_t elapsed = millis() - presetStartTime;
  uint16_t baseHue = (uint64_t)(elapsed % pastelCycleMs) * 65536 / pastelCycleMs;

  for (int i = 0; i < LED_COUNT; i++) {
    uint32_t globalPos = (uint32_t)(NODE_ID - 1) * LED_COUNT + i;

    // Subtracting position makes colors travel from tree 1 toward tree 3
    uint16_t hue = baseHue - (uint16_t)(globalPos * pastelSpread / total);

    uint32_t c = strip.ColorHSV(hue, pastelSat, pastelVal);
    uint8_t r = c >> 16, g = c >> 8, b = c;
    strip.setPixelColor(i, g, r, b);   // (G, R, B) to match your strip
  }

  strip.show();
}

// Full color hue gradient that travels across all trees as one long strip
const uint32_t colorCycleMs = 10000;      // time for one full trip around the color wheel
const uint16_t colorSpread  = 65536 / 2;  // how much of the wheel spans all trees at once
const uint8_t  colorSat     = 255;         // low saturation = pastel (0-255)
const uint8_t  colorVal     = 255;        // brightness

void colorSweep() {
  const uint32_t total = 3 * LED_COUNT;
  uint32_t elapsed = millis() - presetStartTime;
  uint16_t baseHue = (uint64_t)(elapsed % colorCycleMs) * 65536 / colorCycleMs;

  for (int i = 0; i < LED_COUNT; i++) {
    uint32_t globalPos = (uint32_t)(NODE_ID - 1) * LED_COUNT + i;

    // Subtracting position makes colors travel from tree 1 toward tree 3
    uint16_t hue = baseHue - (uint16_t)(globalPos * colorSpread / total);

    uint32_t c = strip.ColorHSV(hue, colorSat, colorVal);
    uint8_t r = c >> 16, g = c >> 8, b = c;
    strip.setPixelColor(i, g, r, b);   // (G, R, B) to match your strip
  }

  strip.show();
}



int candyCaneDelay = 1000;
void candyCane() {
  uint32_t elapsed = millis() - presetStartTime;

  bool phase = (elapsed / candyCaneDelay) % 2;

  for (int i = 0; i < LED_COUNT; i++) {
    if ((i % 2 == 0) ^ phase) {
      strip.setPixelColor(i, 0, 255, 0);     // Red
    } else {
      strip.setPixelColor(i, 255, 255, 255); // White
    }
  }

  strip.show();
}

// Small integer hash: gives each pixel its own repeatable "random" values
static uint32_t hash32(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352dUL;
  x ^= x >> 15; x *= 0x846ca68bUL;
  x ^= x >> 16;
  return x;
}

// Retro C9-style bulbs with a per-bulb fade
const uint8_t bulbMin = 30;    // dimmest point (0 = fully off, 255 = no fade)
const uint8_t bulbMax = 255;   // brightest point

void retroBulbs() {
  // (G, R, B) order to match your strip
  static const uint8_t palette[5][3] = {
    {  0, 255,   0},   // red
    {255,   0,   0},   // green
    {  0,   0, 255},   // blue
    { 80, 255,   0},   // orange
    {200, 255,   0},   // yellow
  };
  uint32_t now = millis();

  for (int i = 0; i < LED_COUNT; i++) {
    const uint8_t *c = palette[i % 5];
    uint32_t h = hash32(i * 2654435761UL + 999);
    uint32_t period = 2500 + (h >> 8) % 3500;   // 2.5-6 s per fade cycle
    uint8_t wave = strip.sine8(((now + (h >> 16)) % period) * 256 / period);

    uint8_t level = bulbMin + (uint16_t)(bulbMax - bulbMin) * wave / 255;
    level = strip.gamma8(level);                 // even-looking fade

    strip.setPixelColor(i, c[0] * level / 255, c[1] * level / 255, c[2] * level / 255);
  }
  strip.show();
}

// =======================
// Aurora: drifting green / teal / purple curtains across all trees
// =======================
void aurora() {
  uint32_t t = millis() - presetStartTime;

  for (int i = 0; i < LED_COUNT; i++) {
    uint32_t pos = (uint32_t)(NODE_ID - 1) * LED_COUNT + i;   // continuous across trees

    // Three sine waves at different speeds/scales -> never visibly repeats
    uint8_t w1 = strip.sine8((uint8_t)(pos * 3 + t / 40));
    uint8_t w2 = strip.sine8((uint8_t)(pos * 5 - t / 27));
    uint8_t w3 = strip.sine8((uint8_t)(pos * 2 + t / 61));

    // Color: mostly green, drifting to teal, purple only where waves overlap
    uint16_t mix = (uint16_t)w1 * w3 / 255;
    uint16_t hue = 20000 + (uint32_t)mix * 29000 / 255;      // green -> teal -> purple

    // Brightness: bright "curtains" where waves line up, dim glow elsewhere
    uint8_t curtain = (uint16_t)w2 * w3 / 255;
    uint8_t bright  = 12 + (uint16_t)curtain * 243 / 255;

    uint32_t c = strip.gamma32(strip.ColorHSV(hue, 255, bright));
    uint8_t r = c >> 16, g = c >> 8, b = c;
    strip.setPixelColor(i, g, r, b);   // (G, R, B) to match your strip
  }

  strip.show();
}

// =======================
// Fireworks: rockets launch up the tree, burst, crackle and fade
// =======================
static inline uint8_t qadd8(uint8_t a, uint8_t b) {
  uint16_t s = a + b;
  return s > 255 ? 255 : s;
}

static void addRGB(uint8_t buf[][3], int i, uint8_t r, uint8_t g, uint8_t b) {
  if (i < 0 || i >= LED_COUNT) return;
  buf[i][0] = qadd8(buf[i][0], r);
  buf[i][1] = qadd8(buf[i][1], g);
  buf[i][2] = qadd8(buf[i][2], b);
}

const int      NUM_ROCKETS = 3;      // rockets in flight per tree
const uint32_t burstMs     = 1500;   // how long a burst lasts

void fireworks() {
  static uint8_t buf[LED_COUNT][3];  // normal R, G, B; overlapping sparks add up
  memset(buf, 0, sizeof(buf));

  uint32_t t = millis() - presetStartTime;

  for (int s = 0; s < NUM_ROCKETS; s++) {
    // Each rocket slot repeats on its own cycle, offset from the others
    uint32_t cycleLen = 3200 + s * 900;
    uint32_t tt       = t + s * 1300;
    uint32_t cycle    = tt / cycleLen;
    uint32_t local    = tt % cycleLen;

    // New random-looking rocket every cycle (different on each tree)
    uint32_t h        = hash32(cycle * 2654435761UL ^ (s * 7919 + NODE_ID * 104729));
    int      burstPos = LED_COUNT / 3 + h % (LED_COUNT / 2);   // upper part of tree
    uint16_t hue      = h >> 16;
    uint32_t launchMs = 450 + (h >> 4) % 350;
    int      maxR     = 8 + (h >> 12) % 10;                     // burst radius, pixels

    if (local < launchMs) {
      // Launch: warm white head with a short fading trail
      int head = (int)((uint32_t)burstPos * local / launchMs);
      for (int k = 0; k < 5; k++) {
        uint8_t v = 255 >> k;
        addRGB(buf, head - k, v, v * 3 / 4, v / 4);
      }
    } else if (local < launchMs + burstMs) {
      // Burst: sparks expand outward (fast, then slowing) and fade
      uint32_t tb   = local - launchMs;
      uint8_t  f    = tb * 255 / burstMs;
      uint8_t  inv  = 255 - f;
      uint8_t  ease = 255 - (uint16_t)inv * inv / 255;          // ease-out expansion
      int      r    = maxR * ease / 255;
      uint8_t  fade = (uint16_t)inv * inv / 255;                // fade-out

      // Bright white flash at the moment of the burst
      if (tb < 80) {
        for (int d = -2; d <= 2; d++) addRGB(buf, burstPos + d, 255, 255, 255);
      }

      for (int d = -r; d <= r; d++) {
        int p    = burstPos + d;
        int edge = r - abs(d);
        uint8_t v = (edge <= 2) ? fade : fade / 3;              // brighter leading edge

        // Crackle: sparks randomly dim for a moment
        if ((hash32(p * 977 + tb / 45 + s * 31337) & 3) == 0) v /= 4;

        uint32_t c = strip.gamma32(strip.ColorHSV(hue, 210, v));
        addRGB(buf, p, c >> 16, c >> 8, c);
      }
    }
    // Otherwise: dark gap before this slot's next rocket
  }

  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, buf[i][1], buf[i][0], buf[i][2]);   // (G, R, B)
  }
  strip.show();
}

const uint8_t  berryChance   = 4;     // ~1 in 8 pixels is a berry
const uint8_t  berryMin      = 100;   // dimmest red (raise for a subtler fade)
const uint8_t  berryMax      = 255;   // brightest red
const uint16_t berryPeriodMin = 2000; // slowest/fastest fade cycle, ms
const uint16_t berryPeriodMax = 5000;

void berryTree() {
  uint32_t now = millis();

  for (int i = 0; i < LED_COUNT; i++) {
    uint32_t h = hash32(i * 2654435761UL + 12345);

    if (h % berryChance != 0) {
      strip.setPixelColor(i, 255, 50, 50);   // Lighter green (G, R, B)
      continue;
    }

    // Per-berry fade speed and starting offset
    uint32_t period = berryPeriodMin + (h >> 8) % (berryPeriodMax - berryPeriodMin);
    uint32_t offset = (h >> 16) % period;

    // 0..255 sine wave position for this berry
    uint8_t angle = ((now + offset) % period) * 256 / period;
    uint8_t wave  = strip.sine8(angle);

    uint8_t level = berryMin + (uint16_t)(berryMax - berryMin) * wave / 255;
    strip.setPixelColor(i, 0, strip.gamma8(level), 0);   // Red (G, R, B)
  }

  strip.show();
}

int LSUDelay = 1000;
void LSU() {
  uint32_t elapsed = millis() - presetStartTime;

  bool phase = (elapsed / LSUDelay) % 2;
  for (int i = 0; i < LED_COUNT; i++) {
    if ((i % 2 == 0) ^ phase) {
      strip.setPixelColor(i, 255, 255, 0);
    } else {
      strip.setPixelColor(i, 0, 255, 255);
    }
  }
  strip.show();
}

const unsigned long rainbowInterval = 10;
const uint32_t rainbowStepPerTick = 60;
void rainbow() {
  uint32_t elapsed = millis() - presetStartTime;

  uint32_t steps = elapsed / rainbowInterval;
  uint32_t effectStep = (steps * rainbowStepPerTick) % (5 * 65536);

  strip.rainbow(effectStep);
  strip.show();
}

const unsigned long snowSparkleInterval = 200; // ms between sparkles
const unsigned long sparkleOnTime = 40;        // sparkle duration
void SnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay) {
  uint32_t elapsed = millis() - presetStartTime;

  // Which sparkle number are we on?
  uint32_t sparkleIndex = elapsed / snowSparkleInterval;

  // Time within this sparkle
  uint32_t sparklePhase = elapsed % snowSparkleInterval;

  // Deterministic "random" pixel
  uint16_t sparklePixel = (sparkleIndex * 37) % LED_COUNT;

  // Dim base color so the sparkle stands out
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, red, green, blue);
  }

  // Bright white sparkle during on-time
  if (sparklePhase < sparkleOnTime) {
    strip.setPixelColor(sparklePixel, 255, 255, 255);
  }

  strip.show();
}
