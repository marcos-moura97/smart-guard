#pragma once

namespace process
{
    /**
    Wether or not the process has completed.
    */
    enum class Update
    {
        pending,
        done
    };
    
    /**
    A process which provides an update method that can be ran at a regular interval.

    This class can be fed into a Timer class which will run it at a regular given interval using the Raspberry Pico hardware-timers.
    */
    class Process
    {
    public:
        /// Action to be performed each tick by the Timer
        virtual Update update(void) = 0;

        virtual ~Process() = default;
    };
}