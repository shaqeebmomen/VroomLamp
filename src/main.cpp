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
#include <DefaultAnimations.h>

// DEBUG FLAGS
// #define DEBUG
// #define DEBUG_MOVING
// #define DEBUG_STICKS
// #define DEBUG_MODE
// #define DEBUG_LED
// #define DEBUG_STICK_TUNE
// #define DEBUG_EEPROM

// Routine enable flags
#define EN_MOTOR
#define EN_ANIMATION

// Hardware defs
#define POT_PIN A6
#define STICK_PIN_1 A7
#define STICK_PIN_2 A5
#define MOTOR_PIN 4
#define PIXEL_PIN A2

#define NUM_LEDS 4

// Numerical Constants
// #define T_LOOP 0     // Execution loop time
#define T_MOTOR 200  // Time motor will be on for after a stick shift
#define T_SETTLE 150 // Settle time for stick position change
#define POT_THRES 10 // Threshold to read new pot values
#define MOVE_THRES 40
#define MOVE_GAIN 3
#define T_MOVE_LOOP 30
#define FILTER_BUFF 20
#define GEAR_COUNT 6

// unsigned long loopTimer;

MotorFSM MotorControl([]() { digitalWrite(MOTOR_PIN, HIGH); }, []() { digitalWrite(MOTOR_PIN, LOW); }, []() { pinMode(MOTOR_PIN, OUTPUT); }, millis, T_MOTOR);
ShifterFSM StickControl(millis, T_SETTLE);
ShifterFSM::mode currentMode;

