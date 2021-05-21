#include <ShifterFSM.h>

// Uncomment to activate debug statements
// #define DEBUG

#ifdef DEBUG
#include <Arduino.h>
#endif

ShifterFSM::ShifterFSM(sysTimeFunc getSysTime, unsigned long tSettle)
{
    _getSysTime = getSysTime;
    _tSettle = tSettle;
}

ShifterFSM::mode ShifterFSM::init(int val)
{
    // Set the initial state of the
    activeMode = getStickMode(val);
    return activeMode;
}

// Get the mode corresponding to a given read hall effect value
ShifterFSM::mode ShifterFSM::getStickMode(int val)
{
    switch (val)
    {
    case 0:
        return ShifterFSM::R;
        break;

    case 1:
        return ShifterFSM::ONE;
        break;
    case 2:
        return ShifterFSM::TWO;
        break;
    case 3:
        return ShifterFSM::THREE;
        break;
    case 4:
        return ShifterFSM::FOUR;
        break;
    case 5:
        return ShifterFSM::FIVE;
        break;
    case 6:
        return ShifterFSM::SIX;
        break;
    default:
        return ShifterFSM::NEUTRAL;
        break;
    }
}

ShifterFSM::mode ShifterFSM::run(int val, bool isMoving)
{
    if (isMoving)
    {
        currentState = MOVING;
    }

    switch (currentState)
    {
    case POLLING:
#ifdef DEBUG
        Serial.print("-POLLING");
        Serial.flush();

#endif
        polledMode = getStickMode(val);
        if (polledMode != activeMode)
        {

            currentState = ARMED;
            intentMode = polledMode;
#ifdef DEBUG
            Serial.print("-ARMING");
            Serial.print(";Intent: ");
            Serial.print(intentMode);
            Serial.flush();

#endif
            _timer = _getSysTime();
        }
        break;
    case MOVING:
#ifdef DEBUG
        Serial.print("-MOVING");
        Serial.flush();
#endif
        activeMode = NEUTRAL;
        currentState = POLLING;
        break;
    case ARMED:

        // Only look again after settle time has passed
        if ((_getSysTime() - _timer) > _tSettle)
        {
#ifdef DEBUG
            Serial.print("-SETTLED");
            Serial.flush();
#endif
            polledMode = getStickMode(val);
            // Check against the original change
            if (polledMode == intentMode) // If it matches, change to update
            {
                currentState = UPDATE;
            }
            else // No match, go back to polling
            {
                currentState = POLLING;
            }
        }
        break;

    case UPDATE:
#ifdef DEBUG
        Serial.println("--UPDATING");
        Serial.flush();

#endif
        activeMode = intentMode;
        if (activeMode != NEUTRAL)
        {
            updateFlag = true;
        }
        currentState = POLLING;
        break;
    } // End switch

#ifdef DEBUG
    Serial.print("Current: ");
    Serial.println(activeMode);
    Serial.flush();

#endif
    return activeMode;
}

bool ShifterFSM::getFlag()
{
    if (updateFlag)
    {
        updateFlag = false;
        return true;
    }
    else
    {
        return false;
    }
}