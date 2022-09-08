#include "mgos.h"
#include "m_net.h"

#include "multiplexer.h"

void network_ev_handler(int ev, void *evd, void *arg) {
		
	switch(ev) {
		case MGOS_NET_EV_DISCONNECTED: { LOG(LL_INFO, ("%s", "Network disconnected.")); break; }
		case MGOS_NET_EV_CONNECTING: { LOG(LL_INFO, ("%s", "Network connecting.")); break; }
		case MGOS_NET_EV_CONNECTED: { LOG(LL_INFO, ("%s", "Network connected.")); break; }
		case MGOS_NET_EV_IP_ACQUIRED: {	LOG(LL_INFO, ("%s", "Network IP acquired.")); break; }
	}

	(void) evd;
	(void) arg;
}
