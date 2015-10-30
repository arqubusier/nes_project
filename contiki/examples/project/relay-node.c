#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "powertrace.h"
#include "dev/cc2420/cc2420.h"

#include <stdio.h>

#include "common.h"

#define SENSOR_ACK_LIST_SIZE 10

struct sensor_ack_elem{
	struct sensor_ack_elem *next;
	linkaddr_t addr;
	uint8_t seqno;
};

static int flg_conf = 0, flg_agg_fwd = 0, flg_agg_send = 0, flg_ack_agg = 0;
static int overwrite_fwd = 1, lock_agg = 0;

static struct etimer et_rnd, et_rnd_ack, et_timeout;

static uint8_t seqno = 0;
static uint8_t hop_nr = HOP_NR_INITIAL;
static uint8_t conf_seqn = SEQN_INITIAL;

static int spc_tx = 0, spc_tmp = 0;       // Sensor packet counter

//struct sensor_data agg_data_buffer[SENSOR_DATA_PER_PACKET];
static struct agg_packet agg_pkt_to_be_sent, agg_data_fwd, agg_data_buffer;
static struct ack_agg_packet ack_agg_fwd;
static linkaddr_t addr_ack_agg_fwd;

/* This holds the broadcast structure. */
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

MEMB(sensor_ack_memb, struct sensor_ack_elem, 2 * SENSOR_DATA_PER_PACKET);
LIST(sensor_ack_list);

/*---------------------------------------------------------------------------*/
/* Declare the broadcast and unicast processes */
PROCESS(broadcast_process, "Broadcast process");
PROCESS(unicast_process, "Unicast process");
PROCESS(send_timeout_process, "Send timeout process");

/* The AUTOSTART_PROCESSES() definition specifices what processes to
       start when this module is loaded.*/
AUTOSTART_PROCESSES(&broadcast_process, &unicast_process,
		&send_timeout_process);

/*---------------------------------------------------------------------------*/
/* This function is called whenever a broadcast message is received. */
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
	struct packet *m;
	static int sensor_data_saved;

	/* 	The packetbuf_dataptr() returns a pointer to the first data byte
            in the received packet. */
	m = packetbuf_dataptr();

	switch (m->type){
	case SENSOR_DATA:
		/*
		 * 1. Add packet to agg_send if it is not locked
		 * 2. If it is locked, add packet to the agg_buff
		 * 3. When agg_send gets unlocked empty it, copy agg_buff into it and finally empty agg_buff
		 */

		// if one of the 2 buffers is under maniulation, lock the access to them to avoid data corruption
		if (lock_agg){
			break;
		}

		lock_agg = 1;
		sensor_data_saved = 0;

		struct sensor_packet *sp = (struct sensor_packet *) m;

		// if the sending buffer is not locked, we are writing into it
		if (!flg_agg_send){
			if (spc_tx == 0){
				//reset buffer parameters
				memset(&agg_pkt_to_be_sent, 0, sizeof(agg_pkt_to_be_sent));
				agg_pkt_to_be_sent.seqno = seqno++;
				linkaddr_copy(&agg_pkt_to_be_sent.address, &linkaddr_node_addr);
				agg_pkt_to_be_sent.type = AGGREGATED_DATA;
			}
			linkaddr_copy(&agg_pkt_to_be_sent.data[spc_tx].address, from);
			agg_pkt_to_be_sent.data[spc_tx].seqno = sp->seqno;
			memcpy(agg_pkt_to_be_sent.data[spc_tx].samples, sp->samples, sizeof(sp->samples));
			spc_tx++;

			sensor_data_saved = 1;
		}
		// if it is locked, then we write into the tmp buffer if it is not full yet
		else if (spc_tmp < SENSOR_DATA_PER_PACKET){
			if (spc_tmp == 0){
				//reset buffer parameters
				memset(&agg_data_buffer, 0, sizeof(agg_data_buffer));
				agg_data_buffer.seqno = seqno++;
				linkaddr_copy(&agg_data_buffer.address, &linkaddr_node_addr);
				agg_data_buffer.type = AGGREGATED_DATA;
			}

			linkaddr_copy(&agg_data_buffer.data[spc_tmp].address, from);
			agg_data_buffer.data[spc_tmp].seqno = sp->seqno;
			memcpy(agg_data_buffer.data[spc_tmp].samples,
					sp->samples, sizeof(sp->samples));
			spc_tmp++;

			sensor_data_saved = 1;
		}

		// if we successfully received a SENSOR_DATA  
		if (sensor_data_saved){
			// Send an acknolwedge to the sensor node we received the
			// packet from.
			struct sensor_ack_elem *se = memb_alloc(&sensor_ack_memb);
			linkaddr_copy(&se->addr, from);
			se->seqno = sp->seqno;
			list_add(sensor_ack_list, se);

			//reset timeout
			PROCESS_CONTEXT_BEGIN(&send_timeout_process);
			etimer_set(&et_timeout, CLOCK_SECOND * AGG_SEND_TIMEOUT);
			PROCESS_CONTEXT_END(&send_timeout_process);

			//set ack transmit timer
			if (etimer_expired(&et_rnd_ack)){
				PROCESS_CONTEXT_BEGIN(&unicast_process);
				etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_ACK_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_ACK_VAR))/1000);
				PROCESS_CONTEXT_END(&unicast_process);
			}
