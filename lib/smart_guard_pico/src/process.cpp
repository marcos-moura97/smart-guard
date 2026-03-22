#include <process.hpp>
#include <assert.h>

extern "C" bool process_update_timer_callback(repeating_timer_t* timer)
{
    // WARNING: There could be nullpointer exceptions happening here, but i want to test nullability first by using assertions.
    // Getting the behaviour of the destructor right could need a bit of trial and error (with my limited knowledge)
    // Update: this seems to work fine. No problems yet! :)
    assert(timer);
    assert(timer->user_data);

    void* process_untyped {timer->user_data};
    Process* process {static_cast<Process*>(process_untyped)};
    Process::Update result {process->update()};
    switch(result)
    {
        case Process::Update::done:
            return false;
        case Process::Update::pending:
            return true;
    }
    return false;
}