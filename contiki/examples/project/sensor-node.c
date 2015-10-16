/**
 * wake up every x seconds, generate five random bytes and put them in a 
 * buffer. Use a flag to signal when buffer is ready for reading.
 *
 */

#include "contiki.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "dev/cc2420/cc2420.h"

#include "powertrace.h"

#include <stdio.h> /* For printf() */
#include <stdbool.h>
#include <string.h>

#include "common.h"

#define SAMPLE_RATE 7
/* Parameters for determining the transmission rate. Each time a new transmission
* is made the sensor node will wait between RND_TIME_MIN and
* RND_TIME_MIN + RND_TIME_VAR, in milliseconds (ms).*/
#define SENSOR_SHORT_TX_MIN 3000
#define SENSOR_SHORT_TX_VAR 3000
#define SENSOR_LONG_TX_MIN 3000
#define SENSOR_LONG_TX_VAR 3000

#define BUFF_SIZE 25 

#define TIMEOUT_MAX 4
#define DEBUG


/*A element to be used in a Contiki list, containting the exact number of
 * sample data for one sensor_packet*/
struct sensor_elem{
    //the next pointer is needed because the struct is used in a Contiki List
    struct sensor_elem *next;
    uint8_t seqno;
    struct sensor_sample samples[SAMPLES_PER_PACKET];
};

/*---------------------------------------------------------------------------*/
PROCESS(sensor_process, "Sensor process");
PROCESS(transmit_process, "Transmit process");
AUTOSTART_PROCESSES(&sensor_process, &transmit_process);
/*---------------------------------------------------------------------------*/

static struct sensor_elem* current_elem = NULL;

MEMB(buff_memb, struct sensor_elem, BUFF_SIZE);
LIST(sensor_buff);

static uint8_t timeout_cnt = 0;

static uint8_t seqno = 0;

/*---------------------------------------------------------------------------*/

#ifdef DEBUG
void print_sensor_elem(struct sensor_elem *sp){
    int i;
    printf("PRINTING SENSOR PACKET\n");
    for (i = 0; i < SAMPLES_PER_PACKET; i++){
        printf("SAMPLE %d: ", i);
        print_sensor_sample(&sp->samples[i]);
    }
    printf("END OF SENSOR PACKET\n");
}

static void print_list(list_t l){
    struct sensor_elem *s;
    for (s = list_head(l); s != NULL; s = list_item_next(s)){
        print_sensor_elem(s);
    }
}
#endif

static void gen_sensor_sample(struct sensor_sample* sample){
    sample->temp = (uint8_t) random_rand() % 256;
    sample->heart = (uint8_t) random_rand() % 256;
    sample->behaviour = (uint8_t) random_rand() % 256;
}

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
#ifdef DEBUG
    /*struct sensor_packet *msg;
    msg = packetbuf_dataptr();
    printf("RECIEVED PACKET:\n");
    print_sensor_packet(msg);*/
	struct packet *m;
	m = packetbuf_dataptr();
	printf("RECIEVED PACKET: %d\n", m->type);
#endif
}

static bool tx_success = false;

static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
	struct packet *m;

	m = packetbuf_dataptr();

	if (m->type == ACK_SENSOR){
		struct ack_sensor_packet *asp = (struct ack_sensor_packet *) m;
		struct sensor_elem *se = list_head(sensor_buff);
		
		if (se !=NULL && asp->seqno == se->seqno){
			tx_success = true;
			printf("SN_R_SAC_SQN_%d\n", se->seqno);
		}
	}
}

static struct etimer et_tx;

static inline void set_tx_timer(bool is_in_range){

    if(is_in_range){
        #ifdef DEBUG
        printf("short timer\n");
        #endif
        etimer_set(&et_tx, (CLOCK_SECOND * SENSOR_SHORT_TX_MIN +
                    random_rand() % (CLOCK_SECOND * SENSOR_SHORT_TX_VAR))/1000);
    }else{
        #ifdef DEBUG
        printf("long timer\n");
        #endif
        etimer_set(&et_tx, (CLOCK_SECOND * SENSOR_LONG_TX_MIN +
                    random_rand() % (CLOCK_SECOND * SENSOR_LONG_TX_VAR))/1000);

    }
}

static uint8_t sample_cnt = 0; 