#ifdef DEBUG
			//printf("Buffer counter: %d\n", spc-1);
			print_sensor_packet(sp);
#endif
#ifdef EVALUATION
			printf("RN_R_SPA_ADDR_%d.%d_SQN_%d\n",
					from->u8[0], from->u8[1], sp->seqno);
#endif
		}

		// if the sending buffer is full and it is not yet in sending process, it should be sent
		if (spc_tx >= SENSOR_DATA_PER_PACKET && flg_agg_send == 0){

			flg_agg_send = 1;

			// reset counter
			spc_tx = 0;

			// stop timeout - we just filled up the buffer, we don't need timeout
			PROCESS_CONTEXT_BEGIN(&send_timeout_process);
			etimer_stop(&et_timeout);
			PROCESS_CONTEXT_END(&send_timeout_process);

			// set broadcast timer - send the agg packet
			if (etimer_expired(&et_rnd)){
				PROCESS_CONTEXT_BEGIN(&broadcast_process);
				etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_AGGDATA_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_AGGDATA_VAR))/1000);
				PROCESS_CONTEXT_END(&broadcast_process);
			}
		}

		// unlock the buffer modification
		lock_agg = 0;
		break;

	case HOP_CONF: 
		; // Set the hop_nr of the relay node
		struct init_packet *init_msg = (struct init_packet *) m;

		if (conf_seqn != init_msg->routing.seqn){
			hop_nr = HOP_NR_INITIAL;
			conf_seqn = init_msg->routing.seqn;
		}

		if (hop_nr > init_msg->routing.hop_nr + 1){
			hop_nr = init_msg->routing.hop_nr + 1;

			flg_conf = 1;
		}
#ifdef EVALUATION
		printf("RN_R_CON_SQN_%d_HOP_%d\n", conf_seqn, init_msg->routing.hop_nr); // relay node - receive - sequence nr
#endif
#ifdef DEBUG
		printf("Hop_nr: %d\n", hop_nr);
#endif
		if (etimer_expired(&et_rnd)){
			PROCESS_CONTEXT_BEGIN(&broadcast_process);
			etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_AGGDATA_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_AGGDATA_VAR))/1000);
			PROCESS_CONTEXT_END(&broadcast_process);
		}

		break;

	case AGGREGATED_DATA:
		;
#ifdef DEBUG
		printf("AGGREGATED DATA\n");
