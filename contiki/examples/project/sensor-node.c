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

#include "powertrace.h"

#include <stdio.h> /* For printf() */
#include <stdbool.h>
#include <string.h>

#include "common.h"

#define SAMPLE_RATE 5
#define TRANSMIT_RATE 2

#define BUFF_SIZE 25 

#define DEBUG


/*A element to be used in a Contiki list, containting the exact number of
 * sample data for one sensor_packet*/
struct sensor_elem{
    //the next pointer is needed because the struct is used in a Contiki List
    struct sensor_elem *next;
    struct sensor_sample samples[SAMPLES_PER_PACKET];
};

/*---------------------------------------------------------------------------*/
PROCESS(sensor_process, "Sensor process");
PROCESS(transmit_process, "Transmit process");
AUTOSTART_PROCESSES(&sensor_process, &transmit_process);
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

void print_sensor_sample(struct sensor_sample *s){
    printf("T: %d, H: %d, B: %d\n", s->temp, s->heart, s->behaviour);
}

void print_sensor_packet(struct sensor_packet *p){
    int i;
    printf("PRINTING SENSOR PACKET\n");
    for (i = 0; i < SAMPLES_PER_PACKET; i++){
        printf("SAMPLE %d: ", i);
        print_sensor_sample(&p->samples[i]);
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
    struct sensor_packet *msg;
    msg = packetbuf_dataptr();
    printf("RECIEVED PACKET:\n");
    print_sensor_packet(msg);
#endif
}

/*---------------------------------------------------------------------------*/

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

static struct sensor_elem* current_elem = NULL;

MEMB(buff_memb, struct sensor_elem, BUFF_SIZE);
LIST(sensor_buff);
//TODO Determine if needed: static bool is_adding_sensor = false;

/*---------------------------------------------------------------------------*/

static uint8_t sample_cnt = 0; 

PROCESS_THREAD(sensor_process, ev, data)
{
    PROCESS_BEGIN();

    powertrace;

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

            #ifdef DEBUG
            print_list(sensor_buff);
            #endif

            //TODO: handle full buffer
            current_elem = memb_alloc(&buff_memb);
            sample_cnt = 0;
        }
    }
    
    PROCESS_END();
}


PROCESS_THREAD(transmit_process, ev, data)
{
    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
    PROCESS_BEGIN();

    static struct etimer et;

    broadcast_open(&broadcast, 129, &broadcast_call);

    while(1){
        #ifdef DEBUG
        printf("process TRANSMIT\n");
        #endif
        etimer_set(&et, CLOCK_SECOND*TRANSMIT_RATE);
        PROCESS_WAIT_UNTIL(etimer_expired(&et));

        struct sensor_elem *elem = list_head(sensor_buff);
        
        if (elem != NULL){
            #ifdef DEBUG
            print_list(sensor_buff);
            #endif
            list_pop(sensor_buff); 
            
            static struct sensor_packet sp;
            sp.type = SENSOR_DATA;
            memcpy(&sp.samples, &elem->samples, sizeof(elem->samples));
            packetbuf_copyfrom(&sp, sizeof(struct sensor_packet));
            broadcast_send(&broadcast);

            memb_free(&buff_memb, elem);
        }
    }

    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
