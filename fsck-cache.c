/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#include "cache.h"

#include <dirent.h>

/*
 * These two functions should build up a graph in memory about
 * what objects we've referenced, and found, and types...
 *
 * Right now we don't do that kind of reachability checking, Yet.
 */
static void mark_needs_sha1(const unsigned char* parent, const char* type,
                            const unsigned char* child) {}

static int mark_sha1_seen(const unsigned char* sha1, const char* tag) {
  return 0;
}

static int fsck_tree(const unsigned char* sha1, const void* data,
                     unsigned long size) {
  while (size) {
    int len = 1 + strlen(data);
    const unsigned char* file_sha1 = data + len;
    char* path = strchr(data, ' ');
    if (size < len + 20 || !path) {
      return -1;
    }
    data += len + 20;
    size -= len + 20;
    mark_needs_sha1(sha1, "blob", file_sha1);
  }

  return 0;
}

static int fsck_commit(const unsigned char* sha1, const void* data,
                       unsigned long size) {
  unsigned char tree_sha1[20];
  unsigned char parent_sha1[20];

  if (memcmp(data, "tree ", 5)) {
    return -1;
  }
  if (get_sha1_hex(data + 5, tree_sha1) < 0) {
    return -1;
  }
  mark_needs_sha1(sha1, "tree", tree_sha1);
  data += 5 + 40 + 1;  /* "tree " + <hex sha1> + '\n' */
  while (!memcmp(data, "parent ", 7)) {
    if (get_sha1_hex(data + 7, parent_sha1) < 0) {
      return -1;
    }
    mark_needs_sha1(sha1, "commit", parent_sha1);
    data += 7 + 40 + 1;  /* "commit " + <hex sha1> + '\n' */
  }

  return 0;
}

static int fsck_entry(const unsigned char* sha1, const char* tag,
                      const void* data, unsigned long size) {
  if (!strcmp(tag, "blob")) {
    /* Nothing to check */
  } else if (!strcmp(tag, "tree")) {
    if (fsck_tree(sha1, data, size) < 0) {
      return -1;
    }
  } else if (!strcmp(tag, "commit")) {
    if (fsck_commit(sha1, data, size) < 0) {
      return -1;
    }
  } else {
    return -1;
  }
  return mark_sha1_seen(sha1, tag);
}

static int fsck_name(const char* hex) {
  unsigned char sha1[20];
  if (!get_sha1_hex(hex, sha1)) {
    unsigned long map_size;
    void* map = map_sha1_file(sha1, &map_size);
    if (map) {
      char type[100];
      unsigned long size;
      void* buffer = NULL;
      if (!check_sha1_signature(sha1, map, map_size)) {
        buffer = unpack_sha1_file(map, map_size, type, &size);
      }
      munmap(map, map_size);
      if (buffer && !fsck_entry(sha1, type, buffer, size)) {
        return 0;
      }
    }
  }
  return -1;
}

static int fsck_dir(int i, const char* path) {
  DIR* dir = opendir(path);
  struct dirent* de;

  if (!dir) {
    fprintf(stderr, "missing sha1 directory '%s'\n", path);
    return -1;
  }

  while ((de = readdir(dir)) != NULL) {
    char name[100];
    int len = strlen(de->d_name);
    switch (len) {
      case 2:
        if (de->d_name[1] != '.') {
          break;
        }
      case 1:
        if (de->d_name[0] != '.') {
          break;
        }
        continue;
      case 38:
        sprintf(name, "%02x", i);
        memcpy(name + 2, de->d_name, len + 1);
        if (!fsck_name(name)) {
          continue;
        }
    }
    fprintf(stderr, "bad sha1 file: %s/%s\n", path, de->d_name);
  }
  closedir(dir);

  return 0;
}

int main(int argc, char* argv[]) {
  int i;
  char* sha1_dir;

  if (argc != 1) {
    usage("fsck-cache");
  }

  sha1_dir = getenv(DB_ENVIRONMENT) ? : DEFAULT_DB_ENVIRONMENT;
  for (i = 0; i < 256; i++) {
    static char dir[4096];
    sprintf(dir, "%s/%02x", sha1_dir, i);
    fsck_dir(i, dir);
  }

  return 0;
}