#endif
		struct agg_packet *agg_data_tmp = (struct agg_packet *) m;

		if (agg_data_tmp->hop_nr > hop_nr && overwrite_fwd){

			agg_data_fwd = *agg_data_tmp;
			agg_data_fwd.hop_nr = hop_nr;

			flg_agg_fwd = 1;
			overwrite_fwd = 0;

			ack_agg_fwd.type = ACK_AGG;
			linkaddr_copy(&ack_agg_fwd.address, &agg_data_tmp->address);
			ack_agg_fwd.seqno = agg_data_tmp->seqno;

			linkaddr_copy(&addr_ack_agg_fwd, from);

			flg_ack_agg = 1;
#ifdef EVALUATION
			printf("RN_R_DAT_SRC_%d.%d_ADDR_%d.%d_SQN_%d\n", 
					from->u8[0], from->u8[1],
					agg_data_tmp->address.u8[0], agg_data_tmp->address.u8[1], agg_data_tmp->seqno);
#endif

			if (etimer_expired(&et_rnd)){
				PROCESS_CONTEXT_BEGIN(&broadcast_process);
				etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_AGGDATA_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_AGGDATA_VAR))/1000);
				PROCESS_CONTEXT_END(&broadcast_process);
			}
			if (etimer_expired(&et_rnd_ack)){
				PROCESS_CONTEXT_BEGIN(&unicast_process);
				etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_ACK_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_ACK_VAR))/1000);
				PROCESS_CONTEXT_END(&unicast_process);
			}
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
		struct ack_agg_packet *ack_agg_rcv = (struct ack_agg_packet *) m;
#ifdef EVALUATION
		printf("RN_R_ACK_SRC_%d.%d_ADDR_%d.%d_SQN_%d\n", 
				from->u8[0], from->u8[1],
				ack_agg_rcv->address.u8[0], ack_agg_rcv->address.u8[1], ack_agg_rcv->seqno);
#endif

		if (flg_agg_send){
			if (lock_agg == 0 && linkaddr_cmp(&agg_pkt_to_be_sent.address, &ack_agg_rcv->address) && (agg_pkt_to_be_sent.seqno == ack_agg_rcv->seqno)){
				// agg packet was successfully delivered to the next hop, so we can overwrite it
				// we are going to modify the buffers, so lock them
				lock_agg = 1;

				spc_tx = spc_tmp;
				spc_tmp = 0;
				memcpy(&agg_pkt_to_be_sent, &agg_data_buffer, sizeof(agg_data_buffer));

				if (spc_tx >= SENSOR_DATA_PER_PACKET){

					// reset counter
					spc_tx = 0;

					// set broadcast timer - send the agg packet
					if (etimer_expired(&et_rnd)){
						PROCESS_CONTEXT_BEGIN(&broadcast_process);
						etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_AGGDATA_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_AGGDATA_VAR))/1000);
						PROCESS_CONTEXT_END(&broadcast_process);
					}
				}
				else{
					flg_agg_send = 0;
				}

				lock_agg = 0;
			}
		}
		else if (flg_agg_fwd){
			if (linkaddr_cmp(&agg_data_fwd.address, &ack_agg_rcv->address) && (agg_data_fwd.seqno == ack_agg_rcv->seqno)){
				flg_agg_fwd = 0;
				overwrite_fwd = 1;
			}
		}		
	}
}


