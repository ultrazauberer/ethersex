/*
 *
 * Copyright (c) Gregor B.
 * Copyright (c) Guido Pannenbecker
 * Copyright (c) Stefan Riepenhausen
 * Copyright (c) 2009 Dirk Pannenbecker <dp@sd-gp.de>
 * Copyright (c) 2012 by Erik Kunze <ethersex@erik-kunze.de>
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <stdlib.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "core/heartbeat.h"

#include "rfm12.h"
#include "rfm12_ask.h"

#define INTERTECHNO_PERIOD 264  // produces pulse of 360 us

#ifdef RFM12_ASK_SENDER_SUPPORT
#ifdef RFM12_ASK_TEVION_SUPPORT
void
rfm12_ask_tevion_send(uint8_t * housecode, uint8_t * command, uint8_t delay,
                      uint8_t cnt)
{
  uint8_t code[41];
  uint8_t *p = code;

  for (uint8_t i = 0; i < 3; i++)
  {
    uint8_t byte = housecode[i];
    for (uint8_t mask = 0x80; mask; mask >>= 1)
    {
      *p++ = byte & mask ? 12 : 6;
    }
  }
  *p++ = housecode[2] & 1 ? 6 : 12;
  for (uint8_t i = 0; i < 2; i++)
  {
    uint8_t byte = command[i];
    for (uint8_t mask = 0x80; mask; mask >>= 1)
    {
      *p++ = byte & mask ? 12 : 6;
    }
  }

  rfm12_prologue();
  rfm12_trans(RFM12_CMD_PWRMGT | RFM12_PWRMGT_ET | RFM12_PWRMGT_ES |
              RFM12_PWRMGT_EX);
  for (uint8_t ii = cnt; ii > 0; ii--)
  {
    wdt_kick();
    uint8_t rfm12_trigger_level = 0;
    for (uint8_t i = 0; i < 41; i++)
    {
      rfm12_ask_trigger(rfm12_trigger_level ^= 1, code[i] * delay);
    }
    rfm12_ask_trigger(0, 24 * delay);
  }
  rfm12_trans(RFM12_CMD_PWRMGT | RFM12_PWRMGT_EX);
  rfm12_epilogue();
}
#endif /* RFM12_ASK_TEVION_SUPPORT */

#ifdef RFM12_ASK_INTERTECHNO_SUPPORT
static void
rfm12_ask_intertechno_send_bit(const uint8_t bit)
{
  rfm12_ask_trigger(1, INTERTECHNO_PERIOD);
  rfm12_ask_trigger(0, 3 * INTERTECHNO_PERIOD);
  if (bit)
  {
    rfm12_ask_trigger(1, 3 * INTERTECHNO_PERIOD);
    rfm12_ask_trigger(0, INTERTECHNO_PERIOD);
  }
  else
  {
    rfm12_ask_trigger(1, INTERTECHNO_PERIOD);
    rfm12_ask_trigger(0, 3 * INTERTECHNO_PERIOD);
  }
}

static void
rfm12_ask_intertechno_send_sync(void)
{
  rfm12_ask_trigger(1, INTERTECHNO_PERIOD);
  rfm12_ask_trigger(0, 31 * INTERTECHNO_PERIOD);
}

void
rfm12_ask_intertechno_send(uint8_t family, uint8_t group,
                           uint8_t device, uint8_t command)
{
  union
  {
    struct
    {
      uint16_t family:4;
      uint16_t device:2;
      uint16_t group:2;
      uint16_t command:4;
    } bits;
    uint16_t raw;
  } code;

  family -= 1;
  code.bits.family = family;
  device -= 1;
  code.bits.device = device;
  group -= 1;
  code.bits.group = group;
  code.bits.command = command ? 0x0E : 0x06;

  rfm12_prologue();
  rfm12_trans(RFM12_CMD_PWRMGT | RFM12_PWRMGT_ET | RFM12_PWRMGT_ES |
              RFM12_PWRMGT_EX);
  for (uint8_t j = 6; j > 0; j--)
  {
    wdt_kick();
    uint16_t c = code.raw;
    for (uint8_t i = 12; i; i--)
    {
      rfm12_ask_intertechno_send_bit(c & 1);
      c >>= 1;
    }
    rfm12_ask_intertechno_send_sync();
  }
  rfm12_trans(RFM12_CMD_PWRMGT | RFM12_PWRMGT_EX);
  rfm12_epilogue();
}
#endif /* RFM12_ASK_INTERTECHNO_SUPPORT */

