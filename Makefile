# Copyright (c) $YEAR Furzoom.com, All rights reserved.
# Author: Furzoom, mn@furzoom.com

# Optional defaults
CFLAGS ?= -Wall -g -g3 -O2
LDFLAGS ?= -lz -lssl -lcrypto

# All binaries
PROG = commit-tree init-db

########################################
# Targets
########################################
all: $(PROG)

init-db: init-db.o

commit-tree: commit-tree.o read-cache.o

init-db.o: cache.h
commit-tree.o: cache.h
read-cache.o: cache.h

install: $(PROG)
	install $(PROG) $(HOME)/bin/

.PHONY: all clean install
clean:
	$(RM) *.o $(PROG)
