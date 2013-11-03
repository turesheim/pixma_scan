/*
 * Canon Pixma Scanner Driver / USB communication
 *
 * Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>
 *
 * You always find the most recent version on http://pixma.schewe.com.
 * $Id: pixma_usb.c 77 2006-04-14 00:02:05Z ms $
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


#include <usb.h>

#include "pixma_debug.h"
#include "pixma_usb.h"
#include "pixma.h"


/*
 * Timeout in ms
 */

#define TIMEOUT		1000


/*
 * USB device handle and model descriptor
 */

struct usb_dev_handle	*devh;


/*
 * Search for the first supported scanner on the USB bus and open the device
 */

int pixma_open()
{
	struct usb_bus    *bus;
	struct usb_device *dev;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	for (bus = usb_busses; bus; bus = bus->next)
		for (dev = bus->devices; dev; dev = dev->next)
			for (model = models; model->name; model++) {
				if (dev->descriptor.idVendor == model->vendor &&
				    dev->descriptor.idProduct == model->product)
				{
					DBG(1, "%s detected\n", model->name);

					if (model->protocol == -1) {
						DBG(1, "...but unsupported\n");
						return -1;
					}

					prot = protocols + model->protocol;

					devh = usb_open(dev);
					return usb_claim_interface(devh,
							model->interface);
				}
			}

	return -1;
}


/*
 * Release the interface and close the USB device
 */

int pixma_close()
{
	usb_release_interface(devh, 0);
	return usb_close(devh);
}


/*
 * Send a buffer to the scanner
 */

int pixma_send(unsigned char *buf, int len)
{
	int ret = usb_bulk_write(devh, model->out, (char*) buf, len, TIMEOUT);

	if (ret != len && len > 0)
		DBG(2, "Incomplete write (0x%04x/0x%04x)\n", ret, len);

	return ret;
}


/*
 * Receive a buffer from the scanner
 */

int pixma_recv(unsigned char *buf, int len)
{
	int ret = usb_bulk_write(devh, model->in, (char*) buf, len, TIMEOUT);

	if (ret != len && len > 0)
		DBG(3, "Incomplete read (0x%04x/0x%04x)\n", ret, len);

	return ret;
}


/*
 * Reset the communication
 */

int pixma_comm_reset()
{
	return usb_reset(devh);
}
