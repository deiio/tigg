/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#include "cache.h"

void usage(const char* err) {
  fprintf(stderr, "%s\n", err);
  exit(1);
}

static unsigned int hex_val(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } else {
    return ~0;
  }
}

int get_sha1_hex(const char* hex, unsigned char* sha1) {
  int i;
  for (i = 0; i < 20; i++) {
    unsigned int val = (hex_val(hex[0]) << 4) | hex_val(hex[1]);
    if (val & ~0xff) {
      return -1;
    }
    *sha1++ = val;
    hex += 2;
  }
  return 0;
}

char* sha1_to_hex(const unsigned char* sha1) {
  static char buffer[50];
  static const char hex[] = "0123456789abcdef";
  char* buf = buffer;
  int i;

  for (i = 0; i < 20; i++) {
    unsigned int val = *sha1++;
    *buf++ = hex[val >> 4];
    *buf++ = hex[val & 0xf];
  }
  return buffer;
}

/*
 * NOTE! This returns a statically allocated buffer, so you have to be
 * careful about using it. Do a "strdup()" if you need to save the
 * filename.
 */
char* sha1_file_name(const unsigned char* sha1) {
  int i;
  static char* name;
  static char* base;

  if (!base) {
    char* sha1_file_directory = getenv(DB_ENVIRONMENT)
                                ? : DEFAULT_DB_ENVIRONMENT;
    int len = strlen(sha1_file_directory);
    base = malloc(len + 60);
    memcpy(base, sha1_file_directory, len);
    memset(base + len, 0, 60);
    base[len] = '/';
    base[len + 3] = '/';
    name = base + len + 1;
  }

  for (i = 0; i < 20; i++) {
    static char hex[] = "0123456789abcdef";
    unsigned int val = sha1[i];
    char* pos = name + i * 2 + (i > 0);
    *pos++ = hex[val >> 4];
    *pos++ = hex[val & 0xf];
  }

  return base;
}

int write_sha1_buffer(unsigned char* sha1, const void* buf, unsigned int size) {
  char* filename = sha1_file_name(sha1);
  int fd;

  fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0666);
  if (fd < 0) {
    return (errno == EEXIST) ? 0 : -1;
  }

  write(fd, buf, size);
  close(fd);

  return 0;
}

int write_sha1_file(const char* buf, unsigned int len) {
  int size;
  char *compressed;
  z_stream stream;
  unsigned char sha1[20];
  SHA_CTX c;

  /* Set it up */
  memset(&stream, 0, sizeof(stream));
  deflateInit(&stream, Z_BEST_COMPRESSION);
  size = deflateBound(&stream, len);
  compressed = malloc(size);

  /* Compress it */
  stream.next_in = buf;
  stream.avail_in = len;
  stream.next_out = compressed;
  stream.avail_out = size;
  while (deflate(&stream, Z_FINISH) == Z_OK) {
    /* nothing */;
  }

  deflateEnd(&stream);
  size = stream.total_out;

  /* Sha1 */
  SHA1_Init(&c);
  SHA1_Update(&c, compressed, size);
  SHA1_Final(sha1, &c);

  if (write_sha1_buffer(sha1, compressed, size) < 0) {
    free(compressed);
    return -1;
  }

  free(compressed);
  return 0;
}
