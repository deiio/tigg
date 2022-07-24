# Important Thoughts

## init-db

1. Create local cache directory `.dircache`.
2. If set the environment variable `SHA1_FILE_DIRECTORY`, use
it as the storage area.
3. Or, use the local cache directory `.dircache` as the storage area.
4. Create all directories in storage area.

# commit-tree

1. Use `commit-tree <sha1> [-p <sha1>]* < changelog`.
2. Read in all parents.
3. Get committer's information, such as name, email, date.
4. commit-tree original structure:
   1. tag + size + '\0'. The size is the sum of the following 5 items.
   2. `tree` + commit sha1.
   3. `parent` + parents sha1.
   4. author information. `author` + `gecos <email> date`.
   5. committer information. `committer` + `gecos <email> date`.
   6. comments.
5. commit-tree original information will deflate with `zlib`.
The contents deflated will be written to the file named its contents
of hash sha1.

