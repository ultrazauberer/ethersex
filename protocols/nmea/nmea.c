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

#include "nmea.h"

/* include debugging */
#include "core/debug.h"
#define DEBUG_NMEA

/* include clock.h for timesupport */
//#include "services/clock/clock.h"
//struct clock_datetime_t current_time;

#include "config.h"
#define USE_USART NMEA_USE_USART
#define BAUD 19200
#include "core/usart.h"

/* globale Variablen für buffer */
 
volatile uint8_t nmea_str_complete = 0;     // 1 .. String komplett empfangen
volatile uint8_t nmea_str_count = 0;
volatile char nmea_string[NMEA_MAXSTRLEN + 1] = "";

struct nmea_gprmc_t gprmc;

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
      nmea_str_complete = 1;
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
void gprmc_parser(uint8_t *buffer, struct nmea_gprmc_t *gprmc){
	//XOR = ^
	//The checksum field consists of a '*' and two hex digits representing
	//an 8 bit exclusive OR of all characters between, but not including, the '$' and '*'.
	uint8_t cnt=1;
	uint8_t checksum=0x00;
	uint8_t checksum_gp;
	//struct ungültig setzen
	gprmc->valid=0;

	//geht buffer mit $ los?
	if(buffer[0]=='$')
	{
		//checksum bilden
		while( buffer[cnt]!='*' && buffer[cnt]!='\0' ){
			checksum^=buffer[cnt++];
		}
		//checksum_gp (gegeben) umwandeln
		checksum_gp=char2hex(buffer[cnt+1])*16+char2hex(buffer[cnt+2]);
		
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
			if(buffer[1]=='G' && buffer[2]=='P' && buffer[3]=='R' && buffer[4]=='M' && buffer[5]=='C')
			{
				//buffer enthält $GPRMC am Anfang
				//kann geparst werden, da pruefsumme auch stimmt
				//$GPRMC,145240.802,A,5229.1103,N,01331.6194,E,0.25,349.21,161211,,,A*69
				//0123456789012345678901234567890123456789012345678901234567890123456789
				//0         1         2         3         4         5         6
				#ifdef DEBUG_NMEA
				debug_printf("PARSEN kann beginnen\n");
				#endif
				gprmc->date[0]=buffer[57];
				gprmc->date[1]=buffer[58];
				gprmc->date[2]=buffer[59];
				gprmc->date[3]=buffer[60];
				gprmc->date[4]=buffer[61];
				gprmc->date[5]=buffer[62];
				gprmc->time[0]=buffer[7];
				gprmc->time[1]=buffer[8];
				gprmc->time[2]=buffer[9];
				gprmc->time[3]=buffer[10];
				gprmc->time[4]=buffer[11];
				gprmc->time[5]=buffer[12];
				gprmc->time[6]=buffer[13];
				gprmc->time[7]=buffer[14];
				gprmc->time[8]=buffer[15];
				gprmc->time[9]=buffer[16];
				gprmc->valid=1;
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
	gprmc_parser(&nmea_string,&gprmc);
	nmea_str_complete=0;
	}
	
	#ifdef DEBUG_NMEA
	debug_printf("GPRMC: valid: %d\n",gprmc.valid);
	#endif
}

//  timer(50, gprmc_start())
/*
  -- Ethersex META --
  header(protocols/nmea/nmea.h)
  init(nmea_init)
  timer(25, gprmc_start())
*/
