/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#include "cache.h"

static int index_fd(const char* path, int name_len, struct cache_entry* ce,
                    int fd, struct stat* st) {
  z_stream stream;
  int max_out_bytes;
  int ret;
  void* out;
  void* metadata;
  void* in;
  SHA_CTX c;

  max_out_bytes = name_len + st->st_size + 200;
  out = malloc(max_out_bytes);
  metadata = malloc(name_len + 200);

  in = mmap(NULL, st->st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (!out || in == MAP_FAILED) {
    return -1;
  }

  memset(&stream, 0, sizeof(stream));
  deflateInit(&stream, Z_BEST_COMPRESSION);

  /*
   * ASCII size + nul byte
   */
  stream.next_in = metadata;
  stream.avail_in = 1 + sprintf(metadata, "blob %lu", (unsigned long)st->st_size);
  stream.next_out = out;
  stream.avail_out = max_out_bytes;
  while (deflate(&stream, 0) == Z_OK) {
    /* nothing */
  }

  /*
   * File content
   */
  stream.next_in = in;
  stream.avail_in = st->st_size;
  while (deflate(&stream, Z_FINISH) == Z_OK) {
    /* nothing */
  }

  deflateEnd(&stream);

  SHA1_Init(&c);
  SHA1_Update(&c, out, stream.total_out);
  SHA1_Final(ce->sha1, &c);

  ret = write_sha1_buffer(ce->sha1, out, stream.total_out);

  munmap(&in, st->st_size);
  free(metadata);
  free(out);

  return ret;
}

static int add_file_to_cache(const char* path) {
  int fd;
  int name_len;
  int size;
  struct cache_entry* ce;
  struct stat st;

  fd = open(path, O_RDONLY);
  if (fd < 0) {
    if (errno == ENOENT) {
      return remove_file_from_cache(path);
    }
    return -1;
  }

  if (fstat(fd, &st) != 0) {
    close(fd);
    return -1;
  }

  name_len = strlen(path);
  size = cache_entry_size(name_len);
  ce = malloc(size);
  memset(ce, 0, size);
  memcpy(ce->name, path, name_len);
  ce->ctime.sec = st.st_ctime;
#ifdef __APPLE__
  ce->ctime.nsec = st.st_ctimespec.tv_nsec;
  ce->mtime.nsec = st.st_mtimespec.tv_nsec;
#else
  ce->ctime.nsec = st.st_ctim.tv_nsec;
  ce->mtime.nsec = st.st_mtim.tv_nsec;
#endif
  ce->mtime.sec = st.st_mtime;
  ce->st_dev = st.st_dev;
  ce->st_ino = st.st_ino;
  ce->st_mode = st.st_mode;
  ce->st_uid = st.st_uid;
  ce->st_gid = st.st_gid;
  ce->st_size = st.st_size;
  ce->namelen = name_len;

  if (index_fd(path, name_len, ce, fd, &st) < 0) {
    return -1;
  }

  return add_cache_entry(ce);
}

/*
 * We fundamentally don't like some paths: we don't want
 * dot or dot-dot anywhere, and in fact, we don't even want
 * any other dot-files (.tigg or anything else). They
 * are hidden, for christ's sake.
 *
 * Also, we don't want double slashes or slashes at the
 * end that can make pathname ambiguous.
 */
static int verify_path(const char* path) {
  char c = *path++;

  if (c == '/' || c == '.' || c == '\0') {
    return 0;
  }

  while (1) {
    if (c == '\0') {
      return 1;
    }

    if (c == '/') {
      c = *path++;
      if (c != '/' && c != '.' && c != '\0') {
        continue;
      }
      return 0;
    }
    c = *path++;
  }
}

int main(int argc, char* argv[]) {
  int i;
  int entries;
  int new_fd;

  entries = read_cache();
  if (entries < 0) {
    perror("cache corrupted");
    return -1;
  }

  new_fd = open(".tigg/index.lock", O_RDWR | O_CREAT | O_EXCL, 0600);
  if (new_fd < 0) {
    perror("unable to create new cachefile");
    return -1;
  }

  for (i = 1; i < argc; i++) {
    char* path = argv[i];
    if (!verify_path(path)) {
      fprintf(stderr, "Ignoring path %s\n", path);
      continue;
    }
    if (add_file_to_cache(path)) {
      fprintf(stderr, "Unable to add %s to database\n", path);
      unlink(".tigg/index.lock");
      return -1;
    }
  }

  if (!write_cache(new_fd, active_cache, active_nr)
      && !rename(".tigg/index.lock", ".tigg/index")) {
    return 0;
  }

  unlink(".tigg/index.lock");
  return 0;
}
