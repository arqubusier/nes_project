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
#include "random.h"
#include "memb.h"
#include "list.h"

#include <stdio.h> /* For printf() */
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
PROCESS(init_process, "Init process");
PROCESS(sensor_process, "Sensor process");
PROCESS(transmit_process, "Transmit process");
AUTOSTART_PROCESSES(&init_process);
/*---------------------------------------------------------------------------*/

struct sensor_vals{
    uint16_t temp;
    uint16_t heart;
    uint8_t behaviour;
};

#define MAX_TEMP 65535
#define MAX_HEART 65535
#define MAX_BEHAVIOUR 255
#define SAMPLE_RATE 14
#define TRANSMIT_RATE 5 

#define BUFF_SIZE 256
#define BUFF_SIZE_EXP 8

#define SENSOR_VALS_PER_PACKET 5


//TODO Determine if needed: static bool is_adding_sensor = false;
MEMB(buff_memb, struct sensor_vals, BUFF_SIZE);

LIST(sensor_buff);

static void print_sensor(struct sensor_vals *s){
    printf("T: %d, H: %d, B: %d", s->temp, s->heart, s->behaviour);
}


static void print_list(list_t l){
    struct sensor_vals * s;
    int cnt;
    printf("PRINTING LIST\n");
    for (s = list_head(l), cnt = 0; s != NULL; s = list_item_next(s), cnt++){
        printf("Cnt: %d, ", cnt);
        print_sensor(s);
        printf("\n");
    }
    printf("END OF LIST\n");
}

PROCESS_THREAD(init_process, ev, data){
    PROCESS_BEGIN();
    list_init(sensor_buff);
    memb_init(&buff_memb);

    process_start(&transmit_process, NULL);
    process_start(&sensor_process, NULL);

    PROCESS_END();
}


PROCESS_THREAD(sensor_process, ev, data)
{
    PROCESS_BEGIN();

    static struct etimer et;
    
    while(1){
        printf("process SENSOR\n");
        etimer_set(&et, CLOCK_SECOND*SAMPLE_RATE);
        PROCESS_WAIT_UNTIL(etimer_expired(&et));

        struct sensor_vals* new_vals = memb_alloc(&buff_memb);

        new_vals->temp = random_rand() % MAX_TEMP;
        new_vals->heart = random_rand() % MAX_HEART;
        new_vals->behaviour = random_rand() % MAX_BEHAVIOUR;

        list_add(sensor_buff, new_vals);

        print_list(sensor_buff);
    }
    
    PROCESS_END();
}

PROCESS_THREAD(transmit_process, ev, data)
{
    PROCESS_BEGIN();
    static struct etimer et;

    while(1){
        printf("process TRANSMIT\n");
        etimer_set(&et, CLOCK_SECOND*TRANSMIT_RATE);
        PROCESS_WAIT_UNTIL(etimer_expired(&et));

        struct sensor_vals *s = list_head(sensor_buff); 
        int i = 0;

        while (s != NULL && i < SENSOR_VALS_PER_PACKET){
            printf("TRANSMITTING: ");
            print_sensor(s);
            printf("\n");

            memb_free(&buff_memb, list_pop(sensor_buff));
            s = list_head(sensor_buff); 
            i++;
        }

    }
    PROCESS_END();
}

/*---------------------------------------------------------------------------*/
