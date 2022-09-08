#include <mgos.h>
#include "gpio_pinout.h" // LOW, HIGH
#include "multiplexer.h" // functions
#include "multiplexer_pinout.h" // SELECT1, 2, 3

// Sensor 0 (000) - Temperature
void select_temperature() {
	mgos_gpio_write(SELECT3, LOW);
	mgos_gpio_write(SELECT2, LOW);
	mgos_gpio_write(SELECT1, HIGH);
};

// Sensor 1 (001) - LDR
void select_ldr() {
	mgos_gpio_write(SELECT3, LOW);
	mgos_gpio_write(SELECT2, HIGH);
	mgos_gpio_write(SELECT1, HIGH);
};
