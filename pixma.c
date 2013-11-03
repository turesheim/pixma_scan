/*
 * Canon Pixma Scanner Driver
 *
 * Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>
 *
 * You always find the most recent version on http://pixma.schewe.com.
 * $Id: pixma.c 81 2006-04-14 00:49:45Z ms $
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


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <arpa/inet.h>

#include "pixma_usb.h"
#include "pixma_cmd.h"
#include "pixma.h"


/*
 * Default packet lenght for scan data requests (should be >> 0x0200)
 */

#define PACKET_LEN		0xc000


/*
 * Gamma table length and max
 */

#define GAMMA_LEN		4096
#define GAMMA_MAX		256


/*
 * Supported devices:
 *
 * MP730/750/760/780: supported
 * MP150/170/450/500: work in progress
 * MP800/830	    : unknown
 * MP110/130	    : looks incompatible
 */

model_t models[] = {	    // vendor  product if out   in    int   prot
	{ "Canon PIXMA MP110", 0x04a9, 0x1700, 0, 0x00, 0x00, 0x00, -1 },
	{ "Canon PIXMA MP130", 0x04a9, 0x1701, 0, 0x00, 0x00, 0x00, -1 },
	{ "Canon PIXMA MP150", 0x04a9, 0x1709, 0, 0x07, 0x88, 0x89,  1 },
	{ "Canon PIXMA MP170", 0x04a9, 0x170a, 0, 0x07, 0x88, 0x89,  1 },
	{ "Canon PIXMA MP450", 0x04a9, 0x170b, 0, 0x07, 0x88, 0x89,  1 },
	{ "Canon PIXMA MP500", 0x04a9, 0x170c, 0, 0x07, 0x88, 0x89,  1 },
	{ "Canon PIXMA MP730", 0x04a9, 0x262f, 1, 0x03, 0x84, 0x85,  0 },
	{ "Canon PIXMA MP750", 0x04a9, 0x1706, 0, 0x03, 0x84, 0x85,  0 },
	{ "Canon PIXMA MP760", 0x04a9, 0x1708, 0, 0x03, 0x84, 0x85,  0 },
	{ "Canon PIXMA MP780", 0x04a9, 0x1707, 0, 0x03, 0x84, 0x85,  0 },
	{ "Canon PIXMA MP800", 0x04a9, 0x170d, 0, 0x07, 0x88, 0x89,  1 },
	{ NULL,                0x0000, 0x0000, 0, 0x00, 0x00, 0x00, -1 }
};


/*
 * Protocol descriptors (status/base/extra/align/min bytes)
 */

protocol_t protocols[] = {
	{ 2, 10, -2, 1, 2 },
	{ 8, 16, -1, 4, 8 }
};


/*
 * Commands we know of so far
 */
			//  opcdoe, extra bytes
cmd_t cmd_activate	= { 0xcf60, 0x000a };
cmd_t cmd_packet_req	= { 0xd420, 0x0004 };
cmd_t cmd_scan_mode	= { 0xdb20, 0      };
cmd_t cmd_setup		= { 0xdd20, 0x000a };
cmd_t cmd_scan		= { 0xde20, 0x002e };
cmd_t cmd_calibration	= { 0xe920, 0      };
cmd_t cmd_unknown_eb80	= { 0xeb80, 0x0014 };
cmd_t cmd_gamma		= { 0xee20, 0x1008 };
cmd_t cmd_abort		= { 0xef20, 0      };
cmd_t cmd_get_info	= { 0xf320, 0x000c };
cmd_t cmd_reset		= { 0xff20, 0x0010 };


/*
 * Selected model and protocol
 */

model_t		*model;
protocol_t	*prot;


/*
 * Scan and scanner parameters
 */

int resolution	= 75;
int monochrome	= 0;
int adf		= 0;
int x		= 0;
int y		= 0;
int width	= MAX_WIDTH_75;
int height	= MAX_HEIGHT_75;


/*
 * The actual size of the image returned by the scanner may be different from
 * the above settings (see pixma_scan)
 */

int raw_width;
int raw_height;


/*
 * Set the resolution -- the check is too sloppy yet
 */

int pixma_set_resolution(int r)
{
	if (r % 75)
		return -1;

	resolution = r;

	return 0;
}


