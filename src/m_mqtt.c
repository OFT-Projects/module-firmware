#include "mgos.h"
#include "m_mqtt.h"
#include "mgos_mqtt.h"
#include "mgos_adc.h"
#include "mgos_pwm.h"
#include "mgos_dht.h"

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

char* fetch_message(char* sensor, const int value) {

	short int led_state = mgos_gpio_read(led_pin);
	short int pump_state = mgos_gpio_read(pump_pin);
	short int fan_state = mgos_gpio_read(fan_pin);
	
	char *message = malloc(300 * sizeof(char));
	sprintf(message, "{\"target_sensors\": [{\"sensor\": \"%s\", \"value\":%hd}], \"current_state\": {\"components\": [{\"component\": \"%s\", \"value\": %hd}, {\"component\": \"%s\", \"value\": %hd}, {\"component\": \"%s\", \"value\": %hd}]}}", sensor, value, "led", !led_state, "pump", !pump_state, "fan", !fan_state);
	return message;
}

void pub_ldr(void *arg) {

	select_ldr();	
	short int ldr = mgos_adc_read(0);

	char *message = fetch_message("ldr", ldr);
	char *ldr_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_tms());	

	mgos_mqtt_pub(ldr_topic, message, strlen(message), 0, 0);
	
	free(message);
	free(ldr_topic);
	
	LOG(LL_INFO, ("%s", "LDR message sent."));
	(void) arg;
}

void pub_water_temperature(void *arg) {
	
	select_water_temperature();	
	short int water_temperature = mgos_adc_read(0);

	char *water_temperature_msg = fetch_message("water_temperature", water_temperature);
	char *water_temperature_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_tms());
	
	mgos_mqtt_pub(water_temperature_topic, water_temperature_msg, strlen(water_temperature_msg), 0, 0);

	free(water_temperature_msg);
	free(water_temperature_topic);

	LOG(LL_INFO, ("%s", "Water temperature message sent."));
	(void) arg;

}

int pub_temperature_now = 1;
int pub_humidity_now = 0;

void pub_temperature(void *arg) {

	if(pub_temperature_now == 1) {

		// Initialize DHT22 sensor
		struct mgos_dht *dht_sensor = mgos_dht_create(D5, DHT11); 
	
		int temperature = (int) mgos_dht_get_temp(dht_sensor);	

		char *temperature_msg = fetch_message("temperature", temperature);
		char *temperature_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_tms());
		
		mgos_mqtt_pub(temperature_topic, temperature_msg, strlen(temperature_msg), 0, 0);

		free(temperature_msg);
		free(temperature_topic);

		LOG(LL_INFO, ("%s:%d", "Temperature message sent", temperature));
		
		pub_temperature_now = 0;
	
		mgos_dht_close(dht_sensor);
	} else {
		pub_temperature_now = 1;
	}

	(void) arg;
}

void pub_humidity(void *arg) {

	
	if(pub_humidity_now == 1) {

		// Initialize DHT22 sensor
		struct mgos_dht *dht_sensor = mgos_dht_create(D5, DHT11); 	
		
		LOG(LL_INFO, ("%s", "Humidity"));
		int humidity = (int) mgos_dht_get_humidity(dht_sensor);	
		
		char *humidity_msg = fetch_message("humidity", humidity);
		char *humidity_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_tms());
		
		mgos_mqtt_pub(humidity_topic, humidity_msg, strlen(humidity_msg), 0, 0);

		free(humidity_msg);
		free(humidity_topic);
		
		LOG(LL_INFO, ("%s:%d", "Humidity message sent", humidity));
		
		pub_humidity_now = 0;
		
		mgos_dht_close(dht_sensor);
	} else {
		pub_humidity_now = 1;
	}
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

			char *connected_msg = json_asprintf("{id:%Q}", mgos_sys_config_get_device_id());
			mgos_mqtt_pub(mgos_sys_config_get_topics_new_conn(), connected_msg, strlen(connected_msg), 0, 0);
			free(connected_msg);

			// Subscribe to control topic
			char *control_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_mcsu());	
			mgos_mqtt_sub(control_topic, mqtt_control_handler, NULL);	
			free(control_topic);
		
			// Timers for publishing
			mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_ldr(), MGOS_TIMER_REPEAT, pub_ldr, NULL);
			mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_water_temperature(), MGOS_TIMER_REPEAT, pub_water_temperature, NULL);
			mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_temperature(), MGOS_TIMER_REPEAT, pub_temperature, NULL);
			mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_humidity(), MGOS_TIMER_REPEAT, pub_humidity, NULL);
			// mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_rpm(), MGOS_TIMER_REPEAT, pub_rpm, NULL);
		}
		case MG_EV_MQTT_SUBACK: { LOG(LL_INFO, ("%s", "MQTT: Subscribed to topic.")); break; }
		case MG_EV_MQTT_PUBACK: { LOG(LL_INFO, ("%s", "MQTT: Message published.")); break; }
		case MG_EV_MQTT_DISCONNECT: { 
			mgos_gpio_write(D0, HIGH);
			LOG(LL_INFO, ("%s", "MQTT: Disconnected from broker.")); break; 
		}
	}
}
