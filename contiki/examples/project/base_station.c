#include "contiki.h"
#include "net/rime/rime.h"

#include "common.h"

#include <stdio.h>

/* This holds the broadcast structure. */
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
/* Declare the broadcast process */
PROCESS(broadcast_process, "Broadcast process");

/* The AUTOSTART_PROCESSES() definition specifices what processes to
   start when this module is loaded.*/
AUTOSTART_PROCESSES(&broadcast_process);

/*---------------------------------------------------------------------------*/
/* This function is called whenever a broadcast message is received. */
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	struct broadcast_message *m;

	/* 	The packetbuf_dataptr() returns a pointer to the first data byte
		in the received packet. */
	m = packetbuf_dataptr();

	printf("Broadcast received\n");
}

/* This is where we define what function to be called when a broadcast
   is received. We pass a pointer to this structure in the
   broadcast_open() call below. */
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{
	static struct etimer et_init;
	static struct etimer et_reconfig;
	struct reconfig_packet reconf_msg;
	struct init_packet init_msg;

	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	broadcast_open(&broadcast, 129, &broadcast_call);

	// Send the first initialization message after 2 seconds
	etimer_set(&et_reconfig, CLOCK_SECOND * 2);

	while(1) {
		PROCESS_WAIT_EVENT();

		if(etimer_expired(&et_reconfig)){
			reconf_msg.type = HOP_RECONF;

			packetbuf_copyfrom(&reconf_msg, sizeof(struct reconfig_packet));
			broadcast_send(&broadcast);

			etimer_set(&et_reconfig, CLOCK_SECOND * 50);
			etimer_set(&et_init, CLOCK_SECOND * 7);

			printf("Reconfig sent!\n");
		}

		if(etimer_expired(&et_init)){
			init_msg.type = HOP_CONF;
			init_msg.routing.hop_nr = 0;

			packetbuf_copyfrom(&init_msg, sizeof(struct init_packet));
			broadcast_send(&broadcast);

			printf("Init sent!\n");
		}
	}

	PROCESS_END();
}
