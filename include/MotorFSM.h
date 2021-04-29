#include <Arduino.h>
#include <stdint.h>


class MotorFSM
{
private:
    uint8_t _pin;
    unsigned long _runtime;
    enum states
    {
        TRIGGERED,
        RUNNING,
        IDLE,
    };
    states currentState;
    unsigned long timer;

public:
    MotorFSM(uint8_t, unsigned long);
    void init();
    void run();
    void trigger();
    bool isRunning();
};
