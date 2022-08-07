/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#include "cache.h"

int main(int argc, char* argv[]) {
  unsigned char sha1[20];
  char type[20];
  void* buf;
  unsigned long size;
  char tempate[] = "temp_tigg_file_XXXXXX";
  int fd;

  if (argc != 2 || get_sha1_hex(argv[1], sha1)) {
    usage("cat-file <sha1>");
  }

  buf = read_sha1_file(sha1, type, &size);
  if (!buf) {
    exit(1);
  }

  fd = mkstemp(tempate);
  if (fd < 0) {
    usage("unable to create tempfile");
  }

  if (write(fd, buf, size) != size) {
    strcpy(type, "bad");
  }

  printf("%s: %s\n", tempate, type);

  return 0;
}
