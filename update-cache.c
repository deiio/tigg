/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#include "cache.h"

static int cache_name_compare(const char* name1, int len1,
                              const char* name2, int len2) {
  int len;
  int cmp;

  len = len1 < len2 ? len1 : len2;
  cmp = memcmp(name1, name2, len);
  if (cmp != 0) {
    return cmp;
  }

  if (len1 < len2) {
    return -1;
  } else if (len1 > len2) {
    return 1;
  } else {
    return 0;
  }
}

/*
 * If name is existed in cache, return it's (-position - 1).
 * Otherwise, return the position that it should be inserted.
 */
static int cache_name_pos(const char* name, int name_len) {
  int first;
  int last;

  first = 0;
  last = active_nr;
  while (last > first) {
    int next = (last + first) >> 1;
    struct cache_entry* ce = active_cache[next];
    int cmp = cache_name_compare(name, name_len, ce->name, ce->namelen);
    if (cmp == 0) {
      return -next - 1;
    } else if (cmp < 0) {
      last = next;
    } else {
      first = next + 1;
    }
  }

  return first;
}

static int remove_file_from_cache(const char* path) {
  int pos;

  pos = cache_name_pos(path, strlen(path));
  if (pos < 0) {
    pos = -pos - 1;
    active_nr--;
    if (pos < active_nr) {
      memmove(active_cache + pos, active_cache + pos + 1,
              (active_nr - pos + 1) * sizeof(struct cache_entry*));
    }
  }
  return 0;
}

static int add_cache_entry(struct cache_entry* ce) {
  int pos;

  pos = cache_name_pos(ce->name, ce->namelen);

  /* existing match? Just replace it */
  if (pos < 0) {
    active_cache[-pos - 1] = ce;
    return 0;
  }

  /* Make sure the array is big enough */
  if (active_nr == active_alloc) {
    active_alloc = alloc_nr(active_nr);
    active_cache = realloc(active_cache,
                           active_alloc * sizeof(struct cache_entry*));
  }

  /* Add it in */
  active_nr++;
  if (active_nr > pos) {
    memmove(active_cache + pos + 1, active_cache + pos,
            (active_nr - pos - 1) * sizeof(struct cache_entry*));
  }
  active_cache[pos] = ce;

  return 0;
}

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

static int write_cache(int fd, struct cache_entry** cache, int entries) {
  SHA_CTX c;
  struct cache_header hdr;
  int i;

  hdr.signature = CACHE_SIGNATURE;
  hdr.version = 1;
  hdr.entries = entries;

  SHA1_Init(&c);
  SHA1_Update(&c, &hdr, offsetof(struct cache_header, sha1));
  for (i = 0; i < entries; i++) {
    struct cache_entry* ce = cache[i];
    int size = ce_size(ce);
    SHA1_Update(&c, ce, size);
  }
  SHA1_Final(hdr.sha1, &c);

  if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
    return -1;
  }

  for (i = 0; i < entries; i++) {
    struct cache_entry* ce = cache[i];
    int size = ce_size(ce);
    if (write(fd, ce, size) != size) {
      return -1;
    }
  }

  return 0;
}

/*
 * We fundamentally don't like some paths: we don't want
 * dot or dot-dot anywhere, and in fact, we don't even want
 * any other dot-files (.tigg or anything else). They
 * are hidden, for christ sake.
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
