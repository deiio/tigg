# Copyright (c) $YEAR Furzoom.com, All rights reserved.
# Author: Furzoom, mn@furzoom.com

# Optional defaults
CFLAGS ?= -Wall -g -g3 -O3
LDFLAGS ?=
LDLIBS ?= -lz -lssl -lcrypto

# All binaries
PROG = cat-file commit-tree init-db read-tree show-diff update-cache write-tree

########################################
# Targets
########################################
all: $(PROG)

cat-file: cat-file.o read-cache.o
commit-tree: commit-tree.o read-cache.o
init-db: init-db.o
read-tree: read-tree.o read-cache.o
show-diff: show-diff.o read-cache.o
update-cache: update-cache.o read-cache.o
write-tree: write-tree.o read-cache.o

cat-file.o: cache.h
commit-tree.o: cache.h
init-db.o: cache.h
read-cache.o: cache.h
read-tree.o: cache.h
show-diff.o: cache.h
update-cache.o: cache.h
write-tree.o: cache.h

install: $(PROG)
	install $(PROG) $(HOME)/bin/

.PHONY: all clean install
clean:
	$(RM) *.o $(PROG)
