#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "powertrace.h"

#include <stdio.h>

#include "common.h"

// random timer to be set to a value between RND_TIME_MIN and RND_TIME_MIN + RND_TIME_VAR
// in milliseconds (ms)
#define RND_TIME_MIN 500
#define RND_TIME_VAR 500 

static int flg_conf = 0, flg_agg_fwd = 0, flg_agg_send = 0, flg_ack_sensor = 0, flg_ack_agg = 0;
static int overwrite_send = 1, overwrite_rcv = 1;

static struct etimer et_rnd, et_rnd_ack;

static uint8_t hop_nr = HOP_NR_INITIAL;

static uint8_t conf_seqn = SEQN_INITIAL;

static int spc = 0;       // Sensor packet counter

/* This holds the broadcast structure. */
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;
static struct agg_packet agg_data_buffer, agg_data_to_be_sent, agg_data_received;

/*---------------------------------------------------------------------------*/
/* Declare the broadcast and unicast processes */
PROCESS(broadcast_process, "Broadcast process");
PROCESS(unicast_process, "Unicast process");

/* The AUTOSTART_PROCESSES() definition specifices what processes to
   start when this module is loaded.*/
AUTOSTART_PROCESSES(&broadcast_process, &unicast_process);

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
				
			if (spc < SENSOR_DATA_PER_PACKET){
				struct sensor_packet *sp = (struct sensor_packet *) m; 		
			 	linkaddr_copy(&agg_data_buffer.data[spc].address, from);
			 	agg_data_buffer.data[spc].seqno = 0;
			 	memcpy(agg_data_buffer.data[spc].samples, sp->samples,  sizeof(sp->samples));
			 				
			 	printf("sensor_packet received, cnt: %d\n", spc);	 
			 	print_sensor_packet(sp);
			 	spc++;
			 	
			 	if (etimer_expired(&et_rnd_ack)){
			 		PROCESS_CONTEXT_BEGIN(&unicast_process);
			 		etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
			 		PROCESS_CONTEXT_END(&unicast_process);
			 	}
			}			
			
 			if (spc == SENSOR_DATA_PER_PACKET && overwrite_send){
                agg_data_to_be_sent = agg_data_buffer; //copy data to buffer

				// reinitialize the agg_data_buffer
				agg_data_buffer = (const struct agg_packet){ 0 };
                //reinitialize the Sensor packet counter
				spc = 0;
				overwrite_send = 0;
				flg_agg_send = 1;

				printf("agg_data_to_be_sent\n");   		
				if (etimer_expired(&et_rnd)){
					PROCESS_CONTEXT_BEGIN(&broadcast_process);
					etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
					PROCESS_CONTEXT_END(&broadcast_process);
				}
			};
 			
			break;

		case HOP_CONF: ; // Set the hop_nr of the relay node
			struct init_packet *init_msg = (struct init_packet *) m;

			if (conf_seqn != init_msg->routing.seqn){
				hop_nr = HOP_NR_INITIAL;
				conf_seqn = init_msg->routing.seqn;
			}

			if (hop_nr > init_msg->routing.hop_nr + 1){
				hop_nr = init_msg->routing.hop_nr + 1;

				flg_conf = 1;
			}

			printf("RN_R_SQN_%d\n", conf_seqn); // relay node - receive - sequence nr
			printf("Hop_nr: %d\n", hop_nr);
			if (etimer_expired(&et_rnd)){
				PROCESS_CONTEXT_BEGIN(&broadcast_process);
				etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
				PROCESS_CONTEXT_END(&broadcast_process);
			}

			break;

		case AGGREGATED_DATA:

			flg_agg_fwd = 1;
			flg_ack_agg = 1;
			
			if (etimer_expired(&et_rnd)){
				PROCESS_CONTEXT_BEGIN(&broadcast_process);
				etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
				PROCESS_CONTEXT_END(&broadcast_process);
			}
			if (etimer_expired(&et_rnd_ack)){
				PROCESS_CONTEXT_BEGIN(&unicast_process);
				etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
				PROCESS_CONTEXT_END(&unicast_process);
			}
			break;
	}
	
}

static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
	struct packet *m;

	m = packetbuf_dataptr();

	if (m->type == ACK_AGG){
		
	}
}


/* This is where we define what function to be called when a broadcast
   is received. We pass a pointer to this structure in the
   broadcast_open() call below. */
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static const struct unicast_callbacks unicast_callbacks = {recv_uc};

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(broadcast_process, ev, data)
{
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();
	powertrace_start(CLOCK_SECOND * 0.2, "RN_P_");

	broadcast_open(&broadcast, 129, &broadcast_call);

	while(1) {

		PROCESS_WAIT_EVENT();
		
		if (etimer_expired(&et_rnd)){
			if (flg_conf){
				struct init_packet init_msg;

				init_msg.type = HOP_CONF;
				init_msg.routing.seqn = conf_seqn;
				init_msg.routing.hop_nr = hop_nr;

				packetbuf_copyfrom(&init_msg, sizeof(struct init_packet));
				broadcast_send(&broadcast);

				printf("RN_S_SQN_%d\n", conf_seqn); // relay node - send - sequence nr
				printf("Hop_nr: %d\n", hop_nr);
			
				flg_conf = 0;
				etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
			}
			else if (flg_agg_fwd){
				
				flg_agg_fwd = 0;
				etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
			}
			else if (flg_agg_send){
				packetbuf_copyfrom(&agg_data_to_be_sent, sizeof(struct agg_packet));
				broadcast_send(&broadcast);
				
				flg_agg_send = 0;
				etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
			}
		}

	}

	PROCESS_END();
}


PROCESS_THREAD(unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&unicast);)
    
  PROCESS_BEGIN();

  unicast_open(&unicast, 129, &unicast_callbacks);

  while(1) {
	  PROCESS_WAIT_EVENT();
	  		
	  if (etimer_expired(&et_rnd_ack)){
		  if (flg_ack_agg){
		  				
			  flg_ack_agg = 0;
			  etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
		  }
		  else if (flg_ack_sensor){
		  				
			  etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
		  }
	  }
  }

  PROCESS_END();
}