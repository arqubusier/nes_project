#include "common.h"

#include <stdio.h>

void print_sensor_sample(struct sensor_sample *s){
    printf("T: %d, H: %d, B: %d\n", s->temp, s->heart, s->behaviour);
}

void print_sensor_packet(struct sensor_packet *p){
    int i;
    printf("PRINTING SENSOR PACKET\n");
    for (i = 0; i < SAMPLES_PER_PACKET; i++){
        printf("SAMPLE %d: ", i);
        print_sensor_sample(&sp->samples[i]);
    }
    printf("END OF SENSOR PACKET\n");
}
