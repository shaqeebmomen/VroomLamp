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

// DEBUG FLAGS
#define DEBUG
// #define DEBUG_MOVING
// #define DEBUG_STICKS
#define DEBUG_MODE
#define DEBUG_LED

// Hardware defs
#define POT_PIN A6
#define STICK_PIN_1 A7
#define STICK_PIN_2 A5
#define MOTOR_PIN 4
// #define MOTOR_PIN 13
#define PIXEL_PIN A2

#define NUM_LEDS 4

// Numerical Constants
// #define T_LOOP 0     // Execution loop time
#define T_MOTOR 200  // Time motor will be on for after a stick shift
#define T_SETTLE 150 // Settle time for stick position change
#define POT_THRES 15 // Threshold to read new pot values
#define MOVE_THRES 16
#define MOVE_GAIN 3
#define T_MOVE_LOOP 30
#define FILTER_BUFF 20

// unsigned long loopTimer;

MotorFSM MotorControl([]() { digitalWrite(MOTOR_PIN, HIGH); }, []() { digitalWrite(MOTOR_PIN, LOW); }, []() { pinMode(MOTOR_PIN, OUTPUT); }, millis, T_MOTOR);
ShifterFSM StickControl(millis, T_SETTLE);
ShifterFSM::mode currentMode;

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

int getStickPos(int *stick1, int *stick2)
{
  return *stick1 - *stick2;
}

int readStick1(bool override)
{
  static unsigned long sum = 0;
  static unsigned long count = 0;
  if (!override)
  {
    if (count > FILTER_BUFF)
    {
      sum = 0;
      count = 0;
    }

    sum += analogRead(STICK_PIN_1);
    count++;
  }

  return sum / count;
}
int readStick2(bool override)
{
  static unsigned long sum = 0;
  static unsigned long count = 0;
  if (!override)
  {
    if (count > FILTER_BUFF)
    {
      sum = 0;
      count = 0;
    }
    sum += analogRead(STICK_PIN_2);
    count++;
  }

  return sum / count;
}

int getStickPos(int stick1, int stick2)
{
  return getStickPos(&stick1, &stick2);
}

bool isMoving(int *stick1, int *stick2)
{
  static unsigned long moveTimer = millis();
  static bool output = false;
  static int prev1 = *stick1;
  static int prev2 = *stick2;

  if (millis() - moveTimer > T_MOVE_LOOP)
  {
    if (abs(*stick1 - prev1) * MOVE_GAIN > MOVE_THRES || abs(*stick2 - prev2) * MOVE_GAIN > MOVE_THRES)
    {
      output = true;
    }
    else
    {
      output = false;
    }

    prev1 = *stick1;
    prev2 = *stick2;
    moveTimer = millis();
  }

  return output;
}
void setup()
{

  // Start Serial Communication
  Serial.begin(115200);
  // LED Setup
  strip.begin();
  strip.show();

  // Initial Motor state
  MotorControl.init();
  // Initial Stick state
  StickControl.init(getStickPos(readStick1(MotorControl.isRunning()), readStick2(MotorControl.isRunning())));
  // Initial Brightnes
  LEDscale = analogRead(POT_PIN);
  strip.setBrightness(LEDscale / 4);
  // Initialize timers
  // loopTimer = millis();
}

void loop()
{
  /************ BRIGHTNESS KNOB ***********/
  LEDscale = analogRead(POT_PIN) / 4;
  if (abs(LEDscale - prevLEDScale) > POT_THRES)
  {
    strip.setBrightness(LEDscale);
    prevLEDScale = LEDscale;
  }

  /************ HANDLING STICK INPUT ***********/
  int stick1 = readStick1(MotorControl.isRunning());
  int stick2 = readStick2(MotorControl.isRunning());

#ifdef DEBUG_MOVING
  Serial.print(isMoving(&stick1, &stick2) ? "MOVING " : "");
  Serial.flush();
#endif

#ifdef DEBUG_STICKS
  Serial.print(" 1: ");
  Serial.print(stick1);
  Serial.print(" 2: ");
  Serial.print(stick2);
  Serial.flush();
#endif

  currentMode = StickControl.run(getStickPos(&stick1, &stick2), isMoving(&stick1, &stick2));

#ifdef DEBUG_MOVING
  ShifterFSM::mode currentMode = StickControl.run(getStickPos(&stick1, &stick2), isMoving(&stick1, &stick2));
#ifndef DEBUG_LED
  if (isMoving(&stick1, &stick2))
  {
    strip.fill(strip.Color(0, 0, 0));
  }
  else
  {
    strip.fill(strip.Color(255, 255, 255));
  }
#endif
#endif

#ifdef DEBUG_MODE
  Serial.print(" mode: ");
  Serial.print(currentMode);
  Serial.flush();

#endif

  /************ MOTOR FSM ***********/
  if (StickControl.getFlag())
  {
    MotorControl.trigger();
  }
  MotorControl.run();

/************ DRIVING LEDS ***********/
// Pass current animation, time stamp, brightness, into animation driving function
#ifdef DEBUG_LED
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

#endif

#ifdef DEBUG
  Serial.println();
  Serial.flush();
#endif
}
