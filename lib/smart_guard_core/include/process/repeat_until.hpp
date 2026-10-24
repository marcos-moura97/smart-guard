#pragma once

#include <functional>
#include <process/process.hpp>

namespace process
{
    class RepeatUntil : public Process
    {
    public:
        using functor_t = std::function<Update(duration_t)>;

    private:
        functor_t _action;

    public:
        RepeatUntil(functor_t action) noexcept;

        virtual Update update(duration_t dt) override;
    };
}