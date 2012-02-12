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

#include <avr/pgmspace.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "protocols/ecmd/ecmd-base.h"
#include "nmea.h"

static const char PROGMEM nmea_na[] = "no data available.";
static const char PROGMEM gmaps_link[] = "http://maps.google.de/?q=";

int16_t
parse_cmd_nmea_buffer(char *cmd, char *output, uint16_t len) 
{
  memcpy (output, nmea_string, 49);
  return ECMD_FINAL(49);
}

int16_t
parse_cmd_nmea_date(char *cmd, char *output, uint16_t len) 
{
  if(!nmea_gprmc.valid)
    return snprintf_P (output, len, nmea_na);

  memcpy (output, nmea_gprmc.time, 10);
  output[10] = ' ';
  memcpy (output + 11, nmea_gprmc.date, 6);

  return ECMD_FINAL(17);
}

int16_t
parse_cmd_nmea_position(char *cmd, char *output, uint16_t len) 
{
  if(!nmea_gprmc.valid)
    return snprintf_P (output, len, nmea_na);

  memcpy (output, nmea_gprmc.latitude, 9);
  output[9] = nmea_gprmc.lat_dir;
  output[10] = ' ';
  memcpy (output + 11, nmea_gprmc.longitude, 10);
  output[21] = nmea_gprmc.long_dir;

  return ECMD_FINAL(22);
}

/*
  -- Ethersex META --
  block([[GPS]])
  ecmd_feature(nmea_buffer, "nmea buffer",,shows the actual filled buffer)
  ecmd_feature(nmea_date, "nmea date",,get gprmc time and date)
  ecmd_feature(nmea_position, "nmea position",,get latitude and longitude data)
*/
