#
# Canon Pixma Scanner Driver
#
# Copyright (C) 2006  Martin Schewe  <pixma@schewe.com>
#
# You always find the most recent version on http://pixma.schewe.com.
# $Id: Makefile 75 2006-04-13 23:02:17Z ms $
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#


CC		= gcc
CFLAGS		= -Wall -Wwrite-strings -O2 #-pedantic -std=c99

OBJECTS		= pixma_debug.o pixma_usb.o pixma_cmd.o pixma.o pixma_scan.o
BINARIES	= pixma_scan
LIBS		= -lusb -lm

PREFIX		= /usr/local
BINDIR		= $(PREFIX)/bin

TESTFILE	= test.pnm


all: $(BINARIES)

install: all
	install -o root -g root -m 755 -s $(BINARIES) $(BINDIR)

test: all
	./pixma_scan -t
	./pixma_scan -r 150 -x 60 -y 10 -w 800 -h 580 -o $(TESTFILE)
	qiv $(TESTFILE)
	@echo "*** make $@ leaved a testfile called $(TESTFILE) behind ***"

clean:
	rm -f $(OBJECTS) $(BINARIES) $(TESTFILE) {**/,}*.tmp

distclean: clean
	rm -fr {**/,}.svn
	rm -f {**/,}{*.rej,.*.swp}

pixma_debug.o: pixma_debug.[ch]

pixma_usb.o: pixma_usb.[ch] pixma_debug.h

pixma_cmd.o: pixma_cmd.[ch] pixma_usb.h pixma_debug.h pixma.h

pixma.o: pixma.[ch] pixma_usb.h pixma_cmd.h

pixma_scan.o: pixma_scan.c pixma.h pixma_debug.h pixma_usb.h

pixma_scan: $(OBJECTS)
	$(CC) $^ -o $@ $(LIBS)
