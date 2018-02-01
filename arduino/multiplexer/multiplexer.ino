/*
 E-drums multiplexer version. In this version we use a 74HC4052 mux to read multiple
 analog inputs.

 Useful link: https://coolcapengineer.wordpress.com/2013/01/04/electronics-serial-expansion-using-74hc4052/
 (we use the same names as in this article for the inputs)
*/

#include "definitions.h"

#define NUMBER_OF_SENSORS 2
#define MUX_CONTROL_PIN_0 7
#define MUX_CONTROL_PIN_1 8
#define MUX_READ_PIN A0

// Turn to true to see useful message to the serial monitor.
// Make sure you don't sent these to the sound card by unplugging the MIDI cable.
//const bool debug = true;
//const bool plot = true;
const bool debug = false;
// To debug a sensor change this to the index of the one you want. 0 means no plot.
bool plot = false;
unsigned plotSensor = 0;

// This maps the multiplexer's input pins to specific MIDI notes (specific drums).
// The position is the pin, the value is the MIDI code.
// https://en.wikipedia.org/wiki/General_MIDI#Percussion
// { Bass drum, Snare }
int inputToMidiMap[NUMBER_OF_SENSORS] = {36,38};

int previousSensorReadings[NUMBER_OF_SENSORS] = {0,0};

// Maximum read values to use for debouncing. Unless it drops to zero, we don't
// consider it a new hit. When the value drops, then play the maximum value;
int sensorMaxReadings[NUMBER_OF_SENSORS] = {0,0};

// After the value of a sensor starts to drop, we wait until it is equal to zero
// before we start calculating the value for the next knock. This is the "cool down"
// period and the status of each sensor is reflected in this variable.
int sensorCooldown[NUMBER_OF_SENSORS] = {false,false};

// The multiplier of the sensor value to use.
int sensorGains[NUMBER_OF_SENSORS] = {20,10};

int sensorReading = 0;
int previousSensorReading = 0;
int sensorMaxReading = 0;
int velocity = 0;

// From the sensor we read values from 0 to 1023. We ignore the first HIT_THRESHOLD
// values as too soft. That leaves us with sensorRange values which trigger hits.
// We need to normalize the read value to the range 0..127 which MIDI expects for
// velocity.
const int HIT_THRESHOLD = 1; // threshold value to decide when the detected sound is a knock or not
const int sensorRange = 1023 - HIT_THRESHOLD;
const float velocityCoefficient = 127.0 / sensorRange;

void setup() {
  // Multiplexer controlling pins
  pinMode(MUX_CONTROL_PIN_0, OUTPUT);
  pinMode(MUX_CONTROL_PIN_1, OUTPUT);

  if (debug || plot) {
    Serial.begin(4800);
  } else {
    Serial.begin(31250); // Set MIDI baud rate:
  }
}

void loop() {
  // Loop through the multiplexer inputs which are connected to some sensor.
  // To read the first sensor (i == 0), we need to set the combination of
  // MUX_CONTROL_PIN_0 and MUX_CONTROL_PIN_1 to "0" ("00" in binary). That means
  // MUX_CONTROL_PIN_0 = 0 (low) and MUX_CONTROL_PIN_1 = 0 (low).
  // To read the next sensor (i == 1), we want the binary 1 -> 01):
  // MUX_CONTROL_PIN_0 = 0, MUX_CONTROL_PIN_1 = 1
  // and so on so forth until we reach the NUMBER_OF_SENSORS (up to 4 for this multiplexer).

  // Useful: https://stackoverflow.com/a/26230537
  for (unsigned i=0; i < NUMBER_OF_SENSORS; i++) {
    digitalWrite(MUX_CONTROL_PIN_0, i & 1);
    digitalWrite(MUX_CONTROL_PIN_1, (i >> 1) & 1);
  //  digitalWrite(MUX_CONTROL_PIN_0, 0);
  //  digitalWrite(MUX_CONTROL_PIN_1, 0);

    sensorReading = analogRead(MUX_READ_PIN);
    if (plot && plotSensor == i && sensorReading > 0) {
      Serial.println(sensorReading);
      continue; // Do nothing else, we are just debugging using the plot.
    }

    // If in cool down mode
    if (sensorCooldown[i]) {
      // If cooldown is done, reset
      if (sensorReading == 0) {
        sensorCooldown[i] = false;
        sensorMaxReadings[i] = 0;
        previousSensorReadings[i] = 0;
      };
      continue;
    }

    previousSensorReading = previousSensorReadings[i];
    // If value is still rising increase the max read value.
    if (sensorReading > previousSensorReading) {
      sensorMaxReadings[i] = sensorReading;
    } else if (sensorReading < previousSensorReading) {
      sensorMaxReading = sensorMaxReadings[i]; 
      if (sensorMaxReading >= HIT_THRESHOLD) {
        velocity = round((sensorMaxReading - HIT_THRESHOLD + 1) * sensorGains[i] * velocityCoefficient);
        if (velocity > 127){ velocity = 127; }

        if (debug) {
          printDebugInfo(sensorMaxReading, sensorReading, previousSensorReading, velocity, velocityCoefficient);
        } else {
          noteOn(0x90, inputToMidiMap[i], velocity);
        }
      }
      sensorCooldown[i] = true;
    }
    previousSensorReadings[i] = sensorReading;
  }
}

void printDebugInfo(int sensorMaxReading, int sensorReading, int previousSensorReading, int velocity, float velocityCoefficient) {
  Serial.print("max value: ");
  Serial.print(sensorMaxReading);
  Serial.print(", sensorReading: ");
  Serial.print(sensorReading);
  Serial.print(", previousSensorReading: ");
  Serial.print(previousSensorReading);
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
}
