#include <WString.h>

// Stick Position Constants
// #define LIGHT_R 67
// #define LIGHT_1 284
// #define LIGHT_2 122
// #define LIGHT_3 175
// #define LIGHT_4 212
// #define LIGHT_5 435
// #define LIGHT_6 141

#define LIGHT_R 51
#define LIGHT_1 298
#define LIGHT_2 100
#define LIGHT_3 -19
#define LIGHT_4 220
#define LIGHT_5 435
#define LIGHT_6 -105
#define LIGHT_THRES 30

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
    mode init(int);           // Initialize the shifter with a value
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
