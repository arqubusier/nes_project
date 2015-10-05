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

#define MAX_TEMP 256
#define MAX_HEART 256
#define MAX_BEHAVIOUR 256
#define SAMPLE_RATE 14
#define TRANSMIT_RATE 5 

#define BUFF_SIZE 256
#define BUFF_SIZE_EXP 8
#define MAX_PACKET_SIZE 128

#define SENSOR_VALS_PER_PACKET 10//(MAX_PACKET_SIZE - 1)/5

struct sensor_data{
    uint8_t temp[2];
    uint8_t heart[2];
    uint8_t behaviour;
};

struct sensor_packet{
    uint8_t count;
    struct sensor_data* data;
};


static uint8_t tmp_pkt_buff[MAX_PACKET_SIZE];

//TODO Determine if needed: static bool is_adding_sensor = false;
MEMB(buff_memb, struct sensor_data, BUFF_SIZE);

/*---------------------------------------------------------------------------*/
PROCESS(sensor_process, "Sensor process");
PROCESS(transmit_process, "Transmit process");
AUTOSTART_PROCESSES(&sensor_process, &transmit_process);
/*---------------------------------------------------------------------------*/


LIST(sensor_buff);

/* Allocates a new sensor_packet struct from a the pointer to
 * the first byte of packetbuff. */
static struct sensor_packet *packetbuff_to_sensor_packet(void * buf){
}

static void gen_sensor_val(uint8_t *arr, int len){
    int i;
    for (i = 0; i < len; i++){
        arr[i] = random_rand() % MAX_TEMP;
    }
}

static void print_sensor(struct sensor_data *s){
    int i;
    for (i = 0; i < 2; i++){
        printf("T%d: %d, ", i, s->temp[i]);
    }
    for (i = 0; i < 2; i++){
        printf("H%d: %d, ", i, s->heart[i]);
    }

    printf("B: %d\n", s->behaviour);
}

static void print_list(list_t l){
    struct sensor_data *s;
    int cnt;
    printf("PRINTING LIST\n");
    for (s = list_head(l), cnt = 0; s != NULL; s = list_item_next(s), cnt++){
        printf("Cnt: %d, ", cnt);
        print_sensor(s);
        printf("\n");
    }
    printf("END OF LIST\n");
}


PROCESS_THREAD(sensor_process, ev, data)
{
    list_init(sensor_buff);
    memb_init(&buff_memb);

    PROCESS_BEGIN();

    static struct etimer et;
    
    while(1){
        printf("process SENSOR\n");
        etimer_set(&et, CLOCK_SECOND*SAMPLE_RATE);
        PROCESS_WAIT_UNTIL(etimer_expired(&et));

        struct sensor_data* new_vals = memb_alloc(&buff_memb);

        gen_sensor_val(new_vals->temp, 2); 
        gen_sensor_val(new_vals->heart, 2);
        gen_sensor_val(&new_vals->behaviour, 1);

        list_add(sensor_buff, new_vals);

        print_list(sensor_buff);
    }
    
    PROCESS_END();
}


static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
    //broadcast packets are discarded.
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

PROCESS_THREAD(transmit_process, ev, data)
{
    static struct etimer et;

    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
    PROCESS_BEGIN();

    broadcast_open(&broadcast, 129, &broadcast_call);

    while(1){
        printf("process TRANSMIT\n");
        etimer_set(&et, CLOCK_SECOND*TRANSMIT_RATE);
        PROCESS_WAIT_UNTIL(etimer_expired(&et));

        struct sensor_data *s = list_head(sensor_buff); 
        struct sensor_data *popped;
        uint8_t i = 0;

        while (s != NULL && i < SENSOR_VALS_PER_PACKET){
            printf("TRANSMITTING: ");
            print_sensor(s);
            printf("\n");

            popped = list_pop(sensor_buff);
            memcpy(tmp_pkt_buff + (i * sizeof(popped)) + 1,
                    popped, sizeof(popped));
            memb_free(&buff_memb, popped);

            s = list_head(sensor_buff); 
            i++;
        }

        if (i > 0){
            tmp_pkt_buff[0] = i;
            packetbuf_copyfrom(&tmp_pkt_buff, 1 + sizeof(struct sensor_data)*i);
        }

    }
    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