#if defined RFM12_ASK_2272_SUPPORT || defined RFM12_ASK_1527_SUPPORT

#ifdef RFM12_ASK_2272_SUPPORT
static const uint8_t ask_2272_pulse_duty_factor[4] PROGMEM = { 13, 5, 7, 11 };
#endif
#ifdef RFM12_ASK_1527_SUPPORT
static const uint8_t ask_1527_pulse_duty_factor[4] PROGMEM = { 9, 3, 3, 9 };
#endif

static void
rfm12_ask_2272_1527_send(uint8_t * command, uint8_t delay, uint8_t cnt,
                         const uint8_t * duty_factor)
{
  uint8_t code[49];
  uint8_t *p = code;

  for (uint8_t i = 0; i < 3; i++)
  {
    uint8_t byte = command[i];
    for (uint8_t mask = 0x80; mask; mask >>= 1)
    {
      if (byte & mask)
      {
        *p++ = pgm_read_byte(duty_factor);
        *p++ = pgm_read_byte(duty_factor + 1);
      }
      else
      {
        *p++ = pgm_read_byte(duty_factor + 2);
        *p++ = pgm_read_byte(duty_factor + 3);
      }
    }
  }
  *p = 7;                 // sync

  rfm12_prologue();
  rfm12_trans(RFM12_CMD_PWRMGT | RFM12_PWRMGT_ET | RFM12_PWRMGT_ES |
              RFM12_PWRMGT_EX);
  for (uint8_t ii = cnt; ii > 0; ii--)
  {
    wdt_kick();
    uint8_t rfm12_trigger_level = 0;
    for (uint8_t i = 0; i < 49; i++)
    {
      rfm12_ask_trigger(rfm12_trigger_level ^= 1, code[i] * delay);
    }
    rfm12_ask_trigger(0, 24 * delay);
  }
  rfm12_trans(RFM12_CMD_PWRMGT | RFM12_PWRMGT_EX);
  rfm12_epilogue();
}

#ifdef RFM12_ASK_2272_SUPPORT
void
rfm12_ask_2272_send(uint8_t * command, uint8_t delay, uint8_t cnt)
{
  rfm12_ask_2272_1527_send(command, delay, cnt, ask_2272_pulse_duty_factor);
}
#endif

#ifdef RFM12_ASK_1527_SUPPORT
void
rfm12_ask_1527_send(uint8_t * command, uint8_t delay, uint8_t cnt)
{
  rfm12_ask_2272_1527_send(command, delay, cnt, ask_1527_pulse_duty_factor);
}
#endif
#endif /* RFM12_ASK_2272_SUPPORT || RFM12_ASK_1527_SUPPORT */

void
rfm12_ask_trigger(uint8_t level, uint16_t us)
{
  if (level)
  {
    rfm12_trans(RFM12_CMD_PWRMGT | RFM12_PWRMGT_ET | RFM12_PWRMGT_ES |
                RFM12_PWRMGT_EX);
#ifdef STATUSLED_RFM12_TX_SUPPORT
    PIN_SET(STATUSLED_RFM12_TX);
#endif
    ACTIVITY_LED_RFM12_TX;
  }
  else
  {
    rfm12_trans(RFM12_CMD_PWRMGT | RFM12_PWRMGT_EX);
#ifdef STATUSLED_RFM12_TX_SUPPORT
    PIN_CLEAR(STATUSLED_RFM12_TX);
#endif
  }
  for (; us > 0; us--)
    _delay_us(1);
}
#endif // RFM12_ASK_SENDER_SUPPORT

#ifdef RFM12_ASK_EXTERNAL_FILTER_SUPPORT
void
rfm12_ask_external_filter_init(void)
{
  rfm12_prologue();
  rfm12_trans(RFM12_CMD_PWRMGT | RFM12_PWRMGT_ER | RFM12_PWRMGT_EBB);
  rfm12_trans(RFM12_CMD_DATAFILTER | RFM12_DATAFILTER_S);
  rfm12_epilogue();
}

void
rfm12_ask_external_filter_deinit(void)
{
  rfm12_init();
}
#endif /* RFM12_ASK_EXTERNAL_FILTER_SUPPORT */
