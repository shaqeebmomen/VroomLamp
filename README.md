# VroomLamp
Code for a shard light that uses a shift stick to pick animations. Animations are stored in EEPROM, and can be updated over serial from a companion desktop app

## Operation
- Companion app is used to make and download animations (up to 6) over USB serial
- Shifter stick is used to select lighting animations as if they were "gears"
- Lamp interpolates through frames of the animation and cycles the correct colors
- Upon shifting, a motor will vibrate the lamp to provide tactile feedback

## Key Features
- Everything custom designed/printed (knob in resin, housing in plastice) & wired aside from acrylic
- 2 light sensors to determine shifter position
- Potentiometer for brightness adjust
- Animations stored to EEPROM (via I2C) after appropriate checks from master computer and slave lamp MCU
- FSM to handle changes in shifter position
- FSM to handle motor operation
- Animation driver class & FSM classes loosely coupled with system functions passed in as pointers for reuse in other projects

## Video

https://user-images.githubusercontent.com/11233745/164988592-1183f7aa-565f-4660-b4ec-511f14b0c26a.mp4
