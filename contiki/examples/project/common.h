#ifndef COMMON_H
#define COMMON_H

#define MAX_PACKET_SIZE 128
#define SAMPLES_PER_PACKET 3

#define SENSOR_DATA 0
#define HOP_CONF 1
#define HOP_RECONF 2
#define AGGREGATED_DATA 3

struct packet{
    uint8_t type;
    void *data;
};

struct sensor_data{
    uint8_t temp[2];
    uint8_t heart[2];
    uint8_t behaviour;
};

struct sensor_packet{
    uint8_t id;
    struct sensor_data data[SAMPLES_PER_PACKET];
};

#endif
