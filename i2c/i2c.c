/*
<<<<<<< HEAD:i2c/i2c.c
 * Copyright (c) 2008 by Jochen Roessner <jochen@lugrot.de>
=======
 * Copyright (c) 2007,2008 by Jochen Roessner <jochen@lugrot.de>
>>>>>>> master:i2c/i2c.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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
 */

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "../net/i2c_state.h"
#include "../uip/uip.h"
#include "../config.h"
#include "i2c.h"

#ifdef I2C_SUPPORT

#define STATS (uip_udp_conn->appstate.i2c)

/*
 * direkter zugriff zum packet buffer
 */
#define BUF ((struct uip_udpip_hdr *)&uip_appdata[-UIP_IPUDPH_LEN])

static struct i2c_tx i2ctx;

void 
i2c_wait_int()
{
	while( (TWCR & _BV(TWINT)) == 0);
}

uint8_t i2c_send ( uint8_t sendbyte )
{
	TWDR = sendbyte;
	TWCR |= _BV(TWINT);
	i2c_wait_int();
	return (TWSR & 0xF8);
}

static void
i2c_port_init(void)
{
  TWCR = 0;
  /* max speed 400khz (problematisch)  ~(_BV(TWPS0) | _BV(TWPS1)) BR = 16
     speed 100khz (normal) _BV(TWPS0) BR = 92 */
  //TWSR &= ~(_BV(TWPS0) | _BV(TWPS1));
#if F_CPU > 10000000
  TWSR |= _BV(TWPS0);
  TWBR = 92;
#else
  TWBR = 52; //max speed for twi bei 8mhz, ca 100khz by 12Mhz Crystal
  PORTC |= _BV(PC4) | _BV(PC5); //enable pullup vor i2c
#endif
  TWCR |= _BV(TWEN);
}

void 
i2c_core_init(uip_udp_conn_t *i2c_conn)
{
  i2c_port_init();

  i2c_conn->appstate.i2c.tx = &i2ctx;
  uip_ipaddr_copy(i2c_conn->ripaddr, all_ones_addr);
  i2c_conn->rport = 0;
  i2c_conn->appstate.i2c.timeout = 0;
  i2c_conn->appstate.i2c.tx->seqnum = 0;
  i2c_conn->appstate.i2c.tx->connstate = I2C_INIT;
}

void
i2c_core_periodic(void)
{
  if(STATS.timeout > 1)
    STATS.timeout--;
  if(STATS.timeout == 1){
    i2c_core_init(uip_udp_conn);
    /* FIXME:   PORTC &= ~_BV(PC2); */
  }
  /* error detection on i2c bus */
  if((TWSR & 0xF8) == 0x00)
    i2c_port_init();
}

