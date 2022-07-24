/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#ifndef TIGG__CACHE_H_
#define TIGG__CACHE_H_

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <openssl/sha.h>
#include <zlib.h>

#define DB_ENVIRONMENT "SHA1_FILE_DIRECTORY"
#define DEFAULT_DB_ENVIRONMENT ".dircache/objects"

/* Return a statically allocated filename matching the sha1 signature */
extern char* sha1_file_name(const unsigned char* sha1);

/* Write a memory buffer out to the sha file */
extern
int write_sha1_buffer(unsigned char* sha1, const void* buf, unsigned int size);

/* Read and unpack a sha1 file into memory, write memory to a sha1 file */
extern int write_sha1_file(const char* buf, unsigned int len);

/* Convert to/from hex/sha1 representation */
extern int get_sha1_hex(const char* hex, unsigned char* sha1);
extern char* sha1_to_hex(const unsigned char* sha1);

/* General helper functions */
extern void usage(const char* err);

#endif  /* TIGG__CACHE_H_ */
