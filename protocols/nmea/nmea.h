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

#ifndef NMEA_H
#define NMEA_H 1

/* enable debugging */
//#define DEBUG_NMEA
#undef DEBUG_NMEA

/* enable timesupport for GPS */
#define NMEA_TIMESUPPORT

#include <stdint.h>
#ifdef NMEA_TIMESUPPORT
#include "services/clock/clock.h"
#endif

/* 
   Example data:
   $GPGGA,174014.360,4923.1834,N,01011.2252,E,1,04

   01234567890123456789012345678901234567890123456
   0         1         2         3         4
 */

/*
   Navibe example data:
   $GPGGA,145240.802,5229.1103,N,01331.6194,E,1,03,4.8,-44.6,M,44.6,M,,0000*47
   $GPRMC,145240.802,A,5229.1103,N,01331.6194,E,0.25,349.21,161211,,,A*69

   012345678901234567890123456789012345678901234567890123456789012345678901234
   0         1         2         3         4         5         6         7
*/

/* added $GPRMC struct */
struct nmea_gprmc_t
{
  volatile unsigned valid	: 1;
  
  char time[11];
  char status;
  char latitude[10];
  char lat_dir;
  char longitude[11];
  char long_dir;
  char speed[6]; //in knots
  char angle[7]; //track angle in degrees
  char date[7];
};

extern struct nmea_gprmc_t nmea_gprmc;

/* globale Variablen für buffer */
#define NMEA_MAXSTRLEN 80
 
extern volatile uint8_t nmea_str_complete;     // 1 .. String komplett empfangen
extern volatile uint8_t nmea_str_count;
extern volatile char nmea_string[NMEA_MAXSTRLEN + 1];

void nmea_init(void);

/* gibt char als hex zurück: 'A'->10 */
uint8_t char2hex(char character);

/* gprmc parser, benötigt nmea_string und nmea_gprmc als globale variablen */
void gprmc_parser(void);

/* startet den parser */
void gprmc_start(void);

/* clock_datetime_t struct befüllen und timestamp zurückgeben */
#ifdef NMEA_TIMESUPPORT
timestamp_t get_nmea_timestamp(void);
#endif

#endif	/* NMEA_H */
