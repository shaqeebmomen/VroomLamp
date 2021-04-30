#include <stdint.h>

// Typedef for functions to turn on/off and initialize hardware
typedef void (*triggerFunc)();
typedef void (*shutoffFunc)();
typedef void (*initFunc)();
typedef unsigned long (*sysTimeFunc)();
class MotorFSM
{
private:
    triggerFunc _trigger;
    shutoffFunc _shutoff;
    initFunc _init;
    sysTimeFunc _getSysTime;

    unsigned long _runtime;
    enum states
    {
        TRIGGERED,
        RUNNING,
        IDLE,
    };
    states currentState;
    unsigned long _timer;

public:
    MotorFSM(triggerFunc, shutoffFunc, initFunc, sysTimeFunc, unsigned long);
    void init();
    void run();
    void trigger();
    bool isRunning();
};
