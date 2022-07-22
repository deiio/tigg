# Copyright (c) $YEAR Furzoom.com, All rights reserved.
# Author: Furzoom, mn@furzoom.com

# Optional defaults
CFLAGS ?= -Wall -g -O2

# All binaries
PROG = init-db

########################################
# Targets
########################################
all: $(PROG)

init-db: init-db.o

install: $(PROG)
	install $(PROG) $(HOME)/bin/

.PHONY: all clean install
clean:
	$(RM) *.o $(PROG)
