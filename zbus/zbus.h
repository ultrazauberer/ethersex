/* vim:fdm=marker ts=4 et ai
 * {{{
 *
 * Copyright (c) 2007 by Christian Dietrich <stettberger@dokucode.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 }}} */

#ifndef _ZBUS_H
#define _ZBUS_H

#include <stdint.h>
#include "../uip/uip.h"

#ifdef ENC28J60_SUPPORT
  #define ZBUS_RECV_BUFFER 128
#else
  #define ZBUS_RECV_BUFFER UIP_CONF_BUFFER_SIZE
#endif

#ifdef _ATMEGA8
  #define RXTX_PORT PORTD
  #define RXTX_DDR  DDRD
  #define RXTX_PIN  PD2
#else
  #define RXTX_PORT PORTC
  #define RXTX_DDR  DDRC
  #define RXTX_PIN  PC2
#endif

/* use 19200 baud at 20mhz (see datasheet for other values) */
#ifdef _ATMEGA8
  #define ZBUS_UART_UBRR 25
#else
  #define ZBUS_UART_UBRR 64
#endif

enum ZBusEscapes {
  ZBUS_START = '0',
  ZBUS_STOP = '1',
};

struct zbus_ctx {
  uint16_t len;
  uint16_t offset;
  uint8_t *data;
};

void zbus_core_init(void);
void zbus_core_periodic(void);

typedef uint8_t (*zbus_send_byte_callback_t)(void **ctx);

uint8_t zbus_send_data(uint8_t *data, uint16_t len);

void zbus_rxstart (void);
struct zbus_ctx *zbus_rxfinish(void);

#if !defined(ENC28J60_SUPPORT)
#define zbus_transmit_packet() zbus_send_data(uip_buf, uip_len)
#endif

void zbus_process(void);

#ifdef _ATMEGA8
  #define ZBUS_BLINK_PORT PORTD
  #define ZBUS_BLINK_DDR DDRD
  #define ZBUS_TX_PIN _BV(PD6)
  #define ZBUS_RX_PIN _BV(PD7)
#endif

#ifdef _ATMEGA644
  #define ZBUS_BLINK_PORT PORTD
  #define ZBUS_BLINK_DDR DDRD
  #define ZBUS_TX_PIN _BV(PD4)
  #define ZBUS_RX_PIN _BV(PD5)
#endif

#endif /* _ZBUS_H */