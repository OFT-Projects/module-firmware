#include "mgos.h"
#include "m_net.h"

#include "gpio_pinout.h"

int off_led_timer_id = -1;
int off_pump_timer_id = -1;
int off_fan_timer_id = -1;

void off_led_timer(void *arg) {
	int led_state = mgos_gpio_read(led_pin);
	mgos_gpio_write(led_pin, !led_state);
	(void) arg;
}

void off_pump_timer(void *arg) {
	int pump_state = mgos_gpio_read(pump_pin);
	mgos_gpio_write(pump_pin, !pump_state);
	(void) arg;
}

void off_fan_timer(void *arg) {
	int fan_state = mgos_gpio_read(fan_pin);
	mgos_gpio_write(fan_pin, !fan_state);
	(void) arg;
}

void network_ev_handler(int ev, void *evd, void *arg) {
		
	switch(ev) {
		case MGOS_NET_EV_DISCONNECTED: { 
			LOG(LL_INFO, ("%s", "Network disconnected."));
			if(off_led_timer_id == -1 && off_pump_timer_id == -1 && off_fan_timer_id == -1) {
				off_led_timer_id = mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_off_led(), MGOS_TIMER_REPEAT, off_led_timer, NULL);
				off_pump_timer_id = mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_off_pump(), MGOS_TIMER_REPEAT, off_pump_timer, NULL);
				off_fan_timer_id = mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_off_fan(), MGOS_TIMER_REPEAT, off_fan_timer, NULL); 
				LOG(LL_INFO, ("Timers: %d, %d and %d initialized.", off_led_timer_id, off_pump_timer_id, off_fan_timer_id)); 
			}
			break; 
		}
		case MGOS_NET_EV_CONNECTING: { LOG(LL_INFO, ("%s", "Network connecting.")); break; }
		case MGOS_NET_EV_CONNECTED: {
			mgos_clear_timer(off_led_timer_id); 
			mgos_clear_timer(off_pump_timer_id); 
			mgos_clear_timer(off_fan_timer_id);
			LOG(LL_INFO, ("Timers: %d, %d and %d terminated.", off_led_timer_id, off_pump_timer_id, off_fan_timer_id)); 
			off_led_timer_id = -1;
			off_pump_timer_id = -1;
			off_fan_timer_id = -1;
			mgos_gpio_write(led_pin, HIGH);
			mgos_gpio_write(pump_pin, HIGH);
			mgos_gpio_write(fan_pin, HIGH);		
			LOG(LL_INFO, ("%s", "Network connected.")); 
			break; 
		}
		case MGOS_NET_EV_IP_ACQUIRED: {	LOG(LL_INFO, ("%s", "Network IP acquired.")); break; }
	}

	(void) evd;
	(void) arg;
}
