/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#include "cache.h"

static int check_valid_sha1(const unsigned char* sha1) {
  char* filename = sha1_file_name(sha1);
  int ret;

  /* If we were anal, we'd check that the sha1 of the content
     actually match. */
  ret = access(filename, R_OK);
  if (ret) {
    perror(filename);
  }

  return ret;
}

static int prepend_integer(char* buffer, unsigned int val, int i) {
  buffer[--i] = '\0';
  do {
    buffer[--i] = '0' + (val % 10);
    val /= 10;
  } while (val);

  return i;
}

/* Enough space to add the header of "tree <size>\0" */
#define ORIG_OFFSET (40)

int main(int argc, char* argv[]) {
  unsigned long size;
  unsigned long offset;
  int i;
  int entries = read_cache();
  char* buffer;

  if (entries < 0) {
    fprintf(stderr, "No file-cache to create a tree\n");
    return -1;
  }

  /* Guess at an initial size */
  size = entries * 40 + 400;
  buffer = malloc(size);
  offset = ORIG_OFFSET;

  for (i = 0; i < entries; i++) {
    struct cache_entry* ce = active_cache[i];
    if (check_valid_sha1(ce->sha1)) {
      return -1;
    }

    if (offset + ce->namelen + 60 > size) {
      size = alloc_nr(offset + ce->namelen + 60);
      buffer = realloc(buffer, size);
    }

    offset += sprintf(buffer + offset, "%o %s", ce->st_mode, ce->name);
    buffer[offset++] = 0;
    memcpy(buffer + offset, ce->sha1, 20);
    offset += 20;
  }

  i = prepend_integer(buffer, offset - ORIG_OFFSET, ORIG_OFFSET);
  i -= 5;
  memcpy(buffer + i, "tree ", 5);

  buffer += i;
  offset -= i;

  write_sha1_file(buffer, offset);

  return 0;
}