PROCESS_THREAD(sensor_process, ev, data)
{
    PROCESS_BEGIN();

    list_init(sensor_buff);
    memb_init(&buff_memb);

    current_elem = memb_alloc(&buff_memb);
    sample_cnt = 0;


    static struct etimer et;
    
    /* Generate a new sample values, store them in a list buffer, and sleep.
     * The buffer is used for caching purposes when out of coverage of the
     * relay nodes. The oldest value is overwritten if the buffer becomes
     * full.*/
    while(1){
        printf("process SENSOR\n");
        etimer_set(&et, CLOCK_SECOND*SAMPLE_RATE);
        PROCESS_WAIT_UNTIL(etimer_expired(&et));

        gen_sensor_sample(&current_elem->samples[sample_cnt]);
        
        #ifdef DEBUG
        printf("NEW DATA\n");
        print_sensor_sample(&current_elem->samples[sample_cnt]);
        #endif
        sample_cnt++;

        if (sample_cnt == SAMPLES_PER_PACKET){
            //The current element is full, make a new element.
            list_add(sensor_buff, current_elem);
            current_elem->seqno = seqno++;
            sample_cnt = 0;

            #ifdef DEBUG
            print_list(sensor_buff);
            #endif

            /*list is manipulated in two different user threads.
             * this is ok, since they are not preemptive.*/
            if (list_length(sensor_buff) == BUFF_SIZE){
                memb_free(&buff_memb, list_pop(sensor_buff));
            }

            if (etimer_expired(&et_tx)){
                PROCESS_CONTEXT_BEGIN(&transmit_process);
                set_tx_timer(true);
                PROCESS_CONTEXT_END(&transmit_process);
            }

            current_elem = memb_alloc(&buff_memb);
        }
    }
    
    PROCESS_END();
}

/*---------------------------------------------------------------------------*/

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

static struct sensor_elem *last_elem = NULL;

void exit_handler(struct broadcast_conn *bc, struct unicast_conn *uc){
    broadcast_close(bc);
    unicast_close(uc);
}

PROCESS_THREAD(transmit_process, ev, data)
{
    PROCESS_EXITHANDLER(exit_handler(&broadcast, &unicast);)
    PROCESS_BEGIN();

    cc2420_set_txpower(SN_TX_POWER);

    broadcast_open(&broadcast, 129, &broadcast_call);
    unicast_open(&unicast, 146, &unicast_callbacks);
    
    struct sensor_elem *elem = NULL;

    /* Transmit the oldest packet in the buffer. A new packet is selected
     * for transmission if the timeout counter reaches a maximum or if an
     * if an acknowledgement has been received.*/
    set_tx_timer(true);
    while(1){
        PROCESS_WAIT_UNTIL(etimer_expired(&et_tx));
        elem = list_head(sensor_buff);

        if (elem != NULL){
            if (tx_success){
                if (elem == last_elem){
                    printf("next packet\n");
                    memb_free(&buff_memb, list_pop(sensor_buff));
                    elem = list_head(sensor_buff);
                }
                timeout_cnt = 0;
                tx_success = false;
            }else{
                timeout_cnt++;
            }
            
            //transmit
            #ifdef DEBUG
            printf("SENSOR NODE TRANSMITTING\n");
            #endif
            static struct sensor_packet sp;
            sp.type = SENSOR_DATA;
            sp.seqno = elem->seqno;
            memcpy(&sp.samples, &elem->samples, sizeof(elem->samples));
            packetbuf_copyfrom(&sp, sizeof(struct sensor_packet));
            broadcast_send(&broadcast);

            printf("SN_S_SPA_ADDR_%d.%d_SQN_%d_DATA_",
                    linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
                    sp.seqno);

            int j;
            for (j = 0; j < SAMPLES_PER_PACKET; j++){
            	printf("%d %d %d ", 
            			sp.samples[j].temp,
                        sp.samples[j].heart, sp.samples[j].behaviour);
            }

            printf("\n");
            
            #ifdef DEBUG
            printf("SENSOR NODE TIMEOUT %d\n", timeout_cnt);
            #endif

            //Check when and if we will try to transmit again.
            last_elem = list_head(sensor_buff);
            if (last_elem != NULL){
                bool out_of_coverage = timeout_cnt >= TIMEOUT_MAX;
                if (out_of_coverage){
                    timeout_cnt = 0;
                }

                set_tx_timer(!out_of_coverage);
            }
            else{
                PROCESS_YIELD();
            }

        }

        PROCESS_YIELD();
    }

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