void i2c_core_newdata(void)
{
	
		struct i2c_request_t *REQ = uip_appdata;
		/*
		* ueberschreiben der connection info. 
		* port und adresse auf den remotehost begrenzen
		* und antwort paket senden mit der maximalen pufferlaenge (i2c open)
		*/
		if(STATS.tx->connstate == I2C_INIT && STATS.tx->seqnum == 0 && REQ->type == I2C_OPEN){
			STATS.timeout = 10;
			uip_ipaddr_copy(uip_udp_conn->ripaddr, BUF->srcipaddr);
			uip_udp_conn->rport = BUF->srcport;

			STATS.tx->connstate = I2C_OPEN;
			STATS.tx->i2cstate = MAXDATAPAKETLEN;
			STATS.tx->datalen = 0;

			uip_send(&i2ctx, I2C_DATAOFFSET);
			uip_process(UIP_UDP_SEND_CONN);
			fill_llh_and_transmit();
			uip_slen = 0;
		}
		else if(REQ->seqnum != STATS.tx->seqnum){
			STATS.tx->seqnum = REQ->seqnum;
			STATS.timeout = 10;
/* FIXME:			PORTC |= _BV(PC2); */
			/* read init des i2c bus */
			if (REQ->type == I2C_READ || (REQ->type == I2C_READON && STATS.tx->connstate != I2C_READON)){
				STATS.tx->connstate = REQ->type;
				TWCR |= _BV(TWINT) | _BV(TWSTA);
				i2c_wait_int();
				STATS.tx->i2cstate = (TWSR & 0xF8);
				if((TWSR & 0xF8) == 0x08 || (TWSR & 0xF8) == 0x10)
				{
					uint8_t TWSRtmp = 0;
					TWSRtmp = i2c_send ( REQ->i2c_addr<<1 | 0x01 );
					TWCR &= ~_BV(TWSTA);
					STATS.tx->i2cstate = TWSRtmp;
					if(TWSRtmp != 0x40)
					{
						STATS.tx->i2cstate = TWSRtmp;
						TWCR = _BV(TWEN) | _BV(TWSTO);
						STATS.tx->connstate = I2C_ERROR;
					}
				}
				else
				{
					STATS.tx->i2cstate = (TWSR & 0xF8);
					TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
					STATS.tx->connstate = I2C_ERROR;
				}
				
				if(REQ->datalen > 0)
					TWCR |= _BV(TWEA);
			}
			
			if ((REQ->type == I2C_READ || REQ->type == I2C_READON ) && STATS.tx->connstate != I2C_ERROR){
				STATS.tx->datalen = 0;
				while (STATS.tx->datalen < REQ->datalen){ // && STATS.tx->datalen <= I2C_TXBUFMAX){
					{
						TWCR |= _BV(TWINT);
						
						i2c_wait_int();
						
						uint8_t TWSRtmp = (TWSR & 0xF8);
						if(STATS.tx->datalen <= REQ->datalen && TWSRtmp == 0x50){
							STATS.tx->buf[STATS.tx->datalen++] = TWDR;
							if(STATS.tx->datalen == REQ->datalen && (REQ->type == I2C_READ || REQ->datalen < MAXDATAPAKETLEN))
							{
								TWCR = (TWCR | _BV(TWINT)) & ~_BV(TWEA);
							}
						}
						else{
							STATS.tx->buf[STATS.tx->datalen++] = TWSRtmp;
						}
					}
				}
			}
			
			
			/* write init des i2c bus */
			if (uip_datalen() >= I2C_DATAOFFSET && ( REQ->type == I2C_WRITE || (REQ->type == I2C_WRITEON && STATS.tx->connstate != I2C_WRITEON))){
				STATS.tx->connstate = REQ->type;
					/* sende startcondition */
				TWCR |= _BV(TWINT) | _BV(TWSTA);
				i2c_wait_int();
				STATS.tx->i2cstate = (TWSR & 0xF8);
				if((TWSR & 0xF8) == 0x08 || (TWSR & 0xF8) == 0x10)
				{
						/* loesche startcondition und sende adresse */
					TWCR &= ~(_BV(TWSTA) | _BV(TWINT));
					i2c_wait_int();
					uint8_t TWSRtmp = 0;
					TWSRtmp = i2c_send ( REQ->i2c_addr<<1 & 0xFE );
					STATS.tx->i2cstate = TWSRtmp;
					if(TWSRtmp != 0x18 && TWSRtmp != 0x20)
					{
						/* adresse nicht erreichbar */
						STATS.tx->i2cstate = TWSRtmp;
						TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
						STATS.tx->connstate = I2C_ERROR;
					}
				}
				else
				{
						/* startcondition fehlgeschlagen */
					STATS.tx->i2cstate = (TWSR & 0xF8);
					TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
					STATS.tx->connstate = I2C_ERROR;
				}
			}
			/* sende daten an den slave */
			if (uip_datalen() > I2C_DATAOFFSET && ( REQ->type == I2C_WRITE || REQ->type == I2C_WRITEON ) && STATS.tx->connstate != I2C_ERROR){
				uint8_t TWSRtmp;
				STATS.tx->datalen = 0;
				/* sende bis paketdaten ende oder bis datalen erreicht ist */
				while ((STATS.tx->datalen + I2C_DATAOFFSET) < uip_datalen() && STATS.tx->datalen < REQ->datalen){
					TWSRtmp = i2c_send ( REQ->data[STATS.tx->datalen] );
					STATS.tx->buf[STATS.tx->datalen] = REQ->data[STATS.tx->datalen];
						/* fehler protokollieren */
					if(TWSRtmp != 0x28){
						STATS.tx->buf[STATS.tx->datalen] = (TWSR & 0xF8);
						if(STATS.tx->datalen >= MAXDATAPAKETLEN){
							break;
						}
						TWCR |= _BV(TWINT) | _BV(TWSTO);
					}
					STATS.tx->datalen++;
				}
			}
					/*
			* zuruecksetzten der connection info. 
			* port und adresse auf alle freigeben
			* und antwort paket senden mit der info closed (i2c close)
			*/
			if(REQ->type == I2C_INIT){
				/* FIXME: PORTC &= ~_BV(PC2); */
				TWCR |= _BV(TWINT) | _BV(TWSTO);
				STATS.timeout = 0;
				
				STATS.tx->i2cstate = TWSR;
				STATS.tx->datalen = 0;
				STATS.tx->connstate = I2C_INIT;

				uip_send(&i2ctx, I2C_DATAOFFSET);
				uip_process(UIP_UDP_SEND_CONN);
				fill_llh_and_transmit();
				uip_slen = 0;
				i2c_core_init(uip_udp_conn);

			}
			else if(STATS.tx->connstate == I2C_ERROR){
				uip_send(&i2ctx, I2C_DATAOFFSET);
				uip_process(UIP_UDP_SEND_CONN);
				fill_llh_and_transmit();
				uip_slen = 0;
				i2c_core_init(uip_udp_conn);
			}else{
				uip_send(&i2ctx, STATS.tx->datalen+I2C_DATAOFFSET);
			}
		}
		/* retransmit des letzten paketes */
		else if(REQ->seqnum == STATS.tx->seqnum){
			STATS.timeout = 10;
			uip_send(&i2ctx, STATS.tx->datalen+I2C_DATAOFFSET);
		}
}

#endif