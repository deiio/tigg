/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#ifndef TIGG_CACHE_H_
#define TIGG_CACHE_H_

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <openssl/sha.h>
#include <zlib.h>

#define DB_ENVIRONMENT "SHA1_FILE_DIRECTORY"
#define DEFAULT_DB_ENVIRONMENT ".dircache/objects"

/*
 * Basic data structures for the directory cache.
 *
 * NOTE NOTE NOTE! This is all in the native CPU byte format. It's
 * not even trying to be portable. It's trying to be efficient. It's
 * just a cache, after all.
 */

#define CACHE_SIGNATURE 0x44495243
struct cache_header {
  unsigned int signature;
  unsigned int version;
  unsigned int entries;
  unsigned char sha1[20];
};

/*
 * The "cache_time" is just the low 32 bits of the
 * time. It doesn't matter if it overflows - we only
 * check it for equality in the 32 bits we save.
 */
struct cache_time {
  unsigned int sec;
  unsigned int nsec;
};

/*
 * dev/ino/mode/uid/gid/size are also just tracked to the low ew bits
 * again - this is just a (very strong in practice) heuristic that
 * the inode hasn't changed.
 */
struct cache_entry {
  struct cache_time ctime;
  struct cache_time mtime;
  unsigned int st_dev;
  unsigned int st_ino;
  unsigned int st_mode;
  unsigned int st_uid;
  unsigned int st_gid;
  unsigned int st_size;
  unsigned char sha1[20];
  unsigned short namelen;
  char name[0];
};

extern const char* sha1_file_directory;
extern struct cache_entry** active_cache;
extern unsigned int active_nr;
extern unsigned int active_alloc;

unsigned long cache_entry_size(unsigned long len);
unsigned int ce_size(const struct cache_entry* ce);
unsigned int alloc_nr(unsigned int n);

/* Initialize the cache information */
extern int read_cache();

/* Return a statically allocated filename matching the sha1 signature */
extern char* sha1_file_name(const unsigned char* sha1);

/* Write a memory buffer out to the sha file */
extern
int write_sha1_buffer(unsigned char* sha1, const void* buf, unsigned int size);

/* Read and unpack a sha1 file into memory, write memory to a sha1 file */
extern void* map_sha1_file(const unsigned char* sha1, unsigned long* size);
extern void* unpack_sha1_file(void* map, unsigned long map_size,
                              char* type, unsigned long* size);
extern void* read_sha1_file(const unsigned char* sha1,
                            char* type, unsigned long *size);
extern int write_sha1_file(const char* buf, unsigned int len);

/* Convert to/from hex/sha1 representation */
extern int get_sha1_hex(const char* hex, unsigned char* sha1);
extern char* sha1_to_hex(const unsigned char* sha1);

/* General helper functions */
extern void usage(const char* err);

#endif  /* TIGG_CACHE_H_ */
