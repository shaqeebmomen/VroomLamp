#include <MotorFSM.h>

/**
 * Constructor for finite state machine object
 * @param trigger function to run to trigger hardware (assumed to be a toggle)
 * @param shutoff function to turn off hardware (assumed to be toggle)
 * @param init function to initialize hardware (assumed that it does not also shut it off initially)
 * @param getSysTime function to get system time from last reset
 * @param runtime the length of time the motor should run for
 * 
 */
MotorFSM::MotorFSM(triggerFunc trigger, shutoffFunc shutoff, initFunc init, sysTimeFunc getSysTime, unsigned long runtime)
{
    _trigger = trigger;
    _shutoff = shutoff;
    _init = init;
    _runtime = runtime;
    _getSysTime = getSysTime;
}

// Initializes the motor pin to output
void MotorFSM::init()
{
    _init();
    _shutoff();
}

void MotorFSM::run()
{
    switch (currentState)
    {
    case TRIGGERED:
        // Reset timer
        _timer = _getSysTime();
        // Turn on motor pin
        _trigger();
        // Write new state
        currentState = RUNNING;
        break;
    case RUNNING:
        // Check runtime
        if (_getSysTime() - _timer > _runtime) // If motor has been on for runtime
        {
            // Turn off motor
            _shutoff();
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