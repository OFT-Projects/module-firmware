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

#include "pinout.h"
#include "m_mqtt.h"
#include "m_net.h"

enum mgos_app_init_result mgos_app_init(void) {

	LOG(LL_INFO, ("%s", "Main source initializing..."));

	mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, network_ev_handler, NULL);

	mgos_mqtt_add_global_handler(mqtt_connection_handler, NULL);

	mgos_gpio_set_mode(D1, MGOS_GPIO_MODE_OUTPUT); // D1
	mgos_gpio_set_mode(D2, MGOS_GPIO_MODE_OUTPUT); // D2
	mgos_gpio_set_mode(D3, MGOS_GPIO_MODE_OUTPUT); // D3
	mgos_gpio_set_mode(D4, MGOS_GPIO_MODE_OUTPUT); // D4 (Front LED)
	mgos_gpio_set_mode(D5, MGOS_GPIO_MODE_OUTPUT); // D5
	mgos_gpio_set_mode(D0, MGOS_GPIO_MODE_OUTPUT); // D0 (Back LED)

	mgos_gpio_write(D1, HIGH); // D1
	mgos_gpio_write(D2, HIGH); // D2
	mgos_gpio_write(D3, HIGH); // D3
	mgos_gpio_write(D4, HIGH); // D4 (Front LED)
	mgos_gpio_write(D5, HIGH); // D5
	mgos_gpio_write(D0, LOW); // D0 (Back LED)
	
	mgos_adc_enable(0);
	
	return MGOS_APP_INIT_SUCCESS;
}
