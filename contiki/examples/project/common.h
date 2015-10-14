#ifndef COMMON_H
#define COMMON_H

#define MAX_PACKET_SIZE 	128
#define SAMPLES_PER_PACKET 	3

#define INITIAL_HOP_NR 		100
#define SENSOR_DATA_PER_PACKET  4
#define SEQN_INITIAL		0
#define HOP_NR_INITIAL 		100

#define SENSOR_DATA 		0
#define HOP_CONF 			1
#define AGGREGATED_DATA 	3
#define ACK_AGG			 	4
#define ACK_SENSOR			5

// random timer to be set to a value between RND_TIME_MIN and RND_TIME_MIN + RND_TIME_VAR
// in milliseconds (ms)
#define RND_TIME_MIN 500
#define RND_TIME_VAR 500 

#define SN_TX_POWER 15
#define RN_TX_POWER 31
#define BS_TX_POWER 31

struct packet{
    uint8_t type;
    void *data;
};

struct sensor_sample{
    uint8_t temp;
    uint8_t heart;
    uint8_t behaviour;
};

struct sensor_packet{          // for receiving the packets
    uint8_t type;
    //for identifying the order of measurement sample at a sensor node       
    uint8_t seqno;
    struct sensor_sample samples[SAMPLES_PER_PACKET];
};

struct routing_init{
	uint8_t seqn;
	uint8_t hop_nr;
};

struct init_packet{
    uint8_t type;
    struct routing_init routing;
};

struct sensor_data{
    linkaddr_t address;
    uint8_t seqno;
    struct sensor_sample samples[SAMPLES_PER_PACKET];
};

struct agg_packet{
     uint8_t type;
     linkaddr_t address;
     uint8_t seqno;
     uint8_t hop_nr;
     struct sensor_data data[SENSOR_DATA_PER_PACKET];
};

struct ack_agg_packet{
	uint8_t type;
	linkaddr_t address;
	uint8_t seqno;
};

struct ack_sensor_packet{
	uint8_t type;
	uint8_t seqno;
};

void print_sensor_sample(struct sensor_sample *s){
    printf("T: %d, H: %d, B: %d\n", s->temp, s->heart, s->behaviour);
}

void print_sensor_packet(struct sensor_packet *p){
    int i;
    printf("PRINTING SENSOR PACKET\n");
    printf("seqno: %d\n", p->seqno);
    for (i = 0; i < SAMPLES_PER_PACKET; i++){
        printf("SAMPLE %d: ", i);
        print_sensor_sample(&p->samples[i]);
    }
    printf("END OF SENSOR PACKET\n\n");
}

#endif
