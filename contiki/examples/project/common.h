#ifndef COMMON_H
#define COMMON_H

#define MAX_PACKET_SIZE 	128
#define SAMPLES_PER_PACKET 	3

#define SEQN_INITIAL		0

#define HOP_NR_INITIAL 		100

#define SENSOR_DATA 		0
#define HOP_CONF 			1
#define AGGREGATED_DATA 	3
#define ACK				 	4

struct packet{
    uint8_t type;
    void *data;
};

struct sensor_sample{
    uint8_t temp;
    uint8_t heart;
    uint8_t behaviour;
};

struct sensor_packet{
    uint8_t type;
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

void print_sensor_sample(struct sensor_sample *s);
void print_sensor_packet(struct sensor_packet *p);
#endif
