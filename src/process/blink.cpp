#include <hardware/gpio.h>
#include <process/blink.hpp>

Blink::Blink(void):
    status(false)
{

}

void Blink::update(void)
{
    this->status = !this->status;

    gpio_put(PICO_DEFAULT_LED_PIN, this->status);
}