/*
 * Canon Pixma Scanner Driver / Debugging helper functions
 *
 * Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>
 *
 * You always find the most recent version on http://pixma.schewe.com.
 * $Id: pixma_debug.c 35 2006-03-25 21:26:01Z ms $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdio.h>
#include <stdarg.h>

#include "pixma_debug.h"
#include "pixma.h"


int verbosity = 1;


/*
 * Set verbosity level
 */

void set_verbosity(int v)
{
	verbosity = v;
}


/*
 * Print a message to stderr if the given level matches the preset verbosity
 */

void DBG(int level, const char *format, ...)
{
	if (level > verbosity)
		return;

	va_list args;
	va_start(args, format);

	vfprintf(stderr, format, args);

	va_end(args);
}


/*
 * Dump a buffer (with a given number of columns per line) to stderr if the
 * given level matches the preset verbosity
 */

void DBG_buf(int level, unsigned char *buf, int len, int cols)
{
	int i;
	for (i = 0; i < len; i++) {
		DBG(level, "%02x ", buf[i]);
		if (i % cols == cols - 1 || i == len - 1)
			DBG(level, "\n");
	}
}


/*
 * Return a string for given status code
 */

const char *status2str(int status)
{
	switch (status) {
		case STATUS_NULL:
			return "NULL";
		case STATUS_READY:
			return "READY";
		case STATUS_BUSY:
			return "BUSY";
		case STATUS_ERR:
			return "ERR";
		default:
			return "???";
	}
}
