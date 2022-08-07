/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#include "cache.h"

#define MTIME_CHANGED 0x0001
#define CTIME_CHANGED 0x0002
#define OWNER_CHANGED 0x0004
#define MODE_CHANGED  0x0008
#define INODE_CHANGED 0x0010
#define DATA_CHANGED  0x0020

static unsigned int match_stat(struct cache_entry* ce, struct stat* st) {
  unsigned int changed = 0;

#ifdef __APPLE__
  if (ce->mtime.sec != (unsigned int)st->st_mtimespec.tv_sec ||
      ce->mtime.nsec != (unsigned int)st->st_mtimespec.tv_nsec) {
#else
  if (ce->mtime.sec != (unsigned int)st->st_mtim.tv_sec ||
      ce->mtime.nsec != (unsigned int)st->st_mtim.tv_nsec) {
#endif
    changed |= MTIME_CHANGED;
  }

#ifdef __APPLE__
  if (ce->ctime.sec != (unsigned int)st->st_ctimespec.tv_sec ||
      ce->ctime.nsec != (unsigned int)st->st_ctimespec.tv_nsec) {
#else
  if (ce->ctime.sec != (unsigned int)st->st_ctim.tv_sec ||
      ce->ctime.nsec != (unsigned int)st->st_ctim.tv_nsec) {
#endif
    changed |= CTIME_CHANGED;
  }

  if (ce->st_uid != (unsigned int)st->st_uid ||
      ce->st_gid != (unsigned int)st->st_gid) {
    changed |= OWNER_CHANGED;
  }

  if (ce->st_mode != (unsigned int)st->st_mode) {
    changed |= MODE_CHANGED;
  }

  if (ce->st_dev != (unsigned int)st->st_dev ||
      ce->st_ino != (unsigned int)st->st_ino) {
    changed |= INODE_CHANGED;
  }

  if (ce->st_size != (unsigned int)st->st_size) {
    changed |= DATA_CHANGED;
  }

  return changed;
}

static void show_differences(struct cache_entry* ce,
                             struct stat* cur,
                             void* old_contents,
                             unsigned long long old_size) {
  static char cmd[1000];
  FILE* f;

  snprintf(cmd, sizeof(cmd), "diff -u - %s", ce->name);
  f = popen(cmd, "w");
  fwrite(old_contents, old_size, 1, f);
  pclose(f);
}

int main(int argc, char* argv[]) {
  int entries = read_cache();
  int i;

  if (entries < 0) {
    perror("read_cache");
    return -1;
  }

  for (i = 0; i < active_nr; i++) {
    struct stat st;
    struct cache_entry* ce = active_cache[i];
    int n;
    unsigned int changed;
    unsigned long size;
    char type[20];
    void* new;

    if (stat(ce->name, &st) < 0) {
      printf("%s: %s\n", ce->name, strerror(errno));
      continue;
    }

    changed = match_stat(ce, &st);
    if (!changed) {
      printf("%s: ok\n", ce->name);
      continue;
    }

    printf("%.*s:  ", ce->namelen, ce->name);
    for (n = 0; n < 20; n++) {
      printf("%02x", ce->sha1[n]);
    }
    printf("\n");

    new = read_sha1_file(ce->sha1, type, &size);
    show_differences(ce, &st, new, size);
    free(new);
  }

  return 0;
}