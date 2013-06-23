# Copyright (c) 2013 Simon Leblanc
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

EXEC = clinfo

CFLAGS = -Wall -O3
LDFLAGS =

ifeq ($(shell uname),Darwin)
	LDFLAGS += -framework OpenCL
else
	LDFLAGS += -lopencl
endif

prefix = /usr/local
bindir = bin
mandir = share/man/man1


CC = cc
RM = rm -f
INSTALL = install

.PHONY: clean install uninstall

all: $(EXEC)

$(EXEC): clinfo.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(EXEC)

install: $(EXEC)
	$(INSTALL) $(EXEC) $(prefix)/$(bindir)/
	$(INSTALL) clinfo.1 $(prefix)/$(mandir)/$(EXEC).1

uninstall:
	@test -s $(prefix)/$(bindir)/$(EXEC) \
	|| { echo "Not found: $(prefix)/$(bindir)/$(EXEC)"; exit 1; }
	$(RM) $(prefix)/$(bindir)/$(EXEC)
	@test -s $(prefix)/$(mandir)/$(EXEC).1 \
	|| { echo "Not found: $(prefix)/$(mandir)/$(EXEC).1"; exit 1; }
	$(RM) $(prefix)/$(mandir)/$(EXEC).1
