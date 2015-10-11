#include "contiki.h"
#include "net/rime/rime.h"
#include "powertrace.h"
#include "lib/random.h"

#include "common.h"

#include <stdio.h>

/* This holds the broadcast structure. */
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

static uint8_t conf_seqn = SEQN_INITIAL;

static struct etimer et_rnd_ack;

static int flg_ack_agg = 0;

static struct ack_agg_packet ack_agg_fwd;
static linkaddr_t addr_ack_agg_fwd;

/*---------------------------------------------------------------------------*/
/* Declare the broadcast process */
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
		case AGGREGATED_DATA:
			;
			struct agg_packet *agg_data_tmp = (struct agg_packet *) m;
		
			int i, j;
			for (i = 0; i <= SENSOR_DATA_PER_PACKET; i++){
				printf("BS_R_DATA_ADDR %d.%d SEQNO %d DATA ", 
						agg_data_tmp->data[i].address.u8[0], agg_data_tmp->data[i].address.u8[1], agg_data_tmp->data[i].seqno);
				for (j = 0; j <= SAMPLES_PER_PACKET; j++){
					printf("%d %d %d ", 
							agg_data_tmp->data[i].samples[j].temp, agg_data_tmp->data[i].samples[j].heart, agg_data_tmp->data[i].samples[j].behaviour);
				}
				printf("\n");
			}
			
			ack_agg_fwd.type = ACK_AGG;
			linkaddr_copy(&ack_agg_fwd.address, &agg_data_tmp->address);
			ack_agg_fwd.seqno = agg_data_tmp->seqno;
	
			linkaddr_copy(&addr_ack_agg_fwd, from);
	
			flg_ack_agg = 1;
	
			if (etimer_expired(&et_rnd_ack)){
				PROCESS_CONTEXT_BEGIN(&unicast_process);
				etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
				PROCESS_CONTEXT_END(&unicast_process);
			}
			
			break;
	}
}

/* This is where we define what function to be called when a broadcast
   is received. We pass a pointer to this structure in the
   broadcast_open() call below. */
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static const struct unicast_callbacks unicast_call = {0};

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data)
{
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();
	powertrace_start(CLOCK_SECOND * 2, "BS_P_");

	static struct etimer et_init;
	struct init_packet init_msg;
	
	broadcast_open(&broadcast, 129, &broadcast_call);
	unicast_open(&unicast, 129, &unicast_call);

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

			printf("BS_S_CONF_SQN_%d\n", conf_seqn); // base station - sent - sequence nr
			printf("Init sent!\n");
		}
	}

	PROCESS_END();
}

PROCESS_THREAD(unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&unicast);)
    
  PROCESS_BEGIN();

  unicast_open(&unicast, 129, &unicast_call);

  while(1) {
	  PROCESS_WAIT_EVENT();
	  		
	  if (etimer_expired(&et_rnd_ack)){
		  if (flg_ack_agg){
		  				
			  packetbuf_copyfrom(&ack_agg_fwd, sizeof(struct ack_agg_packet));
			  unicast_send(&unicast, &addr_ack_agg_fwd);
			  
			  flg_ack_agg = 0;
			  etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
		  }
	  }
  }

  PROCESS_END();
}