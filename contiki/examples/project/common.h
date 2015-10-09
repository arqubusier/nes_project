#ifndef COMMON_H
#define COMMON_H

#define MAX_PACKET_SIZE 	128
#define SAMPLES_PER_PACKET 	3

#define INITIAL_HOP_NR 		100
#define SENSOR_DATA_PER_PACKET  4
#define SENSOR_DATA 		0
#define HOP_CONF 			1
#define HOP_RECONF 			2
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

struct sensor_packet{          // for receiving the packets
    uint8_t type;
    //uint8_t seqno;   // for identifying the order of measurement sample at a sensor node       
    struct sensor_sample samples[SAMPLES_PER_PACKET];
};

struct reconfig_packet{
    uint8_t type;
};

struct routing_init{
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
     struct sensor_data data[SENSOR_DATA_PER_PACKET];
};

void print_sensor_sample(struct sensor_sample *s);
void print_sensor_packet(struct sensor_packet *p);
#endif
