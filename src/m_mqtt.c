#include "mgos.h"
#include "m_mqtt.h"
#include "mgos_mqtt.h"
#include "mgos_adc.h"
#include "mgos_pwm.h"

#include "gpio_pinout.h"
#include "multiplexer.h"

int device_id = 0;

void walker(void *callback_data, const char *name, size_t name_len, const char *path, const struct json_token *token) {

	char component[10] = {'\0'};
	if(token->type == JSON_TYPE_STRING) {	
	
		strncpy(component, token->ptr, token->len);
		if(strcmp(component, "led") == 0) { device_id = 0; }
		else if(strcmp(component, "pump") == 0) { device_id = 1; }
		else if(strcmp(component, "fan") == 0) { device_id = 2;	} else { device_id = -1; }
	
	} else if(token->type == JSON_TYPE_NUMBER) {			
		
		char value_str[10] = {'\0'};
		strncpy(value_str, token->ptr, token->len);
		int value = atoi(value_str);
	
		LOG(LL_INFO, ("device id: %d, value: %d", device_id, value));

		if(device_id == 0) { mgos_gpio_write(led_pin, !value); }
		else if(device_id == 1) { mgos_gpio_write(pump_pin, !value); }
		else if(device_id == 2) { mgos_gpio_write(fan_pin, !value); }
	}	
}

void mqtt_control_handler(struct mg_connection *nc, const char *topic, int topic_len, const char *msg, int msg_len, void *ud) {	
	json_walk(msg, strlen(msg), walker, NULL);
}

char* fetch_topic(const char* device_id, const char* topic_str) {
	char *topic = malloc((1 + strlen(device_id) + strlen(topic_str)) * sizeof(char));
	strcpy(topic, "/");
	strcat(topic, device_id);
	strcat(topic, topic_str);	
	return topic;
}

void pub_ldr(void *arg) {

	select_ldr();
	
	short int ldr = mgos_adc_read(0);
	short int led_state = mgos_gpio_read(led_pin);

	LOG(LL_INFO, ("%d", ldr));

	char *ldr_msg = json_asprintf("{target_sensors: [{sensor: \"%s\", value:%hd}], current_state: {components: [{component: \"%s\", value: %hd}]}}", "ldr", ldr, "led", led_state);
	char *ldr_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_ldr());	
	
	mgos_mqtt_pub(ldr_topic, ldr_msg, strlen(ldr_msg), 0, 0);
	
	free(ldr_msg);
	free(ldr_topic);
	
	LOG(LL_INFO, ("%s", "LDR message sent."));
	(void) arg;
}

void pub_temperature(void *arg) {
	
	select_temperature();
	
	short int temperature = mgos_adc_read(0);
	short int fan_state = mgos_gpio_read(fan_pin);

	char *temperature_msg = json_asprintf("{target_sensors: [{sensor: \"%s\", value:%hd}], current_state: {components: [{component: \"%s\", value: %hd}]}}", "temperature", temperature, "fan", fan_state);	
	char *temperature_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_temperature());
	
	mgos_mqtt_pub(temperature_topic, temperature_msg, strlen(temperature_msg), 0, 0);

	free(temperature_msg);
	free(temperature_topic);

	LOG(LL_INFO, ("%s", "Temperature message sent."));
	(void) arg;

}

void mqtt_connection_handler(struct mg_connection *c, int ev, void *p, void *user_data) {
	switch(ev) {
		case MG_EV_MQTT_CONNACK: {

			LOG(LL_INFO, ("%s", "MQTT: Connection established to broker"));

			mgos_gpio_write(D0, LOW);
					
			mgos_gpio_write(led_pin, HIGH);
			mgos_gpio_write(pump_pin, HIGH);
			mgos_gpio_write(fan_pin, HIGH);		
			
			mgos_gpio_write(SELECT0, LOW);
			mgos_gpio_write(SELECT1, LOW);
			mgos_gpio_write(SELECT2, LOW);
			mgos_gpio_write(SELECT3, LOW);

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
			// mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_rpm(), MGOS_TIMER_REPEAT, pub_rpm, NULL);
		}
		case MG_EV_MQTT_SUBACK: { LOG(LL_INFO, ("%s", "MQTT: Subscribed to topic.")); break; }
		case MG_EV_MQTT_PUBACK: { LOG(LL_INFO, ("%s", "MQTT: Message published.")); break; }
		case MG_EV_MQTT_DISCONNECT: { LOG(LL_INFO, ("%s", "MQTT: Disconnected from broker.")); break; }
	}
}
