#!/bin/bash
#
# Copyright (c) 2022 Furzoom.com, All rights reserved.
# Author: Furzoom, mn@furzoom.com
#

# For repeatability, reset the environment to known value.
unset COMMITTER_NAME
unset COMMITTER_EMAIL
unset COMMITTER_DATE
unset SHA1_FILE_DIRECTORY

# Each test should start with something like this, after copyright notices:
#
# test_description='Description of this test...
# This test checks if command 'foo' does the right thing...
# '
# . ./test-lib.sh

error () {
    echo "* error: $*"
    exit 1
}

say () {
    echo "* $*"
}

test "${test_description}" != "" ||
    error "Test script did not set test_description."

while test "$#" -ne 0
do
    case "$1" in
    -d|--d|--de|--deb|--debu|--debug)
        debug=t
        shift
        ;;
    -h|--h|--he|--hel|--help)
        echo "${test_description}"
        exit 0
        ;;
    -v|--v|--ve|--ver|--verb|--verbo|--verbos|--verbose)
        verbose=t
        shift
        ;;
    *)
        break
        ;;
    esac
done

if test "${verbose}" = "t"
then
    exec 4>&2 3>&1
else
    exec 4>/dev/null 3>/dev/null
fi

test_failure=0
test_count=0

test_debug() {
    test "${debug}" == "" || eval "$1"
}

test_ok() {
    test_count=$((test_count + 1))
    say "ok #${test_count}: $*"
}

test_failure() {
    test_count=$((test_count + 1))
    test_failure=$((test_failure + 1))
    say "NO #${test_count}: $*"
}

test_expect_failure() {
    test "$#" == 2 ||
    error "bug in the test script: not 2 parameters to test_expect_failure"
    say >&3 "expecting failure: $2"
    if eval >&3 2>&4 "$2"
    then
        test_failure "$*"
    else
        test_ok "$1"
    fi
}

test_expect_success() {
    test "$#" == 2 ||
    error "bug in the test script: not 2 parameters to test_expect_success"
    say >&3 "expecting success: $2"
    if eval >&3 2>&4 "$2"
    then
        test_ok "$1"
    else
        test_failure "$*"
    fi
}

test_done() {
    case "${test_failure}" in
    0)
        # We could:
        # cd .. && rm -rf trash
        # but that means we forbid any tests that use their own
        # subdirectory from calling test_done without coming back
        # to where they started from.
        # The Makefile provided will clean this test area so
        # we will have things as they are.
        say "passed all ${test_count} test(s)"
        exit 0
        ;;

    *)
        say "failed ${test_failure} among ${test_count} test(s)"
        exit 1
        ;;

    esac
}

# Test the binaries we have just built.  The tests are kept in
# test/ subdirectory and are run in trash subdirectory.
PATH=$(pwd)/..:$PATH

# Test repository
test=trash
rm -rf "${test}"
mkdir "${test}"
cd "${test}"
init-db 2>/dev/null || error "cannot run init-db"
