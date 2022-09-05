void mqtt_connection_handler(struct mg_connection *c, int ev, void *p, void *user_data);
void mqtt_led_topic_handler(struct mg_connection *nc, const char *topic, int topic_len, const char *msg, int msg_len, void *ud);
void pub_potentiometer_cb(void *arg);
