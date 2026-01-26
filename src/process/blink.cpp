#include <hardware/gpio.h>
#include <process/blink.hpp>

Blink::Blink(void):
    status(false)
{

}

Process::Update Blink::update(void)
{
    this->status = !this->status;

    gpio_put(PICO_DEFAULT_LED_PIN, this->status);

    return Update::pending;
}