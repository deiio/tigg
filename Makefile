# Copyright (c) 2022 Furzoom.com, All rights reserved.
# Author: Furzoom, mn@furzoom.com

# Optional defaults
CFLAGS ?= -Wall -g -g3 -O3
LDFLAGS ?=
LDLIBS ?= -lz $(shell pkg-config --libs openssl)

CFLAGS += $(shell pkg-config --cflags openssl)

# All binaries
PROG = cat-file \
	   checkout-cache \
       commit-tree \
       fsck-cache \
       init-db \
       read-tree \
       show-diff \
       update-cache \
       write-tree \

########################################
# Targets
########################################
all: $(PROG)

cat-file: cat-file.o read-cache.o
checkout-cache: checkout-cache.o read-cache.o
commit-tree: commit-tree.o read-cache.o
fsck-cache: fsck-cache.o read-cache.o
init-db: init-db.o
read-tree: read-tree.o read-cache.o
show-diff: show-diff.o read-cache.o
update-cache: update-cache.o read-cache.o
write-tree: write-tree.o read-cache.o

cat-file.o: cache.h
checkout-cache.o: cache.h
commit-tree.o: cache.h
fsck-cache.o: cache.h
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
