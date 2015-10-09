#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "powertrace.h"

#include <stdio.h>

#include "common.h"

#define EVENT_TIMER_NULL	0
#define EVENT_TIMER_CONF	1

static uint8_t hop_nr = HOP_NR_INITIAL;
static uint8_t timer_event = EVENT_TIMER_NULL;
static struct etimer et_rnd_routing;

static uint8_t conf_seqn = SEQN_INITIAL;

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

		case HOP_CONF: ; // Set the hop_nr of the relay node
			struct init_packet *init_msg = (struct init_packet *) m;

			if (conf_seqn != init_msg->routing.seqn){
				hop_nr = HOP_NR_INITIAL;
				conf_seqn = init_msg->routing.seqn;
			}

			if (hop_nr > init_msg->routing.hop_nr + 1){
				hop_nr = init_msg->routing.hop_nr + 1;

				PROCESS_CONTEXT_BEGIN(&broadcast_process);
					etimer_set(&et_rnd_routing, (CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 5))/10);
				PROCESS_CONTEXT_END(&broadcast_process);

				timer_event = EVENT_TIMER_CONF;
			}

			printf("RN_R_SQN_%d\n", conf_seqn); // relay node - receive - sequence nr
			printf("Hop_nr: %d\n", hop_nr);

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
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();
	powertrace_start(CLOCK_SECOND * 0.2, "RN_P_");

	broadcast_open(&broadcast, 129, &broadcast_call);

	while(1) {

		PROCESS_WAIT_EVENT();

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// I will (have to) change the timer_event from mutex to switch
		// because I can have multiple timers in the same time
		// And I should also check if the timer expired
		//  - Attila
		
		if(ev == PROCESS_EVENT_TIMER && timer_event == EVENT_TIMER_CONF){
			struct init_packet init_msg;

			init_msg.type = HOP_CONF;
			init_msg.routing.seqn = conf_seqn;
			init_msg.routing.hop_nr = hop_nr;

			packetbuf_copyfrom(&init_msg, sizeof(struct init_packet));
			broadcast_send(&broadcast);

			timer_event = EVENT_TIMER_NULL;

			printf("RN_S_SQN_%d\n", conf_seqn); // relay node - send - sequence nr
			printf("Hop_nr: %d\n", hop_nr);
		}

	}

	PROCESS_END();
}