/*
 * Enable or disable the monochrome mode -- we do not respect this setting
 * very much, since the scanner always returns a 24bit color image.  But we
 * can tell the scanner to use different color weights, whomsoever this may
 * help...
 */

int pixma_set_monochrome(int m)
{
	monochrome = m;

	return 0;
}


/*
 * Enable or disable the automatic document feeder
 */

int pixma_set_adf(int a)
{
	adf = a;

	return 0;
}


/*
 * Set the image offset and size
 */

int pixma_set_window(int win_x, int win_y, int win_width, int win_height)
{
	x      = win_x;
	y      = win_y;
	width  = win_width;
	height = win_height;

	return 0;
}


/*
 * Convert the scanner's raw image data to an ordinary RGB image -- the
 * protocol-0-scanners read the blue component first and with some "delay" of
 * (2 * resolution / 75) pixels we get green.  The last one is red with the
 * (4 * resolution / 75) shifting pixels.  If the ADF is used, it is the other
 * way around.
 */

void raw2rgb(unsigned char *rgb, unsigned char *raw)
{
	int comp, x, y;

	for (comp = 0; comp < 3; comp++) {		     // red, green, blue
		int shifting = model->protocol > 0 ? 0 :
			       (adf ? comp : 2 - comp) * 2 * resolution / 75;
		for (y = 0; y < height; y++) {
			int rgb_off = comp + 3 * width * y;
			int raw_off = comp + 3 * raw_width * (y + shifting);

			for (x = 0; x < width; x++)
				rgb[rgb_off + 3 * x] = raw[raw_off + 3 * x];
		}
	}
}


/*
 * Retrieve some information from the scanner that is only partially understood
 */

int pixma_get_info(unsigned char *info)
{
	cmd_prepare(cmd_get_info, info);
	return cmd_exec();
}


/*
 * Check if there is a sheet in the ADF
 */

int pixma_adf_is_ready()
{
	unsigned char info[0x0c];
	return pixma_get_info(info) == STATUS_READY && !info[1];
}


/*
 * Check if the lamp warms up right now
 */

int pixma_is_warming_up()
{
	unsigned char info[0x0c];
	return pixma_get_info(info) == STATUS_READY &&
				       model->protocol == 0 ? info[7] != 0x03 :
				       			      info[8] != 0x01;
}


/*
 * Check if the scanner is calibrated
 */

int pixma_is_calibrated()
{
	unsigned char info[0x0c];
	return pixma_get_info(info) == STATUS_READY &&
				       model->protocol == 0 ? info[8] == 0x0f :
				       			      1;	// FIXME
}


/*
 * Abort the scan process -- works only if the lastly requested packets have
 * already been read.
 */

int pixma_abort()
{
	cmd_prepare(cmd_abort, NULL);
	return cmd_exec();
}


/*
 * Reset the communication with the scanner and demand a device reset thereafter
 */

int pixma_reset()
{
	unsigned char answer[0x10];
	pixma_comm_reset();

	if (model->protocol > 0)		// FIXME:  Ugly hack for prot. 1
		return STATUS_READY;

	cmd_prepare(cmd_reset, answer);
	return cmd_exec();
}


/*
 * Perform a calibration and wait for its completion
 */

int pixma_calibration()
{
	cmd_prepare(cmd_calibration, NULL);
	cmd_set_byte(-6, 0x10);
	int status = cmd_exec();

	if (status == STATUS_READY && !pixma_is_calibrated()) {
		sleep(75);
		while (!pixma_is_calibrated())	   // FIXME: Integrate a timeout
			sleep(5);
	}

	return status;
}


/*
 * (De)activate the scanner:
 *
 * activate = 1: Put the carriage home if it was not already there and wait for
 *		 the scan process to begin
 *
 * activate = 0: Standby and finally put the scanner in sleep mode after some
 *		 minutes of inactivity
 */

int pixma_activate(int activate)
{
	if (model->protocol > 0)		// FIXME:  Ugly hack for prot. 1
		return STATUS_READY;

	cmd_prepare(cmd_activate, NULL);
	cmd_set_byte(1, 1);
	cmd_set_byte(4, activate ? 0x20 : 0);
	return cmd_exec();
}


/*
 * Enable the scan mode
 */

