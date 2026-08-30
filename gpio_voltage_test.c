#include "pico/stdlib.h"

int main(void)
{
    gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);
    gpio_put(2, 1);

    while (true) {}
}