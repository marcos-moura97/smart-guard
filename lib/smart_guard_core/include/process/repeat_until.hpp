#pragma once

#include <functional>
#include <process/process.hpp>

namespace process
{
    class RepeatUntil : public Process
    {
    public:
        using functor_t = std::function<Update()>;

    private:
        functor_t _action;

    public:
        RepeatUntil(functor_t action) noexcept;

        virtual Update update(void) override;
    };
}