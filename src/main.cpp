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
// #define DEBUG_EEPROM_SERIAL

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
// Serial Constants
#define SERIAL_PACKET 142
#define FRAME_SIZE 7
#define META_SIZE 2

// unsigned long loopTimer;

MotorFSM MotorControl([]() { digitalWrite(MOTOR_PIN, HIGH); }, []() { digitalWrite(MOTOR_PIN, LOW); }, []() { pinMode(MOTOR_PIN, OUTPUT); }, millis, T_MOTOR);
ShifterFSM StickControl(millis, T_SETTLE);
ShifterFSM::mode currentMode;

Adafruit_NeoPixel strip(NUM_LEDS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

AnimationDriver::AnimationDriver animator(millis);
// Default animations

const AnimationDriver::animation Solid_White PROGMEM = SOLID_COLOR(255, 255, 255);
const AnimationDriver::animation Solid_Red PROGMEM = SOLID_COLOR(255, 0, 0);
const AnimationDriver::animation Breathe_White PROGMEM = BREATHE_COLOR(255, 255, 255, 3000UL);
const AnimationDriver::animation Solid_Green PROGMEM = SOLID_COLOR(0, 255, 0);
const AnimationDriver::animation Rainbow PROGMEM = RAINBOW(4000UL);
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

// Function used for resetting programmatically
void (*resetFunc)(void) = 0;

// Load specific animation from eeprom into currentAnim
void EEPROM_Load(uint8_t index)
{
#ifdef DEBUG_EERPROM
  Serial.print("Getting Index: ");
  Serial.print(index);
  Serial.print(" Addr: ");
  Serial.println((int)(index * sizeof(currentAnim)));
#endif
  EEPROM.get((int)(index * sizeof(AnimationDriver::animation)), currentAnim);
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
  Serial.println("DEFAULTS WRITTEN TO EEPROM");
  Serial.flush();
}

int getStickPos(int *stick1, int *stick2)
{

#define STICK_THRES 30
// R
#define GR_1 64
#define GR_2 10
// 1
#define G1_1 271
#define G1_2 25
// 2
#define G2_1 114
#define G2_2 10
// 3
#define G3_1 78
#define G3_2 98
// 4
#define G4_1 99
#define G4_2 110
// 5
#define G5_1 26
#define G5_2 402
// 6
#define G6_1 18
#define G6_2 126

  // R
  if (abs(*stick1 - GR_1) < STICK_THRES && abs(*stick2 - GR_2) < STICK_THRES)
  {
    return 0;
  }
  // 1
  else if (abs(*stick1 - G1_1) < STICK_THRES && abs(*stick2 - G1_2) < STICK_THRES)
  {
    return 1;
  }
  // 2
  else if (abs(*stick1 - G2_1) < STICK_THRES && abs(*stick2 - G2_2) < STICK_THRES)
  {
    return 2;

  } // 3
  else if (abs(*stick1 - G3_1) < STICK_THRES && abs(*stick2 - G3_2) < STICK_THRES && *stick1 < 90)
  {
    return 3;

  } // 4
  else if (abs(*stick1 - G4_1) < STICK_THRES && abs(*stick2 - G4_2) < STICK_THRES)
  {
    return 4;

  } // 5
  else if (abs(*stick1 - G5_1) < STICK_THRES && abs(*stick2 - G5_2) < STICK_THRES)
  {
    return 5;
  }
  // 6
  else if (abs(*stick1 - G6_1) < STICK_THRES && abs(*stick2 - G6_2) < STICK_THRES)
  {
    return 6;
  }
  else
  {
    return 7;
  }
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

// Serial Methods

// Parse out an animation object from a serial buffer and store in EEPROM
void saveAnimationFromSerial(byte *buff)
{
  AnimationDriver::animation _a;
  _a.frameCount = buff[1];
  // For each frame
  for (byte i = 0; i < buff[1]; i++)
  {
    byte baseIndex = i * FRAME_SIZE + 2;
    _a.frames[i].color[0] = buff[baseIndex];                                                                                                                            // Red
    _a.frames[i].color[1] = buff[baseIndex + 1];                                                                                                                        // Green
    _a.frames[i].color[2] = buff[baseIndex + 2];                                                                                                                        // Blue
    _a.frames[i].time = (uint32_t)buff[baseIndex + 3] << 24 | (uint32_t)buff[baseIndex + 4] << 16 | (uint32_t)buff[baseIndex + 5] << 8 | (uint32_t)buff[baseIndex + 6]; //time
    if (i == buff[1] - 1)
    {
      _a.time = _a.frames[i].time;
    }
  }
  EEPROM.put(buff[0] * sizeof(AnimationDriver::animation), _a);
}

// Waits for acknowledge byte (0xff) from pc
bool waitForAck(uint32_t timeout)
{
  // Start timer
  uint32_t timer = millis();
  //  Serial.println(F("Waiting for ACK"));
  while (true)
  {
    // Check for Serial data or
    if (Serial.available() > 0)
    {
      uint8_t ack = (byte)Serial.read();
      if (ack == 0xff)
      {
        // Success
        return true;
      }
      else
      {
        // fail
        Serial.println(F("ACK Fail"));
        Serial.flush();
        return false;
      }
    }
    // Check if timer has ran out
    else if (millis() - timer > timeout)
    {
      Serial.println(F("ACK Fail"));
      Serial.flush();
      return false;
    }
  }
}

// Handle an an upload request
void handleUploadRequest()
{
  byte localBuff[SERIAL_PACKET];
  byte buffCount = META_SIZE;
  // Wait for first 2 bytes to come in
  while (Serial.available() < META_SIZE)
    ;
  Serial.readBytes(localBuff, META_SIZE);

  // While the pc is sending data, store it in the buffer
  // Loop untill all bytes expected are read
  while (buffCount < (localBuff[1] * FRAME_SIZE + META_SIZE))
  {
    // Let a byte come in
    while (Serial.available() < 1)
      ;
    if (buffCount <= SERIAL_PACKET)
    {
      // Append to buffer
      byte data = (byte)Serial.read();
      localBuff[buffCount] = data;
      buffCount++;
    }
    // Writing outside buffer space, send an error back
    else
    {
      Serial.println();
      return;
    }
  }
  // Once all the data has been received, write it back to the pc
  Serial.write(localBuff, buffCount);
  // Read a check character (0x00 -> fail, 0xff -> success)
  if (waitForAck(1000))
  {
    // Success
    // Store data in memory if check character came back okay
    saveAnimationFromSerial(localBuff);
    // Send one more string back to indicate write finished
    Serial.println(F("Done"));
    Serial.flush();
  }
  else
  {
    resetFunc();
  }
}

// Handle request for download
void handleDownloadRequest()
{
  for (uint8_t i = 0; i < 6; i++)
  {
    AnimationDriver::animation _a;
    EEPROM.get(i * sizeof(AnimationDriver::animation), _a);
    // Write the frame count
    if (!waitForAck(1000))
    {
      resetFunc();
    }
    Serial.write(i);
    Serial.write(_a.frameCount);
    Serial.flush();
    // Wait for an acknowledge or timeout
    if (!waitForAck(1000))
    {
      resetFunc();
    }
    // Send rest of animation frames
    // Parse animation object into uint8_t array
    uint8_t frameBuff[_a.frameCount * FRAME_SIZE];
    for (uint8_t frame = 0; frame < _a.frameCount; frame++)
    {
      uint8_t baseIndex = frame * FRAME_SIZE;
      // Red
      frameBuff[baseIndex] = _a.frames[frame].color[0];
      // Green
      frameBuff[baseIndex + 1] = _a.frames[frame].color[1];
      // Blue
      frameBuff[baseIndex + 2] = _a.frames[frame].color[2];
      // Timestamp (four bytes)
      frameBuff[baseIndex + 3] = (uint8_t)(_a.frames[frame].time >> 24);
      frameBuff[baseIndex + 4] = (uint8_t)(_a.frames[frame].time >> 16);
      frameBuff[baseIndex + 5] = (uint8_t)(_a.frames[frame].time >> 8);
      frameBuff[baseIndex + 6] = (uint8_t)(_a.frames[frame].time);
    }
    // Send buffer
    Serial.write(frameBuff, _a.frameCount * FRAME_SIZE);
    Serial.flush();
    // Wait for acknowledge or timeout
    if (!waitForAck(1000))
    {
      resetFunc();
    }
  }
}

// Handle overall Serial Communication
void handleSerial()
{
  // Read until code ends
  String code = Serial.readStringUntil('-');
  // Echo Back a ready string and acknowledge the code received
  Serial.print(F("ready_"));
  Serial.println(code);
  Serial.flush();
  // Do something useful with the intent code
  switch (code[0])
  {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
    handleUploadRequest();
    break;
  case 'd':
    handleDownloadRequest();
    break;
  default:
    Serial.println();
    break;
  }
}

// DEBUG Functions
#ifdef DEBUG_EEPROM_SERIAL
void EEPROM_Dump_Anim(uint8_t index)
{
  AnimationDriver::animation _anim;
  EEPROM.get(index * sizeof(AnimationDriver::animation), _anim);
  Serial.print(F("Animation at Index "));
  Serial.println(index);
  Serial.print(F("Frame Count: "));
  Serial.println(_anim.frameCount);
  Serial.print(F("Total Time: "));
  Serial.println(_anim.time);
  Serial.println(F("Frames: "));
  for (uint8_t i = 0; i < _anim.frameCount; i++)
  {
    Serial.print(F("Frame: "));
    Serial.println(i);
    Serial.print(F("R: "));
    Serial.println(_anim.frames[i].color[0]);
    Serial.print(F("G: "));
    Serial.println(_anim.frames[i].color[1]);
    Serial.print(F("B: "));
    Serial.println(_anim.frames[i].color[2]);
    Serial.print(F("Time: "));
    Serial.println(_anim.frames[i].time);
  }
}
#endif

void setup()
{
  // Start Serial Communication
  Serial.begin(115200);
  Serial.println(F("ready"));
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

#ifdef DEBUG_EEPROM_SERIAL
  for (uint8_t i = 0; i < 6; i++)
  {
    Serial.println(F("------------------------"));
    EEPROM_Dump_Anim(i);
    delay(1000);
  }
  Serial.println(F("------------------------"));
#endif
}

void loop()
{
  if (Serial.available() > 0)
  {
    // Handle Serial Request
    handleSerial();
    updateAnimator(&currentMode);
    resetFunc();
  }
  else
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
  }

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
  Serial.print(analogRead(STICK_PIN_1));
  Serial.print(" 2: ");
  Serial.print(analogRead(STICK_PIN_2));
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
