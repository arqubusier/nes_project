/*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
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
 */

/**
 * \file
 *         Orchestra default/example configuration
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#ifndef __ORCHESTRA_DEFAULT_CONF_H__
#define __ORCHESTRA_DEFAULT_CONF_H__

/* A default configuration with:
 * - a sender-based slotframe for EB transmission
 * - a sender-based or receiver-based slotframe for unicast to RPL parents and children
 * - a common shared slotframe for any other traffic (mostly broadcast)
 *  */
struct orchestra_rule eb_per_time_source;
struct orchestra_rule unicast_per_neighbor;
struct orchestra_rule default_common;
#define ORCHESTRA_RULES { &eb_per_time_source, \
                          &unicast_per_neighbor, \
                          &default_common, \
                        }

/* Length of the various slotframes. Tune to balance network capacity,
 * contention, energy, latency. */
#define ORCHESTRA_EBSF_PERIOD                     397
#define ORCHESTRA_COMMON_SHARED_PERIOD            31
#define ORCHESTRA_UNICAST_PERIOD                  17

/* Is the per-neighbor unicast slotframe sender-based (if not, it is receiver-based).
 * Note: sender-based works only with RPL storing mode as it relies on DAO and
 * routing entries to keep track of children and parents. */
#define ORCHESTRA_UNICAST_SENDER_BASED            0

/* The hash function used to assign timeslot to a diven node (based on its link-layer address) */
#define ORCHESTRA_LINKADDR_HASH(addr)             ((addr != NULL) ? (addr)->u8[LINKADDR_SIZE-1] : -1)
/* The maximum hash */
#define ORCHESTRA_MAX_HASH                        0x7fff
/* Is the "hash" function collision-free? (e.g. it maps to unique node-ids) */
#define ORCHESTRA_COLLISION_FREE_HASH             0 /* Set to 1 if ORCHESTRA_LINKADDR_HASH returns unique hashes */

#endif /* __ORCHESTRA_DEFAULT_CONF_H__ */
