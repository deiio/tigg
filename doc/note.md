# Important Thoughts

## `init-db`

1. Create local cache directory `.dircache`.
2. If set the environment variable `SHA1_FILE_DIRECTORY`, use
it as the storage area.
3. Or, use the local cache directory `.dircache` as the storage area.
4. Create all directories in storage area.

## `commit-tree`

1. Use `commit-tree <sha1> [-p <sha1>]* < changelog`.
2. Read in all parents.
3. Get committer's information, such as name, email, date.
4. commit-tree original structure:
   1. `commit + <space> + <size> + '\0'`. The size is the sum of the 
   following 5 items.
   2. `tree` + commit sha1.
   3. `parent` + parents sha1.
   4. author information. `author` + `gecos <email> date`.
   5. committer information. `committer` + `gecos <email> date`.
   6. comments.
5. commit-tree original information will deflate with `zlib`.
The contents deflated will be written to the file named its contents
of hash sha1.

## `cat-file`

1. Use `cat-file <sha1>`.
2. Read in the file.
3. Output its type and write the content to temperate file.

## `update-cache`

1. Use `update-cache [filepath ...]`.
2. Read cached entries from `.dircache/index`.
3. Create the file `.dircache/index.lock` while processing the entries.
4. Add all file to the cache.
   1. Verify the file path, filter out dot, dot-dot, absolute path,
   hidden file, non-regular file.
   2. Add the file to objects and add it to entries.
5. Blob object original structure:
   1. `blob + <space> + <size> + '\0'`.
   2. blob data.
6. Write the cache records to `.dircache/index.lock`, and rename it to
`.dircache/index`.

## `show-diff`

1. Use `show-diff`.
2. Read cached entries from `.dircache/index`.
3. Check the status of every file cached.
   1. Check the modifications of the file, which is changed by `mknod`,
   `truncate`, `utime`, `write`.
   2. Check the change time of the file, which is changed by writing or 
   setting inode information, i.e., owner, group, link count, mode, etc.
   3. Check the owner of the file.
   4. Check the mode and type of the file.
   5. Check the inode of the file.
   6. Check the file size.
4. If the file has been changed, show the difference of the file used
`diff` command.

## `write-tree`

1. Use `write-tree`.
2. Read cached entries from `.dircache/index`.
3. Write cached file to tree object.
4. Tree object original structure:
   1. `tree + <space> + <size> + '\0'`.
   2. `<file mode> + <space> + <filename> + '\0' + <sha1>`.

## `read-tree`

1. Use `read-tree <sha1>`.
2. Read the object, and check its type.
3. Output all the files in the tree object.
