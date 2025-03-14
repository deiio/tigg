/*
 * Copyright (c) 2022 Furzoom.com, All rights reserved.
 * Author: Furzoom, mn@furzoom.com
 */

#include "cache.h"

const char* sha1_file_directory = NULL;
struct cache_entry** active_cache = NULL;
unsigned int active_nr;
unsigned int active_alloc;


inline unsigned long cache_entry_size(unsigned long len) {
  return (offsetof(struct cache_entry, name) + len + 8) & ~7;
}

inline unsigned int ce_size(const struct cache_entry* ce) {
  return cache_entry_size(ce->namelen);
}

inline unsigned int alloc_nr(unsigned int n) {
  return (n + 16) * 3 / 2;
}

static int error(const char* msg) {
  fprintf(stderr, "error: %s\n", msg);
  return -1;
}

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

  if (write(fd, buf, size) != size) {
    perror("write error");
    exit(1);
  }
  close(fd);

  return 0;
}

unsigned int cache_match_stat(struct cache_entry* ce, struct stat* st) {
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
int cache_name_pos(const char* name, int name_len) {
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

int check_sha1_signature(const unsigned char* sha1,
                         const void* buf, unsigned long size) {
  unsigned char real_sha1[20];
  SHA_CTX c;

  SHA1_Init(&c);
  SHA1_Update(&c, buf, size);
  SHA1_Final(real_sha1, &c);
  return memcmp(sha1, real_sha1, 20);
}

void* map_sha1_file(const unsigned char* sha1, unsigned long* size) {
  const char* filename = sha1_file_name(sha1);
  int fd = open(filename, O_RDONLY);
  struct stat st;
  void* map;

  if (fd < 0) {
    perror(filename);
    return NULL;
  }

  if (fstat(fd, &st) < 0) {
    close(fd);
    return NULL;
  }

  map = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (MAP_FAILED == map) {
    return NULL;
  }

  *size = st.st_size;
  return map;
}

void* unpack_sha1_file(void* map, unsigned long map_size,
                       char* type, unsigned long* size) {
  int ret;
  int bytes;
  z_stream stream;
  char buffer[8192];
  void* buf;

  /* Get the data stream */
  memset(&stream, 0, sizeof(stream));
  stream.next_in = map;
  stream.avail_in = map_size;
  stream.next_out = (void*)buffer;
  stream.avail_out = sizeof(buffer);

  inflateInit(&stream);
  ret = inflate(&stream, 0);
  if (sscanf(buffer, "%10s %lu", type, size) != 2) {
    return NULL;
  }

  bytes = strlen(buffer) + 1;
  buf = malloc(*size);
  if (!buf) {
    return NULL;
  }

  memcpy(buf, buffer + bytes, stream.total_out - bytes);
  bytes = stream.total_out - bytes;
  if (bytes < *size && ret == Z_OK) {
    stream.next_out = buf + bytes;
    stream.avail_out = *size - bytes;
    while (inflate(&stream, Z_FINISH) == Z_OK) {
      /* nothing */
    }
  }

  inflateEnd(&stream);

  return buf;
}

void* read_sha1_file(const unsigned char* sha1, char* type,
                     unsigned long *size) {
  unsigned long map_size;
  void* map;
  void* buf;

  map = map_sha1_file(sha1, &map_size);
  if (map) {
    buf = unpack_sha1_file(map, map_size, type, size);
    munmap(map, map_size);
    return buf;
  }

  return NULL;
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
  stream.next_in = (void*)buf;
  stream.avail_in = len;
  stream.next_out = (void*)compressed;
  stream.avail_out = size;
  while (deflate(&stream, Z_FINISH) == Z_OK) {
    /* nothing */
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

static int verify_hdr(struct cache_header* hdr, unsigned long size) {
  SHA_CTX c;
  unsigned char sha1[20];

  if (hdr->signature != CACHE_SIGNATURE) {
    return error("bad signature");
  }

  if (hdr->version != 1) {
    return error("bad version");
  }

  SHA1_Init(&c);
  SHA1_Update(&c, hdr, offsetof(struct cache_header, sha1));
  SHA1_Update(&c, hdr + 1, size - sizeof(*hdr));
  SHA1_Final(sha1, &c);

  if (memcmp(sha1, hdr->sha1, sizeof(sha1)) != 0) {
    return error("bad header sha1");
  }

  return 0;
}

int read_cache() {
  int fd;
  int i;
  unsigned long size = 0;
  unsigned long offset;
  struct stat st;
  void* map;
  struct cache_header* hdr;

  errno = EBUSY;
  if (active_cache) {
    return error("more than one cachefile");
  }

  errno = ENOENT;
  sha1_file_directory = getenv(DB_ENVIRONMENT);
  if (!sha1_file_directory) {
    sha1_file_directory = DEFAULT_DB_ENVIRONMENT;
  }

  if (access(sha1_file_directory, X_OK) != 0) {
    return error("no access to SHA1 file directory");
  }

  fd = open(".tigg/index", O_RDONLY);
  if (fd < 0) {
    return (errno == ENOENT) ? 0 : error("open failed");
  }

  map = MAP_FAILED;
  if (fstat(fd, &st) == 0) {
    size = st.st_size;
    errno = EINVAL;
    if (size >= sizeof(struct cache_header)) {
      map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    }
  }
  close(fd);

  if (map == MAP_FAILED) {
    return error("mmap failed");
  }

  hdr = map;
  if (verify_hdr(hdr, size) < 0) {
    munmap(&map, size);
    errno = EINVAL;
    return error("verify header failed");
  }

  active_nr = hdr->entries;
  active_alloc = alloc_nr(active_nr);
  active_cache = calloc(active_alloc, sizeof(struct cache_entry*));

  offset = sizeof(*hdr);
  for (i = 0; i < hdr->entries; i++) {
    struct cache_entry* ce = map + offset;
    offset += ce_size(ce);
    active_cache[i] = ce;
  }
  return active_nr;
}

int remove_file_from_cache(const char* path) {
  int pos;

  pos = cache_name_pos(path, strlen(path));
  if (pos < 0) {
    pos = -pos - 1;
    active_nr--;
    if (pos < active_nr) {
      memmove(active_cache + pos, active_cache + pos + 1,
              (active_nr - pos) * sizeof(struct cache_entry*));
    }
  }
  return 0;
}

int add_cache_entry(struct cache_entry* ce) {
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

int write_cache(int fd, struct cache_entry** cache, int entries) {
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
