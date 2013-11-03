/*
 * Canon Pixma Scanner Driver / Simple image acquisition tool
 *
 * Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>
 *
 * You always find the most recent version on http://pixma.schewe.com.
 * $Id: pixma_scan.c 75 2006-04-13 23:02:17Z ms $
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "pixma_debug.h"
#include "pixma_usb.h"
#include "pixma.h"


/*
 * Write an image to a PNM file
 */

void write_pnm(unsigned char *rgb, int width, int height, char *filename)
{
	FILE* fp = fopen(filename, "w");
	fprintf(fp, "P6\n%d %d\n255\n", width, height);
	fwrite(rgb, 1, 3 * width * height, fp);
	fclose(fp);
}


/*
 * Show a usage message
 */

void usage()
{
	fprintf(stderr,
		"Canon Pixma Scanner Driver %s\n"
		"Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>\n\n"

		"Usage: pixma_scan [OPTION]... -o <outfile.pnm>\n\n"

		"-r <resolution>        set resolution in dpi (default: 75)\n"
		"-x <x>     -y <y>      set image offset\n"
		"-w <width> -h <height> set image size\n"
		"-c                     calibrate before scanning\n"
		"-a                     use ADF\n"
		"-m                     use monochrome color weights\n"
		"-v <level>             set verbosity level (default: 1)\n"
		"-o <outfile.pnm>       write PNM image date to outfile.pnm\n",
		VERSION);

	exit(EXIT_FAILURE);
}


/*
 * Parse arguments, setup parameters, scan and write the image to a file
 */

int main(int argc, char **argv)
{
	unsigned char*	img		= NULL;
	char* 		outfile		= NULL;

	int		calibration	= 0;
	int		resolution	= 75;
	int		monochrome	= 0;
	int		adf		= 0;
	int		x		= 0;
	int		y		= 0;
	int		width		= 0;
	int		height		= 0;

	int opt;
	while ((opt = getopt(argc, argv, "r:x:y:w:h:camv:o:")) != -1) {
		switch (opt) {
			case 'r':
				resolution = atoi(optarg);
				break;
			case 'x':
				x = atoi(optarg);
				break;
			case 'y':
				y = atoi(optarg);
				break;
			case 'w':
				width = atoi(optarg);
				break;
			case 'h':
				height = atoi(optarg);
				break;
			case 'c':
				calibration = 1;
				break;
			case 'a':
				adf = 1;
				break;
			case 'm':
				monochrome = 1;
				break;
			case 'v':
				set_verbosity(atoi(optarg));
				break;
			case 'o':
				outfile = strdup(optarg);
				break;
			default:
				usage();
		}
	}

	if (!outfile)
		usage();

	if (!width)
		width = (resolution / 75) * MAX_WIDTH_75 - x;

	if (!height)
		height = (resolution / 75) * MAX_HEIGHT_75 - y;


	if (pixma_open() < 0) {
		fprintf(stderr, "Could not connect to scanner\n");
		exit(EXIT_FAILURE);
	}

	pixma_init();

	if (adf && !pixma_adf_is_ready()) {
		fprintf(stderr, "There is no sheet in the ADF\n");
		exit(EXIT_FAILURE);
	}

	if (pixma_set_resolution(resolution) < 0) {
		fprintf(stderr, "Resolution is not supported\n");
		exit(EXIT_FAILURE);
	}

	if (adf && !pixma_adf_is_ready()) {
		fprintf(stderr, "There is no sheet in the ADF\n");
		exit(EXIT_FAILURE);
	}

	pixma_activate(1);

	if (calibration)
		pixma_calibration();

	pixma_scan_mode();

	pixma_set_monochrome(monochrome);
	pixma_set_adf(adf);
	pixma_set_window(x, y, width, height);

	pixma_setup();
	pixma_gamma(1);

	pixma_scan();
	if (pixma_is_warming_up()) {
		DBG(1, "Lamp warmup in progress...\n");
		sleep(15);
	}

	img = malloc(3 * width * height);
	pixma_get_img(img);

	pixma_close();

	write_pnm(img, width, height, outfile);

	return EXIT_SUCCESS;
}
