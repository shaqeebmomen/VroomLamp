#include <WString.h>

// Stick Position Constants
// #define LIGHT_R 106
// #define LIGHT_1 398
// #define LIGHT_2 135
// #define LIGHT_3 339
// #define LIGHT_4 254
// #define LIGHT_5 463
// #define LIGHT_6 176

#define LIGHT_R 48
#define LIGHT_1 280
#define LIGHT_2 65
#define LIGHT_3 87
#define LIGHT_4 10
#define LIGHT_5 -337
#define LIGHT_6 -94
#define LIGHT_THRES 15

// Typedef for system time function
typedef unsigned long (*sysTimeFunc)();

class ShifterFSM
{

public:
    enum mode
    {
        R,
        ONE,
        TWO,
        THREE,
        FOUR,
        FIVE,
        SIX,
        NEUTRAL
    }; // lighting modes
    ShifterFSM(sysTimeFunc, unsigned long);
    void init(int);           // Initialize the shifter with a value
    mode run(int, bool);      // FSM loop to run controller
    String getModeName(mode); // Get the string representation of the passed in mode
    String getModeName();     // Get the string representation of the current mode
    bool getFlag();

private:
    enum states
    {
        POLLING,
        MOVING,
        ARMED,
        UPDATE
    };
    states currentState;                     // Current state of the controller
    mode getStickMode(int);                  // Get map passed value to stick mode
    mode activeMode, intentMode, polledMode; // Current Mode of system, the potential next mode, mode represented by the last sensor read
    sysTimeFunc _getSysTime;                 // Reference to parent scope function to read system time
    unsigned long _timer;                    // Timer used to track settling time
    unsigned long _tSettle;                  // Settle time for stick changes
    bool updateFlag = false;                 // Flag to check if the mode was recently changed
};
