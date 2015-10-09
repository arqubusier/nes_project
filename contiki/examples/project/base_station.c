#include "contiki.h"
#include "net/rime/rime.h"
#include "powertrace.h"

#include "common.h"

#include <stdio.h>

/* This holds the broadcast structure. */
static struct broadcast_conn broadcast;

static uint8_t conf_seqn = SEQN_INITIAL;

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
	struct init_packet init_msg;

	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();
	powertrace_start(CLOCK_SECOND * 2, "BS_P_");

	broadcast_open(&broadcast, 129, &broadcast_call);

	// Send the first initialization message after 2 seconds
	etimer_set(&et_init, CLOCK_SECOND * 2);

	while(1) {
		PROCESS_WAIT_EVENT();

		if(etimer_expired(&et_init)){
			init_msg.type = HOP_CONF;
			init_msg.routing.seqn = ++conf_seqn;
			init_msg.routing.hop_nr = 0;

			packetbuf_copyfrom(&init_msg, sizeof(struct init_packet));
			broadcast_send(&broadcast);

			etimer_set(&et_init, CLOCK_SECOND * 20);

			printf("BS_S_SQN_%d\n", conf_seqn); // base station - sent - sequence nr
			printf("Init sent!\n");
		}
	}

	PROCESS_END();
}
