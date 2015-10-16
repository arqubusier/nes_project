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
    static int overwrite_send = 1, overwrite_fwd = 1;

    static struct etimer et_rnd, et_rnd_ack;

    static uint8_t seqno = 0;
    static uint8_t hop_nr = HOP_NR_INITIAL;
    static uint8_t conf_seqn = SEQN_INITIAL;

    static int spc = 0;       // Sensor packet counter

    static struct agg_packet agg_data_buffer, agg_data_to_be_sent, agg_data_fwd;
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
            case SENSOR_DATA:
                    
                if (spc < SENSOR_DATA_PER_PACKET){
                    printf("sensor_packet received, cnt: %d\n", spc);	 

                    //save to buffer
                    struct sensor_packet *sp = (struct sensor_packet *) m; 		
                    
                    linkaddr_copy(&agg_data_buffer.data[spc].address, from);
                    agg_data_buffer.data[spc].seqno = sp->seqno;
                    memcpy(agg_data_buffer.data[spc].samples, sp->samples,  sizeof(sp->samples));
                    spc++;
                                
                    printf("Buffer counter: %d\n", spc-1);
                    print_sensor_packet(sp);

                    printf("RN_R_SPA_ADDR_%d.%d_SQN_%d", from->u8[0], from->u8[1], sp->seqno);
                    
                    // Send an acknolwedge to the sensor node we received the packet
                    // from.
                    struct sensor_ack_elem *se = memb_alloc(&sensor_ack_memb);
                    linkaddr_copy(&se->addr, from);
                    se->seqno = sp->seqno;
                    list_add(sensor_ack_list, se);
                    
                    if (etimer_expired(&et_rnd_ack)){
                        PROCESS_CONTEXT_BEGIN(&unicast_process);
                        etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
                        PROCESS_CONTEXT_END(&unicast_process);
                    }
                }			
                
                if (spc == SENSOR_DATA_PER_PACKET && overwrite_send){
                    agg_data_to_be_sent = agg_data_buffer; //copy data to buffer

                    agg_data_to_be_sent.seqno = seqno;
                    agg_data_to_be_sent.hop_nr = hop_nr;
                    linkaddr_copy(&agg_data_to_be_sent.address, &linkaddr_node_addr);
                    agg_data_to_be_sent.type = AGGREGATED_DATA;
                    
                    // reinitialize the agg_data_buffer
                    agg_data_buffer = (const struct agg_packet){ 0 };
                    
                    //reinitialize the Sensor packet counter
                    spc = 0;
                    overwrite_send = 0;
                    flg_agg_send = 1;

                    printf("agg_data_to_be_sent, type: %d\n", agg_data_to_be_sent.type);   		
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
                
                printf("RN_R_CON_SQN_%d_HOP_%d\n", conf_seqn, init_msg->routing.hop_nr); // relay node - receive - sequence nr
                printf("Hop_nr: %d\n", hop_nr);
                if (etimer_expired(&et_rnd)){
                    PROCESS_CONTEXT_BEGIN(&broadcast_process);
                    etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
                    PROCESS_CONTEXT_END(&broadcast_process);
                }

                break;

            case AGGREGATED_DATA:
                ;
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
                    
                    printf("RN_R_DAT_ADDR_%d.%d_SQN_%d\n", agg_data_tmp->address.u8[0], agg_data_tmp->address.u8[1], agg_data_tmp->seqno);
                
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
		
		printf("RN_R_ACK_ADDR_%d.%d_SQN_%d\n", ack_agg_rcv->address.u8[0], ack_agg_rcv->address.u8[1], ack_agg_rcv->seqno);
		
		if (flg_agg_send){
			if (linkaddr_cmp(&agg_data_to_be_sent.address, &ack_agg_rcv->address) && (agg_data_to_be_sent.seqno == ack_agg_rcv->seqno)){
				flg_agg_send = 0;
				overwrite_send = 1;
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
	//powertrace_start(CLOCK_SECOND * 0.2, "RN_P_");
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

				printf("RN_S_CON_SQN_%d_HOP_%d\n", init_msg.routing.seqn, init_msg.routing.hop_nr);
				printf("Hop_nr: %d\n", hop_nr);
			
				flg_conf = 0;
				etimer_set(&et_rnd, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
			}
			else if (flg_agg_send){
				
				packetbuf_copyfrom(&agg_data_to_be_sent, sizeof(struct agg_packet));
				broadcast_send(&broadcast);
				
				printf("RN_S_DAT_ADDR_%d.%d_SQN_%d\n", agg_data_to_be_sent.address.u8[0], agg_data_to_be_sent.address.u8[1], agg_data_to_be_sent.seqno);
				//flg_agg_send = 0;
				etimer_set(&et_rnd, (CLOCK_SECOND * 3 * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
			}
			else if (flg_agg_fwd){
				
				packetbuf_copyfrom(&agg_data_fwd, sizeof(struct agg_packet));
				broadcast_send(&broadcast);
				
				printf("RN_S_DAT_ADDR_%d.%d_SQN_%d\n", agg_data_to_be_sent.address.u8[0], agg_data_to_be_sent.address.u8[1], agg_data_to_be_sent.seqno);
				//flg_agg_fwd = 0;
				etimer_set(&et_rnd, (CLOCK_SECOND * 3 * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
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
			  etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
			  
			  printf("RN_S_ACK_ADDR_%d.%d_SQN_%d\n", addr_ack_agg_fwd.u8[0], addr_ack_agg_fwd.u8[1], ack_agg_fwd.seqno);
		  }
		  else if (list_length(sensor_ack_list) > 0){
                struct sensor_ack_elem *se = list_pop(sensor_ack_list);
                struct ack_sensor_packet pkt;
                pkt.type = ACK_SENSOR;
                pkt.seqno = se->seqno;

                printf("ACK sent %d.%d\n", se->addr.u8[0], se->addr.u8[1]);
                
                //transmit
                packetbuf_copyfrom(&pkt, sizeof(struct ack_sensor_packet));
                unicast_send(&unicast, &se->addr);
                memb_free(&sensor_ack_memb, se);
		  				
			  etimer_set(&et_rnd_ack, (CLOCK_SECOND * RND_TIME_MIN + random_rand() % (CLOCK_SECOND * RND_TIME_VAR))/1000);
			  
			  printf("RN_S_SAC_ADDR_%d.%d_SQN_%d\n", se->addr.u8[0], se->addr.u8[1], pkt.seqno);
		  }
	  }
  }

  PROCESS_END();
}
