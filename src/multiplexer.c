#include <mgos.h>
#include "gpio_pinout.h" // LOW, HIGH
#include "multiplexer.h" // functions
#include "multiplexer_pinout.h" // SELECT0, 1, 2, 3 (active high) 

// Sensor 0 (0000) - Temperature
void select_temperature() {
	mgos_gpio_write(SELECT0, LOW);
	mgos_gpio_write(SELECT1, LOW);
	mgos_gpio_write(SELECT2, LOW);
	mgos_gpio_write(SELECT3, LOW);
};

// Sensor 1 (0001) - LDR
void select_ldr() {
	mgos_gpio_write(SELECT0, HIGH);
	mgos_gpio_write(SELECT1, LOW);
	mgos_gpio_write(SELECT2, LOW);
	mgos_gpio_write(SELECT3, LOW);
};
