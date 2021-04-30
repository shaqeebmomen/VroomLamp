#include <ShifterFSM.h>

// Uncomment to activate debug statements
// #define DEBUG
#ifdef DEBUG
#include <Arduino.h>
#endif

ShifterFSM::ShifterFSM(sysTimeFunc getSysTime, unsigned long tSettle)
{
    _getSysTime = _getSysTime;
    _tSettle = tSettle;
}

void ShifterFSM::init(int val)
{
    // Set the initial state of the
    activeMode = getStickMode(val);
}

// Get the mode corresponding to a given read hall effect value
ShifterFSM::mode ShifterFSM::getStickMode(int val)
{
    if (abs(val - LIGHT_R) < LIGHT_THRES)
    {
        return ShifterFSM::R;
    }
    else if (abs(val - LIGHT_1) < LIGHT_THRES)
    {
        return ShifterFSM::ONE;
    }
    else if (abs(val - LIGHT_2) < LIGHT_THRES)
    {
        return ShifterFSM::TWO;
    }
    else if (abs(val - LIGHT_3) < LIGHT_THRES)
    {
        return ShifterFSM::THREE;
    }
    else if (abs(val - LIGHT_4) < LIGHT_THRES)
    {
        return ShifterFSM::FOUR;
    }
    else if (abs(val - LIGHT_5) < LIGHT_THRES)
    {
        return ShifterFSM::FIVE;
    }
    else if (abs(val - LIGHT_6) < LIGHT_THRES)
    {
        return ShifterFSM::SIX;
    }
    else
    {
        return ShifterFSM::NEUTRAL;
    }
}

ShifterFSM::mode ShifterFSM::run(int val)
{
    switch (currentState)
    {
    case POLLING:
#ifdef DEBUG
        Serial.print("-POLLING");
#endif
        polledMode = getStickMode(val);
        if (polledMode != activeMode)
        {
#ifdef DEBUG
            Serial.print("-ARMING");
#endif
            currentState = ARMED;
            intentMode = polledMode;
            _timer = _getSysTime();
        }
        break;
    case ARMED:

        // Only look again after settle time has passed
        if ((_getSysTime() - _timer > _tSettle))
        {
#ifdef DEBUG
            Serial.print("-SETTLED");
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
#endif
        activeMode = intentMode;
        updateFlag = true;
        currentState = POLLING;
        break;
    }
#ifdef DEBUG
    Serial.print("Current: ");
    Serial.println(activeMode);
#endif
    _prevVal = val;
    return activeMode;
}

String ShifterFSM::getModeName()
{
    return getModeName(activeMode);
}

String ShifterFSM::getModeName(mode m)
{
    switch (m)
    {
    case R:
        return "Reverse";
        break;

    case ONE:
        return "One";
        break;

    case TWO:
        return "Two";
        break;

    case THREE:
        return "Three";
        break;

    case FOUR:
        return "Four";
        break;

    case FIVE:
        return "Five";
        break;

    case SIX:
        return "Six";
        break;

    case NEUTRAL:
        return "Neutral";
        break;

    default:
        return "Unknown";
        break;
    }
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