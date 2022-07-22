/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#include "cache.h"

int main(int argc, char* argv[]) {
  char* sha1_dir = getenv(DB_ENVIRONMENT);
  char* path;
  int len;
  int i;

  if (mkdir(".dircache", 0700) < 0) {
    perror("unable to create .dircache");
    exit(1);
  }

  /*
   * Share the DB area.
   */
  if (sha1_dir) {
    struct stat st;
    if (stat(sha1_dir, &st) == 0 && S_ISDIR(st.st_mode)) {
      return 0;
    }
    fprintf(stderr, "DB_ENVIRONMENT set to bad directory %s\n", sha1_dir);
  }

  /*
   * The default case is to have a DB per managed directory.
   */
  sha1_dir = DEFAULT_DB_ENVIRONMENT;
  fprintf(stderr, "defauting to private storage area\n");
  len = strlen(sha1_dir);
  if (mkdir(sha1_dir, 0700) < 0) {
    if (errno != EEXIST) {
      perror(sha1_dir);
      exit(1);
    }
  }

  path = malloc(len + 40);
  memcpy(path, sha1_dir, len);
  for (i = 0; i < 256; i++) {
    sprintf(path + len, "/%02x", i);
    if (mkdir(path, 0700) < 0) {
      if (errno != EEXIST) {
        perror(path);
        exit(1);
      }
    }
  }

  return 0;
}
