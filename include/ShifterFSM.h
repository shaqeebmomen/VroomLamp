#include <Arduino.h>

// Stick Position Constants
#define LIGHT_R 460
#define LIGHT_1 197
#define LIGHT_2 -366
#define LIGHT_3 0
#define LIGHT_4 -253
#define LIGHT_5 -60
#define LIGHT_6 -170
#define LIGHT_THRES 30

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
    ShifterFSM(unsigned long);
    void init(int);           // Initialize the shifter with a value
    mode run(int);            // FSM loop to run controller
    String getModeName(mode); // Get the string representation of the passed in mode
    String getModeName();     // Get the string representation of the current mode
    bool getFlag();

private:
    enum states
    {
        POLLING,
        ARMED,
        UPDATE
    };
    states currentState;                     // Current state of the controller
    mode getStickMode(int);                  // Get map passed value to stick mode
    mode activeMode, intentMode, polledMode; // Current Mode of system, the potential next mode, mode represented by the last sensor read
    unsigned long timer;                     // Timer used to track settling time
    unsigned long _tSettle;                  // Settle time for stick changes
    bool updateFlag = false;                 // Flag to check if the mode was recently changed
};
