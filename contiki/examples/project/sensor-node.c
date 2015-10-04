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

#include <stdio.h> /* For printf() */
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
PROCESS(sensor_process, "Sensor process");
AUTOSTART_PROCESSES(&sensor_process);
/*---------------------------------------------------------------------------*/

struct sensor_vals{
    uint16_t temp;
    uint16_t heart;
    uint8_t behaviour;
};

#define MAX_TEMP 65535
#define MAX_HEART 65535
#define MAX_BEHAVIOUR 255
#define SAMPLE_RATE 4

#define BUFF_SIZE 256
#define BUFF_SIZE_EXP 8


//TODO Determine if needed: static bool is_adding_sensor = false;
MEMB(buff_memb, struct sensor_vals, MAX_NEIGHBORS);

PROCESS_THREAD(sensor_process, ev, data)
{
    PROCESS_BEGIN();
    static struct etimer et;
    etimer_set(&et, CLOCK_SECOND*SAMPLE_RATE);
    
    while(1){
        struct sensor_vals new_vals;
        new_vals.temp = random_rand() % MAX_TEMP;
        new_vals.heart = random_rand() % MAX_HEART;
        new_vals.behaviour = random_rand() % MAX_BEHAVIOUR;

        printf("New temp: %d, heart: %d, behaviour: %d", new_vals.temp,
                new_vals.heart, new_vals.behaviour);

        PROCESS_WAIT_UNTIL(etimer_expired(&et));
        etimer_set(&et, CLOCK_SECOND*4);
    }
    
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
