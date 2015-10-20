/*
 * Copyright (c) 2014, SICS Swedish ICT.
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
 * \file
 *         UART1 drivers
 * \author
 *         Beshr Al Nahas <beshr@sics.se>
 *
 */

#include <jendefs.h>
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>
#include "contiki-conf.h"
#include "dev/uart1.h"
#include "uart-driver.h"

static unsigned char txbuf_data[UART1_TX_BUFFER_SIZE];
static unsigned char rxbuf_data[UART1_RX_BUFFER_SIZE];
static int (*uart1_input)(unsigned char c);

uint8_t
uart1_active(void)
{
  return uart_driver_tx_in_progress(E_AHI_UART_1);
}
void
uart1_set_input(int
                (*input)(unsigned char c))
{
  uart1_input = input;
  uart_driver_set_input(E_AHI_UART_1, uart1_input);
}
void
uart1_writeb(unsigned char c)
{
  uart_driver_write_buffered(E_AHI_UART_1, c);
}
void
uart1_init(uint8_t br)
{
  uart_driver_init(E_AHI_UART_1, br, txbuf_data, UART1_TX_BUFFER_SIZE, rxbuf_data, UART1_RX_BUFFER_SIZE, uart1_input);
}
