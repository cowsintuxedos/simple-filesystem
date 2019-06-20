# Simple File System - mostly complete
For use in presentation; run `make` in directory `fs` to generate `fs.o`.  
Run `make` in directory `/example` and run the generated testcases, instead, to test it.

`btree_persistent` contains an attempt at coding a persistent B+-tree, as well, though it's mostly incomplete at this point.  

## Design
The filesystem is implemented as a collection of blocks, using the initial declarations from `disk.h` and `disk.c`, i.e. 8192 blocks of size 4KB.

The first block - i.e. the super block - actually contains the file allocation table/FAT itself; each file is handled as a directory entry in the FAT, containing the file's name, filesize, and the offset of the first block in the file. This super block containing the FAT is read from/written to disk when the filesystem in mounted/unmounted, keeping the state of the system and the files persistent.

Otherwise, the first two bytes of each block contains either (1) the block offset of the next block in the file, (2) `BLOCK_FREE` indicating the block isn't allocated to a file, or (3) `LAST_BLOCK` indicating the block is the last in the file; essentially, each block is part of a linked list. `BLOCK_FREE` is declared to `0`, and `LAST_BLOCK` to `-2`, to avoid indexing conflicts. New instances of the the filesystem are essentially made up of blocks all set to `BLOCK_FREE`, i.e. `0`.

File descriptors consist of (1) the index in the directory/offset of first block in file, and (2) a seek offset indicating the current position in the file. Unlike the FAT, these are stored in memory, rather than on the disk.

## Minor Implementation Details
```
#define MAX_FILENAME 16         // max length of filename (15 characters)
#define MAX_FILES 64            // max number of files
#define MAX_DESCRIPTORS 32      // max number of descriptors

#define SUPER_BLOCK 0           // super block
#define BLOCK_FREE 0            // indicates block is free
#define LAST_BLOCK -2           // indicates block is last in file
```
The above declarations are used for initializations and conditionals.


```
if (fildes < 0 || fildes >= MAX_DESCRIPTORS || descriptor_table[fildes].index == -1)
```
Used to check if a file descriptor is invalid. Relatively common in the code, but not complex enough to justify being a helper function.


See the comments in the code for anything else :^)


## Helper Function Details
```
// search through directory table for first file with a given name
// return index of file in directory table on success, -1 on failure
int search_directory(char* fname);
```
Used for finding files; used in `fs_open` and `fs_delete`.


```
// get index of next linked block (first two bytes of block on disk)
// return index of next block on success, -1 on failure
int get_block_ptr(int block);
```
Links blocks together (accessor - gets index of next linked block); used in `fs_truncate`, `allocate_block`, `current_seeked`, and `free_list`.


```
// set index of next linked block (first two bytes of block on disk)
// modifies ptr; doesn't alter content of block in question
int set_block_ptr(int block, short ptr);
```
Links blocks together (mutator - sets index of next linked block); used in `fs_write`, `fs_truncate`, `allocate_block`, and `free_list`.


```
// read (BLOCK_SIZE - 2) bytes from block into buf
// return index of next block on success, -1 on failure, -2 on end-of-file
int read_block(int block, char *buf);
```
Reads data from a block into a buffer (modified for linked block implementation); used in `fs_read` and `fs_write`. `-2` comes from the value of `LAST_BLOCK`.


```
// write (BLOCK_SIZE - 2) bytes into block from buf
// return index of next block on success, -1 on failure, -2 on end-of-file
int write_block(int block, char *buf);
```
Writes data from a buffer into a block (modified for linked block implementation); used in `fs_write` and `allocate_block`. `-2` comes from the value of `LAST_BLOCK`.


```
// allocate the first free block & empty old content
// return first free block on success, -1 on failure
int allocate_block(void);
```
Used to allocate the next available block; used in `fs_create` and `fs_write`.


```
// return the block that the file descriptor currently seeked to
// return the seeked block on success, -1 on failure, -2 if overseeked
int current_seeked(int fildes);
```
Access the block currently pointed to by a file descriptor; used in `fs_read` and `fs_write`.


```
// frees every block in a linked list, starting with index head
void free_list(int head);
```
Recursively frees every block in a list, i.e. sets each block to `BLOCK_FREE` and removes the link; used in `fs_delete`, `fs_truncate`, and `free_list` itself, due to recursion.


## Outputs from running the provided testcases:
From the included `example`.

```
./basic_fs_01.run
```

```
./basic_fs_02.run
ERROR: data read does not match data written!`
```

```
./basic_fs_03.run
ERROR: data read does not match data written!`
```

```
./basic_fs_04.run
ERROR: file1 has incorrect size
```