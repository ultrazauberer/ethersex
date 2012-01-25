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

#include <stdint.h>

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

/* enable debugging */
#define DEBUG_NMEA

/* added $GPRMC struct */
struct nmea_gprmc_t
{
  unsigned valid	: 1;
  
  uint8_t time[10];
  uint8_t status;
  uint8_t latitude[9];
  uint8_t lat_dir;
  uint8_t longitude[10];
  uint8_t long_dir;
// Erstmal rausgenommen, da variable Länge möglich
//  uint8_t speed[4]; //in knots
//  uint8_t angle[6]; //track angle in degrees
  uint8_t date[6];
};

/* globale Variablen für buffer */
#define NMEA_MAXSTRLEN 80
 
extern volatile uint8_t nmea_str_complete;     // 1 .. String komplett empfangen
extern volatile uint8_t nmea_str_count;
extern volatile char nmea_string[NMEA_MAXSTRLEN + 1];

extern struct nmea_gprmc_t gprmc;

void nmea_init(void);

/* gibt char als hex zurück: 'A'->10 */
uint8_t char2hex(uint8_t character);

/* gprmc parser */
void gprmc_parser(volatile char *buffer, struct nmea_gprmc_t *gprmc);

/* startet den parser */
void gprmc_start(void);

#endif	/* NMEA_H */