/* This is where we define what function to be called when a broadcast
   is received. We pass a pointer to this structure in the
   broadcast_open() call below. */
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static const struct unicast_callbacks unicast_call = {recv_uc};

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(broadcast_process, ev, data)
{
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

			PROCESS_BEGIN();
#ifdef POWERTRACE
	powertrace_start(CLOCK_SECOND * TIME_POWERTRACE/1000, "RN_P_");
#endif
	cc2420_set_txpower(RN_TX_POWER);

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
#ifdef EVALUATION
				printf("RN_S_CON_SQN_%d_HOP_%d\n", init_msg.routing.seqn, init_msg.routing.hop_nr);
#endif
#ifdef DEBUG
				printf("Hop_nr: %d\n", hop_nr);
#endif

				flg_conf = 0;
				etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_ACK_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_ACK_VAR))/1000);
			}
			else if (flg_agg_send){

				lock_agg = 1;
				agg_pkt_to_be_sent.hop_nr = hop_nr;
				packetbuf_copyfrom(&agg_pkt_to_be_sent, sizeof(struct agg_packet));
				lock_agg = 0;
				broadcast_send(&broadcast);
#ifdef EVALUATION
				printf("RN_S_DAT_SRC_%d.%d_ADDR_%d.%d_SQN_%d\n", 
						linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
						agg_pkt_to_be_sent.address.u8[0], agg_pkt_to_be_sent.address.u8[1], agg_pkt_to_be_sent.seqno);
#endif
				//flg_agg_send = 0;
				etimer_set(&et_rnd, (CLOCK_SECOND * 3 * RND_TIME_AGGDATA_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_AGGDATA_VAR))/1000);
			}
			else if (flg_agg_fwd){

				packetbuf_copyfrom(&agg_data_fwd, sizeof(struct agg_packet));
				broadcast_send(&broadcast);
#ifdef EVALUATION
				printf("RN_S_DAT_SRC_%d.%d_ADDR_%d.%d_SQN_%d\n", 
						linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
						agg_data_fwd.address.u8[0], agg_data_fwd.address.u8[1], agg_data_fwd.seqno);
#endif
				//flg_agg_fwd = 0;
				etimer_set(&et_rnd, (CLOCK_SECOND * 3 * RND_TIME_AGGDATA_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_AGGDATA_VAR))/1000);
			}
		}

	}

	PROCESS_END();
}


PROCESS_THREAD(unicast_process, ev, data)
{
	PROCESS_EXITHANDLER(unicast_close(&unicast);)

		  PROCESS_BEGIN();

	unicast_open(&unicast, 146, &unicast_call);

	while(1) {
		PROCESS_WAIT_EVENT();

		if (etimer_expired(&et_rnd_ack)){
			if (flg_ack_agg){

				packetbuf_copyfrom(&ack_agg_fwd, sizeof(struct ack_agg_packet));
				unicast_send(&unicast, &addr_ack_agg_fwd);

				flg_ack_agg = 0;
				etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_ACK_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_ACK_VAR))/1000);
#ifdef EVALUATION
				printf("RN_S_ACK_DST_%d.%d_ADDR_%d.%d_SQN_%d\n", 
						addr_ack_agg_fwd.u8[0], addr_ack_agg_fwd.u8[1], 
						ack_agg_fwd.address.u8[0], ack_agg_fwd.address.u8[1], ack_agg_fwd.seqno);
#endif
			}
			else if (list_length(sensor_ack_list) > 0){
				struct sensor_ack_elem *se = list_pop(sensor_ack_list);
				struct ack_sensor_packet pkt;
				pkt.type = ACK_SENSOR;
				pkt.seqno = se->seqno;
#ifdef DEBUG
				printf("ACK sent %d.%d\n", se->addr.u8[0], se->addr.u8[1]);
#endif

				//transmit
				packetbuf_copyfrom(&pkt, sizeof(struct ack_sensor_packet));
				unicast_send(&unicast, &se->addr);
				memb_free(&sensor_ack_memb, se);

				etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_ACK_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_ACK_VAR))/1000);
#ifdef EVALUATION
				printf("RN_S_SAC_ADDR_%d.%d_SQN_%d\n", se->addr.u8[0], se->addr.u8[1], pkt.seqno);
#endif
			}
		}
	}

	PROCESS_END();
}


PROCESS_THREAD(send_timeout_process, ev, data)
{
	PROCESS_BEGIN();
	while (1){
#ifdef DEBUG
		printf("RELAY NODE TIMEOUT\n");
#endif
		PROCESS_YIELD();

		//set data aggregation send broadcast timer
		if (etimer_expired(&et_timeout)){

			flg_agg_send = 1;

			PROCESS_CONTEXT_BEGIN(&broadcast_process);
			etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_AGGDATA_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_AGGDATA_VAR))/1000);
			PROCESS_CONTEXT_END(&broadcast_process);
		}
	}
	PROCESS_END();
}
