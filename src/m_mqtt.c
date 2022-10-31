#include "mgos.h"
#include "m_mqtt.h"
#include "mgos_mqtt.h"
#include "mgos_adc.h"
#include "mgos_pwm.h"
#include "mgos_dht.h"

#include "gpio_pinout.h"
#include "multiplexer.h"


// Walkers variableint device_id = 0;
int device_id = 0;

int led_states[2];
int led_states_index;
int pump_states[2];
int pump_states_index;
int fan_states[2];
int fan_states_index;

int led_scheduled_time;
int pump_scheduled_time;
int fan_scheduled_time;

int reading_value = 0;
int reading_schedule_timer = 0;

void cbs_walker(void *callback_data, const char *name, size_t name_len, const char *path, const struct json_token *token) {
	char component[10] = {'\0'};

	if(strcmp(name, "value") == 0) { reading_value = 1; reading_schedule_timer = 0; }
	else if(strcmp(name, "schedule_timer") == 0) { reading_value = 0; reading_schedule_timer = 1; }

	if(token->type == JSON_TYPE_STRING) {
		strncpy(component, token->ptr, token->len);
		if(strcmp(component, "led") == 0) { device_id = 0; }
		else if(strcmp(component, "pump") == 0) { device_id = 1; }
		else if(strcmp(component, "fan") == 0) { device_id = 2;	}
		else { device_id = -1; }
	} else if(token->type == JSON_TYPE_NUMBER) {
		char value_str[10] = {'\0'};
		strncpy(value_stre, token-ptr, token->len);
		int value = atoi(value_str);
		if(reading_value) {			
			if(device_id == 0) {
				if (led_states_index < 2) { led_states[led_states_index] = value; }
			}
			else if(device_id == 1) {
				if (pump_states_index < 2) { pump_states[pump_states_index] = value; }
			}
			else if(device_id == 2) {
				if (fan_states_index < 2) { fan_states[fan_states_index] = value; }
			}
		}
		if(reading_schedule_timer) {	
			if(device_id == 0) { led_scheduled_time = value; }
			else if(device_id == 1) { pump_scheduled_time = value; }
			else if(device_id == 2) { fan_scheduled_time = value; }
		}
	}	
}
void mcsu_walker(void *callback_data, const char *name, size_t name_len, const char *path, const struct json_token *token) {

	char component[10] = {'\0'};
	if(token->type == JSON_TYPE_STRING) {	
		strncpy(component, token->ptr, token->len);
		if(strcmp(component, "value") == 0) { device_id = 0; }
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

void mqtt_control_handler_mcsu(struct mg_connection *nc, const char *topic, int topic_len, const char *msg, int msg_len, void *ud) {	
	json_walk(msg, strlen(msg), mcsu_walker, NULL);
}

void mqtt_control_handler_cbs(struct mg_connection *nc, const char *topic, int topic_len, const char *msg, int msg_len, void *ud) {	
	reading_value = 0;
	reading_schedule_timer = 0;
	
	led_states[2] = {'\0'};
	led_states_index = 0;
	pump_states[2] = {'\0'};
	pump_states_index = 0;
	fan_states[2] = {'\0'};
	fan_states_index = 0;

	reading_value = 0;
	reading_schedule_timer = 0;

	int led_schedule_timer = mgos_sys_config_get_led_backup_timer_ms();
	int pump_schedule_timer	= mgos_sys_config_get_pump_backup_timer_ms();
	int fan_schedule_timer = mgos_sys_config_get_fan_backup_timer_ms();
	
	json_walk(msg, strlen(msg), cbs_walker, NULL);
	
	mgos_sys_config_set_led_backup_timer_ms(led_schdule_timer);
	mgos_sys_config_set_pump_backup_timer_ms(pump_schdule_timer);
	mgos_sys_config_set_fan_backup_timer_ms(fan_schdule_timer);
	
	char *err = NULL;
	save_cfg(&mgos_sys_config, &err);
	free(err);
}

char* fetch_topic(const char* device_id, const char* topic_str) {
	char *topic = malloc((1 + strlen(device_id) + strlen(topic_str)) * sizeof(char));
	strcpy(topic, "/");
	strcat(topic, device_id);
	strcat(topic, topic_str);	
	return topic;
}

char* fetch_message(char *ldr_sensor_title, char *water_temperature_sensor_title, char *dht_sensor_title, short int ldr_value, short int water_temperature_value, short int dht_value, short int led_state, short int pump_state, short int fan_state) {	
	char *message = malloc(300 * sizeof(char));
	sprintf(message, "{\"target_sensors\": [{\"sensor\": \"%s\", \"value\":%hd}, {\"sensor\": \"%s\", \"value\":%hd}, {\"sensor\": \"%s\", \"value\":%hd}], \"current_state\": {\"components\": [{\"component\": \"%s\", \"value\": %hd}, {\"component\": \"%s\", \"value\": %hd}, {\"component\": \"%s\", \"value\": %hd}]}}", ldr_sensor_title, ldr_value, water_temperature_sensor_title, water_temperature_value, dht_sensor_title, dht_value, "led", !led_state, "pump", !pump_state, "fan", !fan_state);
	return message;
}

int pub_dht_now = 1;
void pub_payload(void *arg) {
	
	char *ldr_sensor_title = "ldr";
	char *water_temperature_sensor_title = "water_temperature";
	char *dht_sensor_title = pub_dht_now == 1 ? "humidity" : "temperature";
	
	select_ldr();
	short int ldr = mgos_adc_read(0);
	
	select_water_temperature();	
	short int water_temperature = mgos_adc_read(0);
	
	struct mgos_dht *dht_sensor = mgos_dht_create(D5, DHT11);
	short int dht;
	if(pub_dht_now) {
		dht = (short int) mgos_dht_get_humidity(dht_sensor);
		pub_dht_now = 0;
	} else {
		dht = (short int) mgos_dht_get_temp(dht_sensor);	
		pub_dht_now = 1;
	}
	
	short int led_state = mgos_gpio_read(led_pin);
	short int pump_state = mgos_gpio_read(pump_pin);
	short int fan_state = mgos_gpio_read(fan_pin);

	char *topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_tms());
	char *payload = fetch_message(ldr_sensor_title, water_temperature_sensor_title, dht_sensor_title, ldr, water_temperature, dht, led_state, pump_state, fan_state);
	
	mgos_mqtt_pub(topic, payload, strlen(payload), 0, 0);

	free(topic);
	free(payload);
	
	LOG(LL_INFO, ("%s", "Payload sent"));

	(void) arg;
}

void mqtt_connection_handler(struct mg_connection *c, int ev, void *p, void *user_data) {
	switch(ev) {
		case MG_EV_MQTT_CONNACK: {

			LOG(LL_INFO, ("%s", "MQTT: Connection established to broker"));

			mgos_gpio_write(D0, LOW);
					
			mgos_gpio_write(SELECT0, LOW);
			mgos_gpio_write(SELECT1, LOW);
			mgos_gpio_write(SELECT2, LOW);
			
			char *connected_msg = json_asprintf("{id:%Q}", mgos_sys_config_get_device_id());
			mgos_mqtt_pub(mgos_sys_config_get_topics_new_conn(), connected_msg, strlen(connected_msg), 0, 0);
			free(connected_msg);

			// Subscribe to control and backup topics
			char *mcsu_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_mcsu());	
			char *cbs_topic = fetch_topic(mgos_sys_config_get_device_id(), mgos_sys_config_get_topics_cbs());
			mgos_mqtt_sub(mcsu_topic, mqtt_control_handler_mcsu, NULL);	
			mgos_mqtt_sub(cbs_topic, mqtt_control_handler_cbs, NULL);
			free(control_topic);
			free(cbs_topic);
		
			// Timers for publishing
			mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_payload(), MGOS_TIMER_REPEAT, pub_payload, NULL);
		}
		case MG_EV_MQTT_SUBACK: { LOG(LL_INFO, ("%s", "MQTT: Subscribed to topic.")); break; }
		case MG_EV_MQTT_PUBACK: { LOG(LL_INFO, ("%s", "MQTT: Message published.")); break; }
		case MG_EV_MQTT_DISCONNECT: { 
			mgos_gpio_write(D0, HIGH);
			LOG(LL_INFO, ("%s", "MQTT: Disconnected from broker.")); break; 
		}
	}
}
