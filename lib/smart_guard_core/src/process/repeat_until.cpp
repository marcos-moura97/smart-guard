#include "process/process.hpp"
#include <process/repeat_until.hpp>

process::RepeatUntil::RepeatUntil(functor_t action) noexcept:
    _action(std::move(action))
{

}

process::Update process::RepeatUntil::update(void)
{
    return (this->_action)();
}