/*
 * Canon Pixma Scanner Driver / USB communication
 *
 * Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>
 *
 * You always find the most recent version on http://pixma.schewe.com.
 * $Id: pixma_usb.h 35 2006-03-25 21:26:01Z ms $
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


#ifndef _PIXMA_USB_H
#define _PIXMA_USB_H


/*
 * Exported functions (for a description have a look at the implementation)
 */

int  pixma_open();
int  pixma_close();
int  pixma_send(unsigned char *buf, int len);
int  pixma_recv(unsigned char *buf, int len);
int  pixma_comm_reset();


#endif /* _PIXMA_USB_H */
