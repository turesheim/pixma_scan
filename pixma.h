/*
 * Canon Pixma Scanner Driver
 *
 * Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>
 *
 * You always find the most recent version on http://pixma.schewe.com.
 * $Id: pixma.h 75 2006-04-13 23:02:17Z ms $
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


#ifndef _PIXMA_H
#define _PIXMA_H


/*
 * Status codes returned by the scanner
 */

#define STATUS_NULL		0x0000
#define STATUS_READY		0x0606
#define STATUS_BUSY		0x1414
#define STATUS_ERR		0x1515


/*
 * Head width (A4) and bed length (letter) in pixels for a 75dpi scan.
 * In fact the hardware supports some more pixels in length (ca. up to 904),
 * but the achieved quality may vary -- and it goes beyond the scanner's specs.
 */

#define MAX_WIDTH_75		638
#define MAX_HEIGHT_75		877


/*
 * Version of this release
 */

#define VERSION			"0.1"


/*
 * Model names, IDs, interface, endpoint descriptors and protocol
 */

typedef struct {
	const char*	name;
	int		vendor, product;
	int		interface;
	int		out, in, int_in;
	int		protocol;
} model_t;


/*
 * Protocol descriptor
 */

typedef struct {
	int status_len;
	int cmd_base_len;
	int cmd_extra_bytes_pos;
	int alignment_bytes;
	int min_bytes;
} protocol_t;


/*
 * Global variables for the table of models, the selected model, the table of
 * protocols and the selected protocol
 */

extern model_t		models[], *model;
extern protocol_t	protocols[], *prot;


/*
 * Exported functions (for a description have a look at the implementation)
 */

int pixma_set_resolution(int r);
int pixma_set_monochrome(int m);
int pixma_set_adf(int a);
int pixma_set_window(int win_x, int win_y, int win_width, int win_height);
int pixma_adf_is_ready();
int pixma_is_warming_up();
int pixma_is_calibrated();
int pixma_abort();
int pixma_reset();
int pixma_calibration();
int pixma_activate(int activate);
int pixma_scan_mode();
int pixma_gamma(double gamma);
int pixma_setup();
int pixma_init();
int pixma_scan();
int pixma_get_img(unsigned char *rgb);


#endif /* _PIXMA_H */
