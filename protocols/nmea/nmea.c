/*
 * Copyright (c) 2009 by Stefan Siegl <stesie@brokenpipe.de>
 * Copyright (c) 2012 by Florian Franke <derultrazauberer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
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

#define USE_USART NMEA_USE_USART
#define BAUD 19200
#include "config.h"
#include "core/debug.h"
#include "core/usart.h"

#ifdef NMEA_TIMESUPPORT
#include "services/clock/clock.h"
#endif
#ifdef NTPD_SUPPORT
#include "services/ntp/ntpd_net.h"
#endif

#include "nmea.h"

/* globale Variablen für buffer */
volatile uint8_t nmea_str_complete = 0;     // 1 .. String komplett empfangen
volatile uint8_t nmea_str_count = 0;
volatile char nmea_string[NMEA_MAXSTRLEN + 1] = "";

/* nmea_gprmc_t struct anlegen */
struct nmea_gprmc_t nmea_gprmc;

/* clock_datetime_t struct anlegen */
#ifdef NMEA_TIMESUPPORT
timestamp_t last_sync;
clock_datetime_t current_time;
volatile uint8_t nmea_timestamp_valid=0;
#endif

/* We generate our own usart init module, for our usart port */
generate_usart_init()

void
nmea_init(void)
{
  /* Initialize the usart module */
  usart_init();
}

/* UART in buffer */
ISR(usart(USART,_RX_vect))
{
  unsigned char nextChar;
 
  // Daten aus dem Puffer lesen
  nextChar = usart(UDR);
  if( nmea_str_complete == 0 ) {	// wenn uart_string gerade in Verwendung, neues Zeichen verwerfen
 
    // Daten werden erst in uart_string geschrieben, wenn nicht String-Ende/max Zeichenlänge erreicht ist/string gerade verarbeitet wird
    if( nextChar != '\n' &&
        nextChar != '\r' &&
        nmea_str_count < NMEA_MAXSTRLEN - 1 ) {
      nmea_string[nmea_str_count] = nextChar;
      nmea_str_count++;
    }
    else {
      nmea_string[nmea_str_count] = '\0';
      nmea_str_count = 0;
      /* Pre Parser für $GPRMC, da sonst Puffer immer mit anderen Daten gefüllt */
      if(nmea_string[3]=='R' && nmea_string[4]=='M' && nmea_string[5]=='C')
      {	
        nmea_str_complete = 1;
      }
      else
      {
        nmea_str_complete = 0;
      }
    }
  }
}

uint8_t char2hex(char character){
	//values for lowercase
	if(character>'F'){character-='W';}
	//A==10 --> 'A'-'7'=10
	return (character - (character > '9' ? '7' : '0'));
}

/* eventuell kompletten buffer struct übergeben */
void gprmc_parser(void){
	//XOR = ^
	//The checksum field consists of a '*' and two hex digits representing
	//an 8 bit exclusive OR of all characters between, but not including, the '$' and '*'.
	uint8_t cnt=1;
	uint8_t checksum=0x00;
	uint8_t checksum_gp=0x00;
	//struct ungültig setzen
	nmea_gprmc.valid=0;

	//geht buffer mit $ los?
	if(nmea_string[0]=='$')
	{
		//checksum bilden
		while( nmea_string[cnt]!='*' && nmea_string[cnt]!='\0' ){
			checksum^=nmea_string[cnt++];
		}
		//checksum_gp (gegeben) umwandeln
		checksum_gp=char2hex(nmea_string[cnt+1])*16+char2hex(nmea_string[cnt+2]);
		
		//checksums vergleichen
		if(checksum_gp!=checksum)
		{
			#ifdef DEBUG_NMEA
			debug_printf("ERROR: invalid checksum\n");
			#endif
			return;
		}
		else
		{
			if(strncmp(nmea_string,"$GPRMC",6)==0)
			{
				//nmea_string enthält $GPRMC am Anfang
				//kann geparst werden, da pruefsumme auch stimmt
				//$GPRMC,145240.802,A,5229.1103,N,01331.6194,E,0.25,349.21,161211,,,A*69
				//$GPRMC,095607.000,A,5229.0455,N,01331.6009,E,0.00,55.55,200112,,,A*54
				//$GPRMC,143416.794,V,,,,,,,161211,,,N*46
				//0123456789012345678901234567890123456789012345678901234567890123456789
				//0         1         2         3         4         5         6

				//pointer action
				char* p = &nmea_string[7]; // $GPRMC überspringen, pointer auf das 1. Zeichen nach Komma zeigen lassen
				char* pDest = nmea_gprmc.time;
				while (*p != ',') {*pDest++ = *p++;} // copy time
				*pDest='\0'; //Ende 0 für Array
				p++; // skip delimiter
				pDest = &nmea_gprmc.status;
				while (*p != ',') {*pDest++ = *p++;} // copy status
				
				if(nmea_gprmc.status!='A')
				{
					#ifdef DEBUG_NMEA
					debug_printf("Invalid $GPRMC dataset: not active(A)\n");
					#endif
					return;
				}
				
				p++; // skip delimiter
				pDest = nmea_gprmc.latitude;
				while (*p != ',') {*pDest++ = *p++;} // copy latitude
				*pDest='\0'; //Ende 0 für Array
				p++; // skip delimiter
				pDest = &nmea_gprmc.lat_dir;
				while (*p != ',') {*pDest++ = *p++;} // copy lat_dir
				p++; // skip delimiter
				pDest = nmea_gprmc.longitude;
				while (*p != ',') {*pDest++ = *p++;}; // copy longitude
				*pDest='\0'; //Ende 0 für Array
				p++; // skip delimiter
				pDest = &nmea_gprmc.long_dir;
				while (*p != ',') {*pDest++ = *p++;}; // copy long_dir
				p++; // skip delimiter
				pDest = nmea_gprmc.speed;
				while (*p != ',') {*pDest++ = *p++;}; // copy speed
				*pDest='\0'; //Ende 0 für Array
				p++; // skip delimiter
				pDest = nmea_gprmc.angle;
				while (*p != ',') {*pDest++ = *p++;}; // copy angle
				*pDest='\0'; //Ende 0 für Array
				p++; // skip delimiter
				pDest = nmea_gprmc.date;
				while (*p != ',') {*pDest++ = *p++;}; // copy date
				*pDest='\0'; //Ende 0 für Array

				//Struct auf gültig setzen
				nmea_gprmc.valid=1;
				return;
			}
			else
			{
				#ifdef DEBUG_NMEA
				debug_printf("ERROR: invalid beginning (expected $GPRMC)\n");
				#endif
				return;
			}
		}
	}
	else
	{
		#ifdef DEBUG_NMEA
		debug_printf("ERROR: wrong first character (expected $)\n");
		#endif
		return;
	}
}

