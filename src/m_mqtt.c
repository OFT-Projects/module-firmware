#include "mgos.h"
#include "m_mqtt.h"
#include "mgos_mqtt.h"
#include "mgos_adc.h"

#include "pinout.h"

void mqtt_control_handler(struct mg_connection *nc, const char *topic, int topic_len, const char *msg, int msg_len, void *ud) {
	
	// Change devices state
	LOG(LL_INFO, ("Control message received: %s", msg));
	
	int current_fan_state = mgos_gpio_read(D1);
	int current_pump_state = mgos_gpio_read(D2);
	int fan_state = 0;
	int pump_state = 0;

	json_scanf(msg, strlen(msg), "{ fan_state:%d, pump_state:%d }", &fan_state, &pump_state);
	
	if(current_fan_state != !fan_state) { mgos_gpio_write(D1, !fan_state); }
	if(current_pump_state != !pump_state) { mgos_gpio_write(D2, !pump_state); }
}

void pub_ldr(void *arg) {

	// Publish LDR
	char *ldr_msg = json_asprintf("{ldr:%d}", mgos_adc_read(0));
	
	char *ldr_topic = NULL;
	ldr_topic = malloc((1 + strlen(mgos_sys_config_get_device_id()) + strlen(mgos_sys_config_get_topics_ldr())) * sizeof(char));
	strcpy(ldr_topic, "/");
	strcat(ldr_topic, mgos_sys_config_get_device_id());
	strcat(ldr_topic, mgos_sys_config_get_topics_ldr());
	
	mgos_mqtt_pub(ldr_topic, ldr_msg, strlen(ldr_msg), 0, 0);
	
	free(ldr_msg);
	free(ldr_topic);
	
	LOG(LL_INFO, ("%s", "LDR message sent."));
	(void) arg;
}

void mqtt_connection_handler(struct mg_connection *c, int ev, void *p, void *user_data) {
	switch(ev) {
		case MG_EV_MQTT_CONNACK: {

			LOG(LL_INFO, ("%s", "MQTT: Connection established to broker"));
			
			mgos_gpio_write(D1, HIGH);
			mgos_gpio_write(D2, HIGH);
			mgos_gpio_write(D3, HIGH);
			mgos_gpio_write(D4, LOW);
			mgos_gpio_write(D5, HIGH);

			// New connection publish
			char *connected_msg = json_asprintf("{id:%Q, message:%Q}", mgos_sys_config_get_device_id(), "Connected.");
			mgos_mqtt_pub(mgos_sys_config_get_topics_new_conn(), connected_msg, strlen(connected_msg), 0, 0);
			free(connected_msg);

			// Subscribe to control topic
			char *control_topic = malloc((1 + strlen(mgos_sys_config_get_device_id()) + strlen(mgos_sys_config_get_topics_control())) * sizeof(char));
			strcpy(control_topic, "/");
			strcat(control_topic, mgos_sys_config_get_device_id());
			strcat(control_topic, mgos_sys_config_get_topics_control());
			
			mgos_mqtt_sub(control_topic, mqtt_control_handler, NULL);
			
			free(control_topic);
			
			// Timers for publishing
			mgos_set_timer(mgos_sys_config_get_topics_publish_interval_ms_ldr(), MGOS_TIMER_REPEAT, pub_ldr, NULL);
		}
		case MG_EV_MQTT_SUBACK: { LOG(LL_INFO, ("%s", "MQTT: Subscribed to topic.")); break; }
		case MG_EV_MQTT_PUBACK: { LOG(LL_INFO, ("%s", "MQTT: Message published.")); break; }
		case MG_EV_MQTT_DISCONNECT: { LOG(LL_INFO, ("%s", "MQTT: Disconnected from broker.")); break; }
	}
}