Adafruit_NeoPixel strip(NUM_LEDS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

AnimationDriver::AnimationDriver animator(millis);
// Default animations

const AnimationDriver::animation Solid_White PROGMEM = SOLID_COLOR(255, 255, 255);
const AnimationDriver::animation Solid_Red PROGMEM = SOLID_COLOR(255, 0, 0);
const AnimationDriver::animation Breathe_White PROGMEM = BREATHE_COLOR(255, 255, 255, 2000);
const AnimationDriver::animation Solid_Green PROGMEM = SOLID_COLOR(0, 255, 0);
const AnimationDriver::animation Rainbow PROGMEM = RAINBOW(2000);
const AnimationDriver::animation Solid_Blue PROGMEM = SOLID_COLOR(0, 0, 255);

const AnimationDriver::animation defaults[] PROGMEM = {
    Solid_White,
    Solid_Red,
    Breathe_White,
    Solid_Green,
    Rainbow,
    Solid_Blue};

AnimationDriver::animation currentAnim;

// Current and previous values for LED brightness (used to only change brightness when needed)
uint16_t LEDscale;
uint16_t prevLEDScale;

// Load specific animation from eeprom into currentAnim
void EEPROM_Load(uint8_t index)
{
#ifdef DEBUG_EERPROM
  Serial.print("Getting Index: ");
  Serial.print(index);
  Serial.print(" Addr: ");
  Serial.println((int)(index * sizeof(currentAnim)));
#endif
  EEPROM.get((int)(index * sizeof(currentAnim)), currentAnim);
#ifdef DEBUG_EEPROM
  Serial.println(currentAnim.frameCount);
  Serial.println("Animation Loaded");
  Serial.flush();
#endif
}

// Write defaults to eeprom
void EEPROM_WriteDefaults()
{
#ifdef DEBUG_EEPROM
  Serial.println("RESETTING ANIMATIONS");
  Serial.flush();
#endif

  // Clear the buffer of the trigger character
  Serial.read();
  // Write to defaults to eeprom
  for (uint8_t i = 0; i < GEAR_COUNT; i++)
  {
    AnimationDriver::animation animBuff;
    memcpy_P(&animBuff, &defaults[i], sizeof(animBuff));
#ifdef DEBUG_EEPROM
    Serial.print("Writing To: ");
    Serial.println((int)(i * sizeof(animBuff)));
#endif
    EEPROM.put((int)(i * sizeof(AnimationDriver::animation)), animBuff);
  }
#ifdef DEBUG_EEPROM
  Serial.println("DEFAULTS WRITTEN TO EEPROM");
  Serial.flush();
#endif
}
// Handle Serial Communication
// This function runs whenever new data comes in
void serialEvent()
{
  char data = ' ';
  data = Serial.peek();
  if (data == '!')
  {
    EEPROM_WriteDefaults();
  }

  // byte buffer[122];
  // // Data from the desktop should be a '_' character
  // char check = (char)Serial.read();
  // if (check == '_')
  // {
  //   // Send a confirmation character
  //   Serial.println("ready");
  //   //then read all bytes until a '\n' is sent
  //   Serial.readBytesUntil('\n', buffer, sizeof(buffer) / sizeof(byte));

  //   AnimationDriver::animation *animationBuf = (AnimationDriver::animation *)malloc(sizeof(AnimationDriver::animation));
  //   animationBuf->frameCount = buffer[1];
  //   // Parse bytes into animation structure
  //   parseAnimation(buffer, animationBuf);
  //   // Write to EEPROM
  //   updateStoredMode(animationBuf, buffer[0]);
  //   // Free buffer
  //   free(animationBuf);
  // }
  // else
  // {
  //   // Incorrect initial character, flush and try again
  //   while (Serial.available() > 0)
  //   {
  //     Serial.read();
  //   }
  //   // Send error over tx
  //   Serial.println("error");
  // }
}

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
  int sum = *stick1 + *stick2;
  int diff = *stick1 - *stick2;
  if (sum > 190)
  {
    return sum;
  }
  else
  {
    return diff;
  }
  // return *stick1 + *stick2;
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

void updateAnimator(ShifterFSM::mode *mode)
{
  static ShifterFSM::mode lastMode;
  if (lastMode != *mode)
  {
    if (*mode == ShifterFSM::R)
    {
      currentAnim = SOLID_COLOR(0, 0, 0);
    }
    else if (*mode > 0 && *mode < 7)
    {
      EEPROM_Load(*mode - 1);
    }
    animator.updateAnimation(currentAnim);
    lastMode = *mode;
  }
}

void setup()
{
  // Start Serial Communication
  Serial.begin(115200);
  Serial.println("STARTING ");
  // LED Setup
  strip.begin();
  strip.show();
  // Initial Motor state
  MotorControl.init();
  // Initial Stick state
  currentMode = StickControl.init(getStickPos(readStick1(MotorControl.isRunning()), readStick2(MotorControl.isRunning())));
  // Initial Brightness
  LEDscale = analogRead(POT_PIN);
  strip.setBrightness(LEDscale / 4);
  // Animation Controller
  updateAnimator(&currentMode);
// Initialize timers
// loopTimer = millis();
#ifdef DEBUG_STICK_TUNE
  strip.fill(strip.Color(255, 255, 255));
#endif
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

  currentMode = StickControl.run(getStickPos(&stick1, &stick2), isMoving(&stick1, &stick2));

  /************ MOTOR & ANIMATION RESET TRIGGER ***********/
  if (StickControl.getFlag())
  {
#ifdef EN_MOTOR
    MotorControl.trigger();
#endif
#ifdef EN_ANIMATION
    updateAnimator(&currentMode);
#endif
  }
  MotorControl.run();

  /************ DRIVING LEDS ***********/
  // Pass current animation, time stamp, brightness, into animation driving function
#ifdef EN_ANIMATION
  animator.run([](uint8_t r, uint8_t g, uint8_t b) { strip.fill(strip.Color(r, g, b));strip.show(); });
#endif


  /************ DUBUGGING HELP ***********/
#ifdef DEBUG_STICK_TUNE
  strip.fill(strip.Color(255, 255, 255));
  strip.show();
#endif
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

#ifdef DEBUG_MOVING
  Serial.print(isMoving(&stick1, &stick2) ? "MOVING " : "");
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

#ifdef DEBUG_STICKS
  Serial.print(" 1: ");
  Serial.print(stick1);
  Serial.print(" 2: ");
  Serial.print(stick2);
#endif

#ifdef DEBUG_MODE
  Serial.print(" mode: ");
  Serial.print(currentMode);
#endif

#ifdef DEBUG
  Serial.println();
  Serial.flush();
#endif
}
