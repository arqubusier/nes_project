#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"

#include <stdio.h>

#include "common.h"

static int hop_nr;
static struct etimer et;

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
	struct packet *m;

	/* 	The packetbuf_dataptr() returns a pointer to the first data byte
		in the received packet. */
	m = packetbuf_dataptr();

	switch (m->type){
		case SENSOR_DATA: //Vipul

			break;

		case HOP_CONF: //Attila
			if (hop_nr > m->hop_nr + 1){
				hop_nr = m->hop_nr + 1;
				PROCESS_CONTEXT_BEGIN(&broadcast_process);
				etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 5));
				PROCESS_CONTEXT_END(&broadcast_process);
			}
			break;

		case HOP_RECONF: //Attila

			break;

		case AGGREGATED_DATA:

			break;
	}

}

/* This is where we define what function to be called when a broadcast
   is received. We pass a pointer to this structure in the
   broadcast_open() call below. */
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{
	struct packet msg;

	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();

	hop_nr = 100;

	broadcast_open(&broadcast, 129, &broadcast_call);

	while(1) {

		PROCESS_WAIT_EVENT();

		if(etimer_expired(&et)){
			msg.hop_nr = hop_nr;

			packetbuf_copyfrom(&msg, sizeof(struct packet)); //??????????????????????????????????????
			broadcast_send(&broadcast);
		}

	}

	PROCESS_END();
}
