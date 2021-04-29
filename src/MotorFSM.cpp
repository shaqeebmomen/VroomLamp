#include <MotorFSM.h>

/**
 * Constructor for finite state machine object
 * @param pin the pin connected to the motor
 * @param runtime the length of time the motor should run for
 * 
 */
MotorFSM::MotorFSM(uint8_t pin, unsigned long runtime)
{
    _pin = pin;
    _runtime = runtime;
}

// Initializes the motor pin to output
void MotorFSM::init()
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

void MotorFSM::run()
{
    switch (currentState)
    {
    case TRIGGERED:
        // Reset timer
        timer = millis();
        // Turn on motor pin
        digitalWrite(_pin, HIGH);
        // Write new state
        currentState = RUNNING;
        break;
    case RUNNING:
        // Check runtime
        if (millis() - timer > _runtime) // If motor has been on for runtime
        {
            // Turn off motor
            digitalWrite(_pin, LOW);
            // Update State
            currentState = IDLE;
        }
        break;
    case IDLE:
        // Do nothing
        break;
    }
}

// Used to trigger the motor
void MotorFSM::trigger()
{
    currentState = TRIGGERED;
}

// Check if the motor is running;
bool MotorFSM::isRunning()
{
    return currentState == RUNNING;
}