#include <process.hpp>
#include <assert.h>

extern "C" bool process_update_timer_callback(repeating_timer_t* const timer)
{
    // WARNING: There could be nullpointer exceptions happening here, but i want to test nullability first by using assertions.
    // Getting the behaviour of the destructor right could need a bit of trial and error (with my limited knowledge)
    // TODO: make this more robust later
    assert(timer);
    assert(timer->user_data);

    void* system_untyped {timer->user_data};
    Process* system {static_cast<Process*>(system_untyped)};
    system->update();

    return true;
}