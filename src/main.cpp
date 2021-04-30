/**
 * VroomLamp/main.cpp
 * 
 * Main file for VroomLamp Code. 
 *  This lamp is meant to change animations based on the position of the shift stick.
 *  After the stick settles, the lighting animation is adjust and a vibration motor will go off.
 *  Also incorporates writing custom animations to EEPROM from a companion desktop app over serial.
 *  
 * Author: Shaqeeb Momen
 * Date: April 28, 2021
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <ShifterFSM.h>
#include <MotorFSM.h>
#include <AnimationDriver.h>

#define DEBUG

// Hardware defs
#define POT_PIN A6
#define STICK_PIN_1 A7
#define STICK_PIN_2 A5
#define MOTOR_PIN 4
#define PIXEL_PIN A2

#define NUM_LEDS 4

// Numerical Constants
// #define T_LOOP 0     // Execution loop time
#define T_MOTOR 300 // Time motor will be on for after a stick shift
#define T_SETTLE 300 // Settle time for stick position change
#define POT_THRES 15 // Threshold to read new pot values

// unsigned long loopTimer;

MotorFSM MotorControl([]() { digitalWrite(MOTOR_PIN, HIGH); }, []() { digitalWrite(MOTOR_PIN, LOW); }, []() { pinMode(MOTOR_PIN, OUTPUT); }, []() { return millis(); }, T_MOTOR);
ShifterFSM StickControl([]() { return millis(); }, T_SETTLE);

Adafruit_NeoPixel strip(NUM_LEDS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Current and previous values for LED brightness (used to only change brightness when needed)
uint16_t LEDscale;
uint16_t prevLEDScale;

// Handle Serial Communication
// This function runs whenever new data comes in
// TODO uncomment
/**
void serialEvent()
{
  byte buffer[122];
  // Data from the desktop should be a '_' character
  char check = (char)Serial.read();
  if (check == '_')
  {
    // Send a confirmation character
    Serial.println("ready");
    //then read all bytes until a '\n' is sent
    Serial.readBytesUntil('\n', buffer, sizeof(buffer) / sizeof(byte));

    animation *animationBuf = (animation *)malloc(sizeof(animation));
    animationBuf->frameCount = buffer[1];
    // Parse bytes into animation structure
    parseAnimation(buffer, animationBuf);
    // Write to EEPROM
    updateStoredMode(animationBuf, buffer[0]);
    // Free buffer
    free(animationBuf);
  }
  else
  {
    // Incorrect initial character, flush and try again
    while (Serial.available() > 0)
    {
      Serial.read();
    }
    // Send error over tx
    Serial.println("error");
  }
}
*/

/**
 * Parse animation from byte buffer into animation struct
 * @param buffer the buffer where the serial data was stored
 * @param animation the animation buffer allocated to parse the data into
 */
// void parseAnimation(byte *buffer, animation *animation)
// {
//   byte activeLength = 2 + animation->frameCount * sizeof(animFrame); // Length of populated items in buffer
//   byte frame = 0;
//   for (byte i = 2; i < activeLength; i += sizeof(animFrame))
//   {
//     animation->frames[frame].r = buffer[i];
//     animation->frames[frame].g = buffer[i + 1];
//     animation->frames[frame].b = buffer[i + 2];
//     animation->frames[frame].bright = buffer[i + 3];
//     // TODO Check if little endian vs big endian, current assumes big endian (most significant byte first)
//     animation->frames[frame].time = (buffer[i + 4] << 8) | buffer[i + 5];
//     if (i + sizeof(animFrame) >= activeLength) // Last iteration
//     {
//       // Save the total animation time
//       animation->totalTime = animation->frames[frame].time;
//     }
//     frame++;
//   }
// }

// LED METHODS

void setup()
{

  // Start Serial Communication
  Serial.begin(115200);
  //  Digital I/O
  pinMode(MOTOR_PIN, OUTPUT);
  // LED Setup
  strip.begin();
  strip.show();

  // Initial Motor state
  MotorControl.init();
  // Initial Stick state
  StickControl.init(analogRead(STICK_PIN_1) - analogRead(STICK_PIN_2));
  // Initial Brightnes
  LEDscale = analogRead(POT_PIN);
  strip.setBrightness(LEDscale / 4);
  // Initialize timers
  // loopTimer = millis();
}

void loop()
{

  // /************ BRIGHTNESS KNOB ***********/
  LEDscale = analogRead(POT_PIN) / 4;
  if (abs(LEDscale - prevLEDScale) > POT_THRES)
  {
    strip.setBrightness(LEDscale);
    prevLEDScale = LEDscale;
  }

  // /************ HANDLING STICK INPUT ***********/
  ShifterFSM::mode currentMode = StickControl.run(analogRead(STICK_PIN_1) - analogRead(STICK_PIN_2));

  // /************ MOTOR FSM ***********/
  if (StickControl.getFlag())
  {
    MotorControl.trigger();
  }
  MotorControl.run();

  /************ DRIVING LEDS ***********/
  // Pass current animation, time stamp, brightness, into animation driving function
  switch (currentMode)
  {
  case ShifterFSM::R:
    strip.fill(strip.Color(255, 0, 0));
    break;

  case ShifterFSM::ONE:
    strip.fill(strip.Color(0, 255, 0));
    break;

  case ShifterFSM::TWO:
    strip.fill(strip.Color(0, 0, 255));
    break;

  case ShifterFSM::THREE:
    strip.fill(strip.Color(200, 200, 0));
    break;

  case ShifterFSM::FOUR:
    strip.fill(strip.Color(0, 200, 200));
    break;

  case ShifterFSM::FIVE:
    strip.fill(strip.Color(200, 0, 200));
    break;

  case ShifterFSM::SIX:
    strip.fill(strip.Color(100, 200, 40));
    break;
  default:
    break;
  }
  strip.show();
}
