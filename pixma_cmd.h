/*
 * Canon Pixma Scanner Driver / Command handling
 *
 * Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>
 *
 * You always find the most recent version on http://pixma.schewe.com.
 * $Id: pixma_cmd.h 68 2006-04-12 19:06:07Z ms $
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


#ifndef _PIXMA_CMD_H
#define _PIXMA_CMD_H


/*
 * Command descriptor
 */

typedef struct {
	int opcode;
	int extra_bytes;
} cmd_t;


/*
 * Exported functions (for a description have a look at the implementation)
 */

void cmd_set_byte(int pos, char value);
void cmd_set_word(int pos, int value);
void cmd_set_dword(int pos, long value);
void cmd_insert_buf(int pos, unsigned char *buf, int len);
void cmd_prepare(cmd_t cmd, unsigned char *answer);
int cmd_exec();
int read_hangover(unsigned char *buf, int len);

#endif /* _PIXMA_CMD_H */
