#include <mgos.h>
#include "gpio_pinout.h" // LOW, HIGH  SELECT0, 1, 2, 3 
#include "multiplexer.h" // functions

// Sensor 0 (0000) - Temperature
void select_temperature() {
	mgos_gpio_write(SELECT0, LOW);
	mgos_gpio_write(SELECT1, LOW);
	mgos_gpio_write(SELECT2, LOW);
	mgos_gpio_write(SELECT3, LOW);
};

// Sensor 1 (0011) - LDR
void select_ldr() {
	mgos_gpio_write(SELECT0, HIGH);
	mgos_gpio_write(SELECT1, HIGH);
	mgos_gpio_write(SELECT2, LOW);
	mgos_gpio_write(SELECT3, LOW);
};
