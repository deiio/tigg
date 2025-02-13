# Copyright (c) 2022 Furzoom.com, All rights reserved.
# Author: Furzoom, mn@furzoom.com
#
# Run tests
#OPTS ?= --verbose --debug
OPTS ?=

T = $(wildcard t[0-9][0-9][0-9][0-9]-*.sh)

all::
	@$(foreach t,$T,echo "*** $t ***"; bash $t $(OPTS) || exit;)
	@$(RM) -rf trash

.PHONY: all clean
clean::
	$(RM) -rf trash

