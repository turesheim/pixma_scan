/*
 * Canon Pixma Scanner Driver / Debugging helper functions
 *
 * Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>
 *
 * You always find the most recent version on http://pixma.schewe.com.
 * $Id: pixma_debug.h 35 2006-03-25 21:26:01Z ms $
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


#ifndef _PIXMA_DEBUG_H
#define _PIXMA_DEBUG_H


/*
 * Exported functions (for a description have a look at the implementation)
 */

void set_verbosity(int v);
void DBG(int level, const char *format, ...);
void DBG_buf(int level, unsigned char *buf, int len, int cols);
const char *status2str(int status);


#endif /* _PIXMA_DEBUG_H */
