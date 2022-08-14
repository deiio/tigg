#!/bin/bash
#
# Copyright (c) 2022 Furzoom.com, All rights reserved.
# Author: Furzoom, mn@furzoom.com
#

test_description='Test the very basics part #1.

The rest of the test suite does not check the basic operation of tigg
plumbing commands to work very carefully.  Their job is to concentrate
on tricky features that caused bugs in the past to detect regression.

This test runs very basic features, like registering things in cache,
writing tree, etc.

Note that this test *deliberately* hard-codes many expected object
IDs.  When object ID computation changes, like in the previous case of
swapping compression and hashing order, the person who is making the
modification *should* take notice and update the test vectors here.
'

. ./test-lib.sh

############################################################
# init-db has been done in an empty repository.
# make sure it is empty

find ".tigg/objects" -type f -print > should-be-empty
test_expect_success \
    '.tigg/objects should be empty after init-db in an empty repo.' \
    'cmp -s /dev/null should-be-empty'

# also it should have 256 subdirectories.  257 is counting "objects"
find ".tigg/objects" -type d -print > full-of-directories
test_expect_success \
    '.tigg/objects should have 256 subdirectories.' \
    "test \$(wc -l full-of-directories | awk '{print \$1}') = 257"