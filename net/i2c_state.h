/*
 * Copyright (c) 2007 by Jochen Roessner <jochen@lugrot.de>
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

#ifndef _I2C_STATE_H
#define _I2C_STATE_H

/* constants */
#define MAXDATAPAKETLEN 36

struct i2c_tx {
	union {
		uint8_t raw[0];
		uint8_t seqnum;
	};
	uint8_t connstate;
	uint8_t i2cstate;
	uint8_t datalen;
	uint8_t buf[MAXDATAPAKETLEN];
};

struct i2c_request_t {
	union{
		uint8_t raw[0];
		uint8_t seqnum;
	};
	uint8_t type;
	uint8_t i2c_addr;
	uint8_t datalen;
	union{
		uint8_t data[0];
	};
};

struct i2c_connection_state_t {
	uint8_t txstate;
	uint8_t timeout;
	struct i2c_tx *tx;
};

#endif