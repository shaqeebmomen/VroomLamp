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
    mode init(int);      // Initialize the shifter with a value
    mode run(int, bool); // FSM loop to run controller
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
