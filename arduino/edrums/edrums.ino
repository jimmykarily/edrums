/*
 E-drums
*/

#include "definitions.h"

// Turn to true to see useful message to the serial monitor.
// Make sure you don't sent these to the sound card by unplugging the MIDI cable.

const bool debug = true;
const bool plot = true;
//const bool debug = false;
//const bool plot = false;

int sensorReading = 0;
const int ledPin = 13;
int ledState = LOW; // variable used to store the last LED status, to toggle the light

// From the sensor we read values from 0 to 1023. We ignore the first HIT_THRESHOLD
// values as too soft. That leaves us with sensorRange values which trigger hits.
// We need to normalize the read value to the range 0..127 which MIDI expects for
// velocity.
const int sensorRange = 1023 - HIT_THRESHOLD;
const float velocityCoefficient = 127.0 / sensorRange;

      // Normalize velocity to 0..127 (Analog pin ranges from 0 to 1023)
      // Take into account that our "effective range is reduced by HIT_THRESHOLD.
      // vel = max * 127 / (1023 - HIT_THRESHOLD)

// Maximum read value to use for debouncing. Unless it drops to zero, we don't
// consider it a new hit. When the value drops, then play the maximum value;
int maxValue = 0;
int velocity = 0;
bool coolDown = false;

void setup() {
  pinMode(ledPin, OUTPUT); // declare the ledPin as as OUTPUT

  // Set MIDI baud rate:
  if (debug) {
    Serial.begin(9600);
  } else {
    Serial.begin(31250);
  }
}

void loop() {
  sensorReading = analogRead(SNARE_PIN);

  if (plot && sensorReading > 0) {
    Serial.println(sensorReading);
  }

  // Wait until voltage is zeroed.
  if (coolDown == true) {
    if (sensorReading == 0) {
      coolDown = false;
    }
  } else if (sensorReading > maxValue) {
    maxValue = sensorReading;
    digitalWrite(ledPin, HIGH);
  // When value is seriously reduced (below half), it is a new hit. Play the last hit and
  // reset to the new value.
  } else if (!coolDown) { // if (sensorReading < round(maxValue / 2)) {
    if (maxValue >= HIT_THRESHOLD) {
      velocity = round((maxValue - HIT_THRESHOLD + 1) * velocityCoefficient);
      if (debug && !plot) {
        printDebugInfo();
      } else if (!plot) {
        // Normalize velocity to 0..127
        noteOn(0x90, SNARE, velocity);
      }
    }

    resetHit();
  }
}

void resetHit() {
  digitalWrite(ledPin, LOW);
  maxValue = 0;
}

void printDebugInfo() {
  Serial.print("cooldown: ");
  Serial.print(coolDown);
  Serial.print(", max value: ");
  Serial.print(maxValue);
  Serial.print(", velocity: ");
  Serial.print(velocity);
  Serial.print(", velocity coefficient: ");
  Serial.println(velocityCoefficient);
}

// plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that
// data values are less than 127:
void noteOn(int cmd, int pitch, int velocity) {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
  coolDown = true;
  //delay(100);
}
