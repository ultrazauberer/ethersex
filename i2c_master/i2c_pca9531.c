/* 
 * Copyright (c) 2009 by Stefan Riepenhausen <rhn@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or
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

#include <avr/io.h>
#include <util/twi.h>

#include "../config.h"
#include "i2c_master.h"
#include "i2c_pca9531.h"

#ifdef I2C_PCA9531_SUPPORT


uint8_t
i2c_pca9531_set(uint8_t address, 
	uint8_t prescaler0, uint8_t pwm0duty,
	uint8_t prescaler1, uint8_t pwm1duty,
	uint8_t led0to3, uint8_t led4to7)
{

  uint8_t ret;
  uint8_t tmp;

  i2c_master_select(address, TW_WRITE);

  TWDR = 0x11;
  tmp = i2c_master_transmit_with_ack();

  if (tmp != TW_MT_DATA_ACK) { ret = 0; goto end; }
  TWDR = prescaler0;
  tmp = i2c_master_transmit_with_ack();
  if (tmp != TW_MT_DATA_ACK) { ret = 0; goto end; }

  TWDR = pwm0duty;
  tmp = i2c_master_transmit_with_ack();
  if (tmp != TW_MT_DATA_ACK) { ret = 0; goto end; }

  TWDR = prescaler1;
  tmp = i2c_master_transmit_with_ack();
  if (tmp != TW_MT_DATA_ACK) { ret = 0; goto end; }

  TWDR = pwm1duty;
  tmp = i2c_master_transmit_with_ack();
  if (tmp != TW_MT_DATA_ACK) { ret = 0; goto end; }

  TWDR = led0to3;
  tmp = i2c_master_transmit_with_ack();
  if (tmp != TW_MT_DATA_ACK) { ret = 0; goto end; }

  TWDR = led4to7;
  tmp = i2c_master_transmit();
  if (tmp != TW_MT_DATA_ACK) { ret = 0; goto end; }

  ret = 0;
end:
  i2c_master_stop();
  return ret;
}

#endif /* I2C_PCA9531_SUPPORT */
