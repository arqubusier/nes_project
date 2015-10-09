#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"

#include <stdio.h>

#include "common.h"

#define EVENT_TIMER_NULL	0
#define EVENT_TIMER_RECONF 	1
#define EVENT_TIMER_CONF	2
#define EVENT_TIMER_AGG         3
static uint8_t hop_nr = INITIAL_HOP_NR;
static uint8_t timer_event = EVENT_TIMER_NULL;
static struct etimer et_rnd_routing, et_rnd_reconfig, et_rnd_agg ;

static int spc=0;       // Sensor packet counter
/* This holds the broadcast structure. */
static struct broadcast_conn broadcast;
static struct agg_packet agg_data_buffer, agg_data_to_be_sent;

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
			;
			struct sensor_packet *sp = (struct sensor_packet *) m; 		
            linkaddr_copy(&agg_data_buffer.data[spc++].address, from);
			agg_data_buffer.data[spc++].seqno = 0;
			memcpy(agg_data_buffer.data[spc++].samples,
                    sp->samples,  sizeof(sp->samples));
				/* create a sensor_data LIST .
                 * copy the incoming sensor_sample packets into the list
                 * together with seqn and address.
                 * copy the full list into the agg_packet*/
				printf("sensor_packet received, cnt %d\n", spc);   				 
				
 			if (spc==SENSOR_DATA_PER_PACKET){
                agg_data_to_be_sent = agg_data_buffer; //copy data to buffer

				// reinitialize the agg_data_buffer
				agg_data_buffer = {0};
                //reinitialize the Sensor packet counter
				spc=0;

				PROCESS_CONTEXT_BEGIN(&broadcast_process);
					etimer_set(&et_rnd_agg,
                            (CLOCK_SECOND * 5 +
                             random_rand() % (CLOCK_SECOND * 5))/10);
				PROCESS_CONTEXT_END(&broadcast_process);
				timer_event = EVENT_TIMER_AGG;
				printf("agg_data_to_be_sent\n");   				 
				};
				
			break;

		case HOP_CONF: ; // Set the hop_nr of the relay node
			struct init_packet *init_msg = (struct init_packet *) m;

			if (hop_nr > init_msg->routing.hop_nr + 1){
				hop_nr = init_msg->routing.hop_nr + 1;

				PROCESS_CONTEXT_BEGIN(&broadcast_process);
					etimer_set(&et_rnd_routing, (CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 5))/10);
				PROCESS_CONTEXT_END(&broadcast_process);

				timer_event = EVENT_TIMER_CONF;
			}

			printf("Hop_nr: %d\n", hop_nr);

			break;

		case HOP_RECONF: // Reset the hop_nr of the relay node
			hop_nr = INITIAL_HOP_NR;

			PROCESS_CONTEXT_BEGIN(&broadcast_process);
				etimer_set(&et_rnd_reconfig, (CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 5))/10);
			PROCESS_CONTEXT_END(&broadcast_process);

			timer_event = EVENT_TIMER_RECONF;
			printf("Reconf rec\n");

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

	broadcast_open(&broadcast, 129, &broadcast_call);

	while(1) {

		PROCESS_WAIT_EVENT();
		
		if(ev == PROCESS_EVENT_TIMER && timer_event == EVENT_TIMER_RECONF){
			struct reconfig_packet reconf_msg;

			reconf_msg.type = HOP_RECONF;

			packetbuf_copyfrom(&reconf_msg, sizeof(struct reconfig_packet));
			broadcast_send(&broadcast);

			timer_event = EVENT_TIMER_NULL;
		}

		if(ev == PROCESS_EVENT_TIMER && timer_event == EVENT_TIMER_CONF){
			struct init_packet init_msg;

			init_msg.type = HOP_CONF;
			init_msg.routing.hop_nr = hop_nr;

			packetbuf_copyfrom(&init_msg, sizeof(struct init_packet));
			broadcast_send(&broadcast);

			timer_event = EVENT_TIMER_NULL;
		}

		if(ev == PROCESS_EVENT_TIMER && timer_event == EVENT_TIMER_AGG){
			
			packetbuf_copyfrom(&agg_data_to_be_sent, sizeof(struct agg_packet));
			broadcast_send(&broadcast);

			timer_event = EVENT_TIMER_NULL;
		}

	}

	PROCESS_END();
}
