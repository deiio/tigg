# Copyright (c) $YEAR Furzoom.com, All rights reserved.
# Author: Furzoom, mn@furzoom.com

# Optional defaults
CFLAGS ?= -Wall -g -g3 -O2
LDFLAGS ?= -lz -lssl -lcrypto

# All binaries
PROG = cat-file commit-tree init-db update-cache

########################################
# Targets
########################################
all: $(PROG)

cat-file: cat-file.o read-cache.o
commit-tree: commit-tree.o read-cache.o
init-db: init-db.o
update-cache: update-cache.o read-cache.o

cat-file.o: cache.h
commit-tree.o: cache.h
init-db.o: cache.h
read-cache.o: cache.h
update-cache.o: cache.h

install: $(PROG)
	install $(PROG) $(HOME)/bin/

.PHONY: all clean install
clean:
	$(RM) *.o $(PROG)
