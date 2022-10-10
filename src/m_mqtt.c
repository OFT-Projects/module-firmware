#include "mgos.h"
#include "m_mqtt.h"
#include "mgos_mqtt.h"
#include "mgos_adc.h"
#include "mgos_pwm.h"

#include "gpio_pinout.h"
#include "multiplexer_pinout.h"
#include "multiplexer.h"

void mqtt_control_handler(struct mg_connection *nc, const char *topic, int topic_len, const char *msg, int msg_len, void *ud) {
	
	// Change devices state
	LOG(LL_INFO, ("Control message received: %s", msg));
	
	int current_pump_state = mgos_gpio_read(D3);
	int current_led_state = mgos_gpio_read(D1);
	int current_fan_state = mgos_gpio_read(D2);
	
	int pump_state = 0;
	int led_state = 0;
	int fan_state = 0;
	float fan_intensity = 0;

	json_scanf(msg, strlen(msg), "{ fan_intensity:%f pump:%d, led:%d, fan:%d }", &fan_intensity, &pump_state, &led_state, &fan_state);

	if(current_pump_state != pump_state) { mgos_gpio_write(D3, pump_state); }	
	if(current_led_state != led_state) { mgos_gpio_write(D1, led_state); }
	if(current_fan_state != fan_state) { mgos_gpio_write(D2, fan_state); }
	if(mgos_pwm_set(D5, PWM_FREQ, fan_intensity)) {
		LOG(LL_INFO, ("PWM okay set to: %.1f", fan_intensity));
	} else {
		LOG(LL_INFO, ("PWM failed: %.1f", fan_intensity));
	}

}

char* fetch_topic(const char* device_id, const char* topic_str) {
	char *topic = malloc((1 + strlen(device_id) + strlen(topic_str)) * sizeof(char));
	strcpy(topic, "/");
	strcat(topic, device_id);
	strcat(topic, topic_str);	
	return topic;
}

void pub_ldr(void *arg) {

	// Publish LDR
	select_ldr();
	
	short int ldr = mgos_adc_read(0);
	char *ldr_msg = json_asprintf("{ldr:%hd}", ldr);
	char *ldr_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_ldr());	
	
	mgos_mqtt_pub(ldr_topic, ldr_msg, strlen(ldr_msg), 0, 0);
	
	free(ldr_msg);
	free(ldr_topic);
	
	LOG(LL_INFO, ("%s", "LDR message sent."));
	(void) arg;
}

void pub_temperature(void *arg) {
	
	// Publish temperature
	select_temperature();
	
	short int temperature = mgos_adc_read(0);
	char *temperature_msg = json_asprintf("{temperature:%hd}", temperature);
	char *temperature_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_temperature());
	
	mgos_mqtt_pub(temperature_topic, temperature_msg, strlen(temperature_msg), 0, 0);

	free(temperature_msg);
	free(temperature_topic);

	LOG(LL_INFO, ("%s: %hd", "Temperature message sent.", temperature));
	(void) arg;

}

void pub_rpm(void *arg) {

	// TODO: Fix this shit
	
	// Publish rpm	
	float fan_intensity = mgos_gpio_read(D5);
	short int fan_rpm = fan_intensity * fan_rpm_operation_range + min_fan_rpm;
		
	char *rpm_msg = json_asprintf("{rpm:%hd}", fan_rpm);
	char *rpm_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_rpm());
	
	mgos_mqtt_pub(rpm_topic, rpm_msg, strlen(rpm_msg), 0, 0);

	(void) arg;
}

void mqtt_connection_handler(struct mg_connection *c, int ev, void *p, void *user_data) {
	switch(ev) {
		case MG_EV_MQTT_CONNACK: {

			LOG(LL_INFO, ("%s", "MQTT: Connection established to broker"));

			mgos_gpio_write(D0, LOW);
			mgos_gpio_write(D1, LOW);
			mgos_gpio_write(D2, LOW);
			mgos_gpio_write(D3, LOW);
			mgos_gpio_write(SELECT0, LOW);
			mgos_gpio_write(SELECT1, LOW);
			mgos_gpio_write(SELECT2, LOW);
			mgos_gpio_write(SELECT3, LOW);

			// New connection publish
			char *connected_msg = json_asprintf("{id:%Q}", mgos_sys_config_get_device_id());
			mgos_mqtt_pub(mgos_sys_config_get_topics_new_conn(), connected_msg, strlen(connected_msg), 0, 0);
			free(connected_msg);

			// Subscribe to control topic
			char *control_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_control());	
			mgos_mqtt_sub(control_topic, mqtt_control_handler, NULL);	
			free(control_topic);
			
			// Timers for publishing
			mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_ldr(), MGOS_TIMER_REPEAT, pub_ldr, NULL);
			mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_temperature(), MGOS_TIMER_REPEAT, pub_temperature, NULL);
			mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_rpm(), MGOS_TIMER_REPEAT, pub_rpm, NULL);
		}
		case MG_EV_MQTT_SUBACK: { LOG(LL_INFO, ("%s", "MQTT: Subscribed to topic.")); break; }
		case MG_EV_MQTT_PUBACK: { LOG(LL_INFO, ("%s", "MQTT: Message published.")); break; }
		case MG_EV_MQTT_DISCONNECT: { LOG(LL_INFO, ("%s", "MQTT: Disconnected from broker.")); break; }
	}
}
