/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mgos.h"
#include "mgos_mqtt.h"
#include "mgos_adc.h"
#include "mgos_pwm.h"

#include "gpio_pinout.h"

#include "m_mqtt.h"
#include "m_net.h"

enum mgos_app_init_result mgos_app_init(void) {

	LOG(LL_INFO, ("%s", "Main source initializing..."));

	mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, network_ev_handler, NULL);

	mgos_mqtt_add_global_handler(mqtt_connection_handler, NULL);

	mgos_gpio_set_mode(D0, MGOS_GPIO_MODE_OUTPUT); // D0 (Back LED)
	mgos_gpio_set_mode(led_pin, MGOS_GPIO_MODE_OUTPUT); // D1
	mgos_gpio_set_mode(pump_pin, MGOS_GPIO_MODE_OUTPUT); // D2
	mgos_gpio_set_mode(fan_pin, MGOS_GPIO_MODE_OUTPUT); // D3
	// mgos_gpio_set_mode(dht_pin, MGOS_GPIO_MODE_INPUT); // D4
	mgos_gpio_set_mode(SELECT0, MGOS_GPIO_MODE_OUTPUT);
	mgos_gpio_set_mode(SELECT1, MGOS_GPIO_MODE_OUTPUT);
	mgos_gpio_set_mode(SELECT2, MGOS_GPIO_MODE_OUTPUT);

	mgos_gpio_write(D0, HIGH);
	mgos_gpio_write(led_pin, HIGH);
	mgos_gpio_write(pump_pin, HIGH);
	mgos_gpio_write(fan_pin, HIGH);
	mgos_gpio_write(SELECT0, LOW); // D8
	mgos_gpio_write(SELECT1, LOW); // D7
	mgos_gpio_write(SELECT2, LOW); // D6

	mgos_adc_enable(0);
	
	return MGOS_APP_INIT_SUCCESS;
}
