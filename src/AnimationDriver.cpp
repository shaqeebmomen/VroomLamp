#include <AnimationDriver.h>
namespace AnimationDriver
{

    AnimationDriver::AnimationDriver(animation initAnim, sysTimeFunc getSysTime)
    {
        _getSysTime = getSysTime;
        updateAnimation(initAnim);
    }

    void AnimationDriver::restart()
    {
        frameIndex = 0;
        lastStartTime = _getSysTime();
        currentTime = 0;
    }

    // Updates private timing variables
    void AnimationDriver::updateTime()
    {
        // Set current time since last animation start
        currentTime = _getSysTime() - lastStartTime;

        // If the current time has reached the next frame time
        if (currentTime > activeAnimation.frames[frameIndex + 1].time)
        {
            frameIndex++;
            // Frame index has passed the last frame
            if (frameIndex == activeAnimation.frameCount)
            {
                // Move last start time forward by one animation period
                lastStartTime += activeAnimation.time;
                // Trim the extra animation period from current time
                currentTime -= activeAnimation.time;
                // Reset Frame index
                frameIndex = 0;
            }
        }
    }

    // Interpolates b/w frames and updates current color state
    void AnimationDriver::interpolateColor()
    {

        animFrame *last = &activeAnimation.frames[frameIndex];
        animFrame *next = &activeAnimation.frames[frameIndex + 1];

        // Linearly interpolate between current and next R,G,B values
        for (uint8_t i = 0; i < 3; i++)
        {
            color[i] = last->color[i] + (next->color[i] - last->color[i]) / (next->time - last->time) * (currentTime - last->time);
        }
    }

    // Update the current animation and refresh index
    void AnimationDriver::updateAnimation(animation newAnim)
    {
        activeAnimation = newAnim;
        restart();
    }

    /**
     * Runs the Animation logic based on system time and calls hardware-aware function defined in parent scope
     * @param drivingFunc the function to drive hardware, arguments passed in are (uint8_t r, uint8_t g, uint8_t b)
     */
    void AnimationDriver::run(drivingFunc runLEDs)
    {
        // Update time-dependant variables
        updateTime();
        // Determine color state
        interpolateColor();
        // Pass color state to parent hardware-aware function
        runLEDs(color[0], color[1], color[2]);
    }

} // namespace AnimationDriver