int pixma_scan_mode()
{
	int status;

	do {
		cmd_prepare(cmd_scan_mode, NULL);
		status = cmd_exec();
	} while (status == STATUS_BUSY);	   // FIXME: Integrate a timeout

	return status;
}


/*
 * Set gamma
 */

int pixma_gamma(double gamma)
{
	if (model->protocol == 0)
		return STATUS_READY;

	unsigned char table[GAMMA_LEN];

	int i;
	for (i = 0; i < GAMMA_LEN; i++)
		table[i] = (int) (GAMMA_MAX * pow((double) i / GAMMA_LEN,
						  1 / gamma));

	cmd_prepare(cmd_gamma, NULL);
	cmd_set_dword(1, 0x10001004);
	cmd_insert_buf(5, table, GAMMA_LEN);

	return cmd_exec();
}


/*
 * Set some parameters -- except the ADF (de)activation we do not know what
 * they are really used for
 */

int pixma_setup()
{
	cmd_prepare(cmd_setup, NULL);
	cmd_set_byte(1, adf ? 2 : 1);
	cmd_set_byte(2, 1);

	return cmd_exec();
}


/*
 * Initialize the scanner so that it is accessible in a well-defined way
 */

int pixma_init()
{
	int status;

	do {
		pixma_reset(); // FIXME: We should only reset in the last resort
		status = pixma_activate(0);
	} while (status != STATUS_READY);	   // FIXME: Integrate a timeout

	return status;
}


/*
 * Start the scan
 */

int pixma_scan()
{
	/*
	 * The effective width must be a multiple of 4 and the effective height
	 * must incorporate the "shifting pixels" for the delayed color
	 * components for protocol 0 (see raw2rgb)
	 */

	raw_width  = width + (4 - width % 4) % 4;
	raw_height = height + (model->protocol == 0 ?
			       4 * resolution / 75 + 1 : 0);

	cmd_prepare(cmd_scan, NULL);
	cmd_set_word ( 5, resolution | 0x8000);
	cmd_set_word ( 7, resolution | 0x8000);	     // interpolated resolution?
	cmd_set_dword( 9, x);
	cmd_set_dword(13, y);
	cmd_set_dword(17, raw_width);
	cmd_set_dword(21, raw_height);
	cmd_set_word (25, 0x0818);
	cmd_set_byte (33, 0xff);
	cmd_set_byte (36, 0x81);
	cmd_set_word (39, 0x0201);
	cmd_set_byte (42, model->protocol == 0 && !monochrome);

	return cmd_exec();
}


/*
 * Request a packet of image data.  The packet length and an indicator for the
 * last packet is stored in the referenced arguments.
 */

int pixma_packet_req(int *packet_len, int *last_packet_flag)
{
	unsigned char header[8];

	cmd_prepare(cmd_packet_req, header);
	cmd_set_byte(-2, model->protocol == 0 ? PACKET_LEN >> 8 : 0x08);
	int status = cmd_exec();

	*packet_len = model->protocol == 0 ? ntohs(((uint16_t*) header)[1]) :
					     ntohl(((uint32_t*) header)[1]);

	*last_packet_flag = header[0] && 0x20;

	return status;
}


/*
 * Read and convert the image data from the scanner -- to be called after the
 * scan has been started
 */

int pixma_get_img(unsigned char *rgb)
{
	int status;
	int last_packet_flag;
	unsigned char *raw, *rawp;

	rawp = raw = malloc(3 * raw_width * raw_height);

	do {
		// Request next packet
		int bytes_left;
		status = pixma_packet_req(&bytes_left, &last_packet_flag);
		if (status != STATUS_READY)
			return status;

		// Receive image data
		while (bytes_left > 0) {
			int bytes_received = read_hangover(rawp, bytes_left);

			if (bytes_received <= 0)
				bytes_received = pixma_recv(rawp, bytes_left);

			if (bytes_received < 0)
				return STATUS_NULL;

			rawp	   += bytes_received;
			bytes_left -= bytes_received;
		}

		// Looks absurd, but is inevitable...
		if (model->protocol == 0)
			pixma_recv(rawp, 0);
	} while (!last_packet_flag);

	status = pixma_activate(0);

	raw2rgb(rgb, raw);

	free(raw);

	return status;
}
