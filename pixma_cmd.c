/*
 * Canon Pixma Scanner Driver / Command handling
 *
 * Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>
 *
 * You always find the most recent version on http://pixma.schewe.com.
 * $Id: pixma_cmd.c 77 2006-04-14 00:02:05Z ms $
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


#include <string.h>
#include <arpa/inet.h>

#include "pixma_debug.h"
#include "pixma_usb.h"
#include "pixma.h"
#include "pixma_cmd.h"


/*
 * Buffer length
 */

#define BUF_LEN		0x4000


/*
 * Find the lesser/larger value of x and y
 */

#define MIN(x, y)	(x) < (y) ? (x) : (y)
#define MAX(x, y)	(x) > (y) ? (x) : (y)


/*
 * Calculate bytes including alignment
 */

#define ALIGN(x, align)	(x) + ((align) - (x) % (align)) % (align);


/*
 * Command/status and answer buffers, length information
 */

unsigned char pixma_buf[BUF_LEN];
unsigned char *answer_buf;
unsigned char *hangover_buf;

int	      answer_len;
int	      cmd_arg_len;
int	      hangover_len;


/*
 * Sums up the bytes of the given buffer and returns its complement
 */

unsigned char checksum(unsigned char *buf, int len)
{
	int sum = 0;
	int i;

	for (i = 0; i < len; i++)
		sum -= buf[i];

	return sum;
}


/*
 * Set a command byte
 */

void cmd_set_byte(int pos, char value)
{
	pixma_buf[pos + prot->cmd_base_len - 1] = value;
}


/*
 * Set a command word
 */

void cmd_set_word(int pos, int value)
{
	*((uint16_t*) (pixma_buf + prot->cmd_base_len + pos - 1)) =
		htons(value);
}


/*
 * Set a command double word
 */

void cmd_set_dword(int pos, long value)
{
	*((uint32_t*) (pixma_buf + prot->cmd_base_len + pos - 1)) =
		htonl(value);
}


/*
 * Insert the given buffer into the command buffer
 */

void cmd_insert_buf(int pos, unsigned char *buf, int len)
{
	memcpy(pixma_buf + prot->cmd_base_len + pos - 1, buf, len);
}


/*
 * Setup command and answer buffer
 */

void cmd_prepare(cmd_t cmd, unsigned char *answer)
{
	int extra_bytes = cmd.extra_bytes;

	if (extra_bytes > 0)
		extra_bytes = ALIGN(MAX(cmd.extra_bytes, prot->min_bytes),
				    prot->alignment_bytes);

	/*
	 * Depending on wether the command returns an answer, extra_bytes is
	 * the length of the answer buffer or the number of the command's extra
	 * argument bytes.  If there are extra arguments, there is no answer and
	 * vice versa.
	 */

	answer_len  = answer != NULL ? extra_bytes : 0;
	cmd_arg_len = answer != NULL ? 0 : extra_bytes;

	memset(pixma_buf, 0, prot->cmd_base_len + cmd_arg_len);
	cmd_set_word(1 - prot->cmd_base_len, cmd.opcode);
	cmd_set_word(prot->cmd_extra_bytes_pos, extra_bytes);

	answer_buf = answer;
}


/*
 * Send a command to the scanner and read the status bytes plus optional answer
 * bytes back.  If the command buffer is more than cmd_base_len bytes long, the
 * last byte gets overwritten with a checksum.
 *
 * Command buffers consist of the following bytes (protocol 0):
 *
 * 1..2: type of instruction
 *    3: seems to be always zero
 *    4: only e9 20 makes use of it, but we don't know what it meens
 * 5..7: seems to be always zero
 *    8: high byte of the expected answer length (for the 2nd read)
 *    9: low byte of the expected answer length (for the 1st read) _OR_
 *       number of extra argument bytes (see 11..) to follow
 *   10: seems to be always zero
 * 11..: extra argument bytes					(optional)
 * last: Inverted checksum over 11..
 */

int cmd_exec()
{
	if (cmd_arg_len)
		cmd_set_byte(cmd_arg_len,
			     checksum(pixma_buf + prot->cmd_base_len,
				      cmd_arg_len - 1));

	DBG(2, "\n");
	DBG_buf(2, pixma_buf, prot->cmd_base_len + cmd_arg_len,
		   prot->cmd_base_len);
	if (pixma_send(pixma_buf, prot->cmd_base_len + cmd_arg_len) !=
	    prot->cmd_base_len + cmd_arg_len)
		return STATUS_NULL;

	// The first data packet must be read at once (only protocol 1)
	int bytes_to_recv =
		model->protocol == 1 && pixma_buf[prot->cmd_base_len - 3] ?
		BUF_LEN : prot->status_len + answer_len;

	DBG(3, "expecting %i status and %i answer bytes...\n",
	       prot->status_len, answer_len);
	int buf_len = pixma_recv(pixma_buf, bytes_to_recv);

	if (buf_len < prot->status_len)
		return STATUS_NULL;

	int status = pixma_buf[0] << 8 | pixma_buf[1];
	DBG(2, "%s\n", status2str(status));

#ifndef DIRTY_STATUS
	if (buf_len < prot->status_len + answer_len)
		status = STATUS_NULL;
#endif
	if (answer_len > 0) {
		memcpy(answer_buf, pixma_buf + prot->status_len, answer_len);
		DBG_buf(2, answer_buf, answer_len, 16);
	}

	hangover_buf = pixma_buf + prot->status_len + answer_len;
	hangover_len = buf_len   - prot->status_len - answer_len;
	DBG(3, "0x%04x bytes in hangover buffer\n", hangover_len);

#ifdef EMPTY_BUF_ON_ERR
	if (status == STATUS_NULL)
		while (pixma_recv(pixma_buf, BUF_LEN) > 0)
			;
#endif
	return status;
}


/*
 * Read data from the hangover buffer
 */

int read_hangover(unsigned char *buf, int len)
{
	if (hangover_len <= 0)
		return 0;

	int ret = MIN(len, hangover_len);
	memcpy(buf, hangover_buf, ret);
	hangover_len -= ret;

	return ret;
}
