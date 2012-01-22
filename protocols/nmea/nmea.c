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
#include "nmea.h"
#ifdef NMEA_TIMESUPPORT
#include "services/clock/clock.h"
#endif
#ifdef NTPD_SUPPORT
#include "services/ntp/ntpd_net.h"
#endif

/* globale Variablen für buffer */
volatile uint8_t nmea_str_complete = 0;     // 1 .. String komplett empfangen
volatile uint8_t nmea_str_count = 0;
volatile char nmea_string[NMEA_MAXSTRLEN + 1] = "";

/* nmea_gprmc_t struct anlegen */
struct nmea_gprmc_t nmea_gprmc;

/* clock_datetime_t struct anlegen */
#ifdef NMEA_TIMESUPPORT
struct clock_datetime_t current_time;
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

uint8_t char2hex(uint8_t character){
	//returns hex value of char
	switch (character)
	{
		case 48: return 0; break; //0
		case 49: return 1; break; //1
		case 50: return 2; break; //2
		case 51: return 3; break; //3
		case 52: return 4; break; //4
		case 53: return 5; break; //5
		case 54: return 6; break; //6
		case 55: return 7; break; //7
		case 56: return 8; break; //8
		case 57: return 9; break; //9
		case 65: return 10; break; //A
		case 66: return 11; break; //B
		case 67: return 12; break; //C
		case 68: return 13; break; //D
		case 69: return 14; break; //E
		case 70: return 15; break; //F
		case 97: return 10; break; //a
		case 98: return 11; break; //b
		case 99: return 12; break; //c
		case 100: return 13; break; //d
		case 101: return 14; break; //e
		case 102: return 15; break; //f
		default: return 255;
	}
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
	
	/* Abweichung größer gleich 1 Sekunde? Neue Zeit setzen */
	#ifdef NMEA_TIMESUPPORT
	if(abs(clock_get_time()-get_nmea_timestamp())>=1)
	{
		clock_set_time(get_nmea_timestamp());
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
uint32_t get_nmea_timestamp(void){
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

	return clock_utc2timestamp(&current_time,2);
	}
}
#endif

/*
  -- Ethersex META --
  header(protocols/nmea/nmea.h)
  init(nmea_init)
  timer(25, gprmc_start())
*/
