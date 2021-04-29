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

#define DEBUG

// Hardware defs
#define POT_PIN A6
#define STICK_PIN_1 A7
#define STICK_PIN_2 A5
#define MOTOR_PIN 4
#define PIXEL_PIN A2

#define NUM_LEDS 4

// Numerical Constants
#define T_MOTOR 300   // Time motor will be on for after a stick shift
#define T_SETTLE 400 // Settle time for stick position change

MotorFSM MotorControl(MOTOR_PIN, T_MOTOR);
ShifterFSM StickControl(T_SETTLE);

Adafruit_NeoPixel strip(NUM_LEDS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

struct animFrame
{
  byte r;
  byte g;
  byte b;
  byte bright;
  int time;
};

struct animation
{
  animFrame frames[20];
  byte frameCount;
  int totalTime;
};
animation activeAnimation;
// uint8_t LEDscale = 255;

bool isIdle = false; // Actively reading inputs or not

unsigned long animationTimeStamp = 0; // System time when current animation assigned

// Get the time within the current cycle of the active animation
unsigned long getAnimationTime(unsigned long *timeStamp, unsigned long animPeriod)
{
  // Getting the time since the current animation was first initiated,
  // then getting modulo of the animation's period to get position in current cycle
  return (millis() - *timeStamp) % animPeriod;
}

// Run the LED's
// TODO: pass in parameter for current loaded animation frame
void writeLEDs(unsigned long animT, int brightness)
{
}

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
void parseAnimation(byte *buffer, animation *animation)
{
  byte activeLength = 2 + animation->frameCount * sizeof(animFrame); // Length of populated items in buffer
  byte frame = 0;
  for (byte i = 2; i < activeLength; i += sizeof(animFrame))
  {
    animation->frames[frame].r = buffer[i];
    animation->frames[frame].g = buffer[i + 1];
    animation->frames[frame].b = buffer[i + 2];
    animation->frames[frame].bright = buffer[i + 3];
    // TODO Check if little endian vs big endian, current assumes big endian (most significant byte first)
    animation->frames[frame].time = (buffer[i + 4] << 8) | buffer[i + 5];
    if (i + sizeof(animFrame) >= activeLength) // Last iteration
    {
      // Save the total animation time
      animation->totalTime = animation->frames[frame].time;
    }
    frame++;
  }
}

void updateStoredMode(animation *animation, byte mode)
{
  // TODO test everything else before writing to EEPROM to save cycles
  // Store structure to EEPROM
  // EEPROM.put(mode * sizeof(animation), animation);
}

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
  strip.setBrightness(200);

  // Initial Motor state
  MotorControl.init();
  // Initial Stick state
  StickControl.init(analogRead(STICK_PIN_1));
  // Initialize timers
  animationTimeStamp = millis();
}

void loop()
{

  // /************ BRIGHTNESS KNOB ***********/
  strip.setBrightness(analogRead(POT_PIN) / 4);

  // // /************ HANDLING STICK INPUT ***********/
  ShifterFSM::mode currentMode = StickControl.run(analogRead(STICK_PIN_1) - analogRead(STICK_PIN_2));
  // Serial.println();
  // Serial.print("  |  ");
  // Serial.print("MODE: ");
  // Serial.println(!MotorControl.isRunning() ? StickControl.getModeName(currentMode) : "SHIFTING");
  if (StickControl.getFlag())
  {
    MotorControl.trigger();
  }
  MotorControl.run();

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

  case ShifterFSM::NEUTRAL:
  default:
    Serial.println("UNKOWN");
    strip.fill(strip.Color(0, 0, 0));
    break;
  }
  strip.show();

  /************ DRIVING LEDS ***********/
  // Pass current animation, time stamp, brightness, into animation driving function
}