void gprmc_start(void){
	if(nmea_str_complete==1)
	{
	gprmc_parser();
	nmea_str_complete=0;
	
	last_sync=get_nmea_timestamp();

	/* Abweichung größer gleich 1 Sekunde? Neue Zeit setzen */
	#ifdef NMEA_TIMESUPPORT
	if(abs(clock_get_time()-last_sync)>=1 && nmea_timestamp_valid==1 && last_sync>0)
	{
		clock_set_time_weighted(last_sync,1);
		//clock_set_time(last_sync);
		//reset the ms: now just for 32khz crystal
		#ifdef CLOCK_CRYSTAL_SUPPORT
		TIMER_8_AS_1_COUNTER_CURRENT=0;
		#endif
		nmea_timestamp_valid=0;
		#ifdef NTPD_SUPPORT
		ntp_setstratum(0);
		#endif
		#ifdef DEBUG_NMEA
		debug_printf("GPS-Zeit gesetzt!\n");
		#endif
	}
	#endif
	}

	#ifdef DEBUG_NMEA
	debug_printf("GPRMC: valid: %d lat_dir: %c long_dir: %c\n",nmea_gprmc.valid,nmea_gprmc.lat_dir,nmea_gprmc.long_dir);
	#ifdef NMEA_TIMESUPPORT
	debug_printf("Timestamp: %lu\n",get_nmea_timestamp());
	#endif
	#endif
}

/* Zeitfunktionen */
#ifdef NMEA_TIMESUPPORT
timestamp_t get_nmea_timestamp(void){
	if(nmea_gprmc.valid)
	{
	current_time.sec=(nmea_gprmc.time[4]-0x30)*10;
	current_time.sec+=(nmea_gprmc.time[5]-0x30);
	current_time.min=(nmea_gprmc.time[2]-0x30)*10;
	current_time.min+=(nmea_gprmc.time[3]-0x30);
	current_time.hour=(nmea_gprmc.time[0]-0x30)*10;
	current_time.hour+=(nmea_gprmc.time[1]-0x30);
	current_time.day=(nmea_gprmc.date[0]-0x30)*10;
	current_time.day+=(nmea_gprmc.date[1]-0x30);
	current_time.month=(nmea_gprmc.date[2]-0x30)*10;
	current_time.month+=(nmea_gprmc.date[3]-0x30);
	/* year enthält aktuelles jahr-1900 --> deswegen yy+100 */
	current_time.year=(nmea_gprmc.date[4]-0x30)*10;
	current_time.year+=(nmea_gprmc.date[5]-0x30)+100;

	nmea_timestamp_valid=1;
	return clock_mktime(&current_time,0);
	}
	else return 0;
}
#endif

/*
  -- Ethersex META --
  header(protocols/nmea/nmea.h)
  init(nmea_init)
  timer(25, gprmc_start())
*/
