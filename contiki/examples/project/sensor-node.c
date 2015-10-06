/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

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

#include <stdio.h> /* For printf() */
#include <stdbool.h>
#include <string.h>

#include "common.h"

#define SAMPLE_RATE 1
#define TRANSMIT_RATE 10 

#define BUFF_SIZE 25 

#define DEBUG




/*---------------------------------------------------------------------------*/
PROCESS(sensor_process, "Sensor process");
PROCESS(transmit_process, "Transmit process");
AUTOSTART_PROCESSES(&sensor_process, &transmit_process);
/*---------------------------------------------------------------------------*/

//TODO: error checking: count integrity, size etc...
static void gen_sensor_data(struct sensor_data* data){
    int i;
    for (i = 0; i < sizeof(data->temp); i++){
        data->temp[i] = random_rand() % 256;
    }

    for (i = 0; i < sizeof(data->temp); i++){
        data->heart[i] = random_rand() % 256;
    }

    data->behaviour = random_rand() % 256;
}

#ifdef DEBUG
void print_sensor_data(struct sensor_data *s){
    int i;
    for (i = 0; i < 2; i++){
        printf("T%d: %d, ", i, s->temp[i]);
    }
    printf("\n");

    for (i = 0; i < 2; i++){
        printf("H%d: %d, ", i, s->heart[i]);
    }
    printf("\n");

    printf("B: %d\n", s->behaviour);
}
#endif

#ifdef DEBUG
void print_sensor_packet(struct sensor_packet *sp){
    int i;
    printf("PRINTING SENSOR PACKET\n");
    for (i = 0; i < SAMPLES_PER_PACKET; i++){
        printf("SAMPLE %d: ", i);
        print_sensor_data(sp->data + i);
    }
    printf("END OF SENSOR PACKET\n");
}
#endif

#ifdef DEBUG
static void print_list(list_t l){
    struct sensor_packet *s;
    int cnt;
    printf("PRINTING LIST\n");
    for (s = list_head(l), cnt = 0; s != NULL; s = list_item_next(s), cnt++){
        print_sensor_packet(s);
    }
    printf("END OF LIST\n");
}
#endif

static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
#ifdef DEBUG
    struct sensor_packet *packet;
    packet = packetbuf_dataptr();
    print_sensor_packet(packet);
#endif
}

/*---------------------------------------------------------------------------*/

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

static struct sensor_packet* current_packet = NULL;

MEMB(buff_memb, struct sensor_packet, BUFF_SIZE);
LIST(sensor_buff);
//TODO Determine if needed: static bool is_adding_sensor = false;

/*---------------------------------------------------------------------------*/

static uint8_t sensor_data_cnt = 0; 

PROCESS_THREAD(sensor_process, ev, data)
{

    list_init(sensor_buff);
    memb_init(&buff_memb);

    PROCESS_BEGIN();

    current_packet = memb_alloc(&buff_memb);
    current_packet->id = SENSOR_DATA;
    sensor_data_cnt = 0;


    static struct etimer et;
    
    while(1){
        printf("process SENSOR\n");
        etimer_set(&et, CLOCK_SECOND*SAMPLE_RATE);
        PROCESS_WAIT_UNTIL(etimer_expired(&et));

        if (sensor_data_cnt == SAMPLES_PER_PACKET){
            //new packet
            list_add(sensor_buff, current_packet);
            current_packet = memb_alloc(&buff_memb);
            current_packet->id = SENSOR_DATA;
            sensor_data_cnt = 0;
            
#ifdef DEBUG
            print_list(sensor_buff);
#endif
        }
        else{
            //add to current packet
            gen_sensor_data(&current_packet->data[sensor_data_cnt]);
            sensor_data_cnt++;
#ifdef DEBUG
            printf("NEW DATA\n");
            print_sensor_data(&current_packet->data[sensor_data_cnt]);
#endif
        }
    }
    
    PROCESS_END();
}


PROCESS_THREAD(transmit_process, ev, data)
{
    static struct etimer et;

    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
    PROCESS_BEGIN();

    broadcast_open(&broadcast, 129, &broadcast_call);

    while(1){
#ifdef DEBUG
        printf("process TRANSMIT\n");
#endif
        etimer_set(&et, CLOCK_SECOND*TRANSMIT_RATE);
        PROCESS_WAIT_UNTIL(etimer_expired(&et));

        struct sensor_packet *packet = list_head(sensor_buff);
        
        if (packet != NULL){
#ifdef DEBUG
            print_list(sensor_buff);
#endif
            list_pop(sensor_buff); 

            packetbuf_copyfrom(packet, sizeof(struct sensor_packet));
            broadcast_send(&broadcast);

            memb_free(&buff_memb, packet);
        }
    }
    
    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
