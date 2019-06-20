#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include "disk.h"

// ANDREW LAI 9519687 CS170 W19 PA4

/* Declared in "myfs.h":
int make_fs(char *disk_name);
int mount_fs(char *disk_name);
int umount_fs(char *disk_name);

int fs_open(char *name);
int fs_close(int fildes);
int fs_create(char *name);
int fs_delete(char *name);
int fs_read(int fildes, void *buf, size_t nbyte);
int fs_write(int fildes, void *buf, size_t nbyte);
int fs_get_filesize(int fildes);
int fs_lseek(int fildes, off_t offset);
int fs_truncate(int fildes, off_t length);
*/

/* Declared in "disk.h":
#define DISK_BLOCKS  8192      // number of blocks on the disk
#define BLOCK_SIZE   4096      // block size on "disk" 

int make_disk(char *name);     // create an empty, virtual disk file
int open_disk(char *name);     // open a virtual disk (file)
int close_disk();              // close a previously opened disk (file)

int block_write(int block, char *buf); // write a block of size BLOCK_SIZE to disk
int block_read(int block, char *buf); // read a block of size BLOCK_SIZE from disk 
*/

// Declared here :^)
#define MAX_FILENAME 16         // max length of filename (15 characters)
#define MAX_FILES 64            // max number of files
#define MAX_DESCRIPTORS 32      // max number of descriptors

#define SUPER_BLOCK 0           // super block
#define BLOCK_FREE 0            // indicates block is free
#define LAST_BLOCK -2           // indicates block is last in file

// directory entry (file)
typedef struct t_directory_entry {
  char filename[MAX_FILENAME];  // max length 15; char *name
  unsigned int size;            // file size; returned by fs_get_filesize
  short head;                   // offset of first block in file; 0 if free
} directory_entry;

// file descriptor (fildes)
typedef struct t_file_descriptor {
  int index;                    // index in directory/offset of first block in file
  int offset;                   // seek offset; indicates current position in file
} file_descriptor;

// directory & directory setup
directory_entry directory[MAX_FILES];               // files in root directory
file_descriptor descriptor_table[MAX_DESCRIPTORS];  // file descriptor array
int descriptors; // idk man u tell me

// ** HELPER FUNCTIONS ** //
// search through directory table for first file with a given name
// return index of file in directory table on success, -1 on failure
int search_directory(char* fname);

// get index of next linked block (first two bytes of block on disk)
// return index of next block on success, -1 on failure
int get_block_ptr(int block);

// set index of next linked block (first two bytes of block on disk)
// modifies ptr; doesn't alter content of block in question
// return 0 on success, -1 on failure
int set_block_ptr(int block, short ptr);

// read (BLOCK_SIZE - 2) bytes from block into buf
// return index of next block on success, -1 on failure, -2 on end-of-file
int read_block(int block, char *buf);

// write (BLOCK_SIZE - 2) bytes into block from buf
// return index of next block on success, -1 on failure, -2 on end-of-file
int write_block(int block, char *buf);

// allocate the first free block & empty old content
// return first free block on success, -1 on failure
int allocate_block(void);

// return the block that the file descriptor currently seeked to
// return the seeked block on success, -1 on failure, -2 if overseeked
int current_seeked(int fildes);

// frees every block in a linked list, starting with index head
void free_list(int head);

// --
// ** fucks sake :^) fs implementation ** //

// make empty filesystem; returns 0 on success
int make_fs(char* disk_name){
  if(make_disk(disk_name)) {
    printf("make_fs: Could not create disk.\n");
    return -1; // a failure, like me :^)
  }
  return 0; // success. :V can't relate
}

// mounts the filesystem/makes it "ready to use"; returns 0 on success
int mount_fs(char* disk_name){
  if(open_disk(disk_name)) {
    printf("mount_fs: Could not open disk.\n");
    return -1;
  }

  // read entire super block into buffer
  char* buffer = malloc(BLOCK_SIZE);
  if(block_read(SUPER_BLOCK, buffer)) {
    printf("mount_fs: Failed to read super block from disk.\n");
    return -1;
  }

  // extract directory table into memory
  memcpy(&directory, buffer, sizeof(directory));
  free(buffer);

  // initialize descriptor table
  int i;
  for (i = 0; i < MAX_DESCRIPTORS; i++) {
    descriptor_table[i].index = -1;
  }
  descriptors = 0;

  return 0; // success
}

// unmounts filesystem, write back all meta information; returns 0 on success
int umount_fs(char* disk_name){
  if (descriptors > 0) {
    printf("umount_fs: There are still open file descriptors.\n");
    return -1;
  }

  // copy directory table into super block buffer
  char* buffer = malloc(BLOCK_SIZE);
  memset(buffer, 0, BLOCK_SIZE);
  memcpy(buffer, &directory, sizeof(directory));

  // write super block to disk
  block_write(SUPER_BLOCK, buffer);
  free(buffer);

  if(close_disk()) {
    printf("umount_fs: Could not close disk.\n");
    return -1;
  }

  return 0; // success
}

// file "name" is opened for read/write; returns file descriptor
int fs_open(char* name){
  // get file from directory
  int index = search_directory(name);
  if (index == -1) {
    printf("fs_open: Couldn't find file %s.\n", name);
    return -1;
  }

  // insert new file descriptor into first open slot in table
  int i;
  for (i = 0; i < MAX_DESCRIPTORS; i++) {
    if (descriptor_table[i].index == -1) {
      descriptor_table[i].index = index;
      descriptor_table[i].offset = 0;
      return i;
    }
  }

  // error if table is full
  printf("fs_open: Too many open file descriptors.\n");
  return -1;
}

// closes file descriptor filedes; returns 0 on success
int fs_close(int fildes){
  // check if fildes invalid
  if (fildes < 0 || fildes >= MAX_DESCRIPTORS
      || descriptor_table[fildes].index == -1) {
    printf("fs_close: Invalid file descriptor.\n");
    return -1;
  } else {
    descriptor_table[fildes].index = -1;
    return 0;   // success
  }
}

// create new file with name name in root; returns 0 on success
// max 15 characters in filename, max 64 files in directory
int fs_create(char* name){
  int len = strlen(name); // check filename length; max 15 chars
  if (len >= MAX_FILENAME) {
    printf("fs_create: File name too long (> 15 characters).\n");
    return -1;
  }
  
  // find first free entry in root, and check if name in use */
  int index = -1;
  int i;
  for (i = 0; i < MAX_FILES; i++) {
    if (directory[i].head == 0) { // entry available
      if (index == -1) {
        index = i;
      }
    } else { // entry in use
      if (!strcmp(name, directory[i].filename)) {
        // filename in use
        printf("fs_create: File %s already exists on disk.\n", name);
        return -1;
      }
    }
  }

  // if no free entries, max number of files/fs capacity reached
  if (index == -1) {
    printf("fs_create: Disk is at file capacity (64 files).\n");
    return -1;
  }

  // get the next available block
  int start_block = allocate_block();
  if (start_block == -1) {
    printf("fs_create: Block allocation failed.\n");
    return -1;
  }
    
  // set directory name to new name, and pad with 0s
  for (i = 0; i < MAX_FILENAME; i++) {
    directory[index].filename[i] = (i < len ? name[i] : 0);
  }

  directory[index].head = start_block;
  directory[index].size = 0;

  return 0; // success
}

// deletes file "name" from root & frees all data blocks/meta info
// returns 0 on success, -1 on failure
int fs_delete(char* name){
  int index = search_directory(name); // find the file in directory
  if (index == -1) {
    printf("fs_delete: File %s does not exist on disk.\n", name);
    return -1;
  }

  // make sure no open file descriptors to file, i.e. file isn't open
  int i;
  for (i = 0; i < MAX_DESCRIPTORS; i++) {
    if (descriptor_table[i].index == index) {
      printf("fs_delete: There are open descriptors to file %s.\n", name);
      return -1;
    }
  }
  free_list(directory[index].head); // marks all blocks in list as free
  directory[index].head = 0;        // mark directory entry as free

  return 0; // success
}

// attempts to read nbyte bytes of data from fildes into buffer buf
// returns number of bytes actually read, -1 on failure
int fs_read(int fildes, void* buf, size_t nbyte){
  // assume buffer buf large enough to hold at least nbyte bytes
  int num_read = 0; // holds number of bytes read

  int current_block = current_seeked(fildes);
  int offset_in_block = descriptor_table[fildes].offset % (BLOCK_SIZE - 2);
  char tmp_buf[BLOCK_SIZE - 2];

  // big mood incoming :^)

  // while not done reading bytes
  while((nbyte > 0) && (current_block > 0)) {
    if((current_block = read_block(current_block, tmp_buf)) == -1) {
      printf("fs_read: Unable to read from file\n");
      return -1;
    }

    // if not past end of block, finish
    if((nbyte + offset_in_block) <= (BLOCK_SIZE - 2)) {
      memcpy(buf + num_read, tmp_buf + offset_in_block, nbyte);
      num_read += nbyte;
      descriptor_table[fildes].offset += nbyte;
      nbyte = 0;
      offset_in_block = 0;

    // read all bytes in block & continue
    } else {
      memcpy(buf + num_read, tmp_buf + offset_in_block, (BLOCK_SIZE - 2) - offset_in_block);
      num_read += (BLOCK_SIZE - 2) - offset_in_block;
      descriptor_table[fildes].offset += (BLOCK_SIZE - 2) - offset_in_block;
      nbyte -= (BLOCK_SIZE - 2) - offset_in_block;
      offset_in_block = 0;
    }
  }

  // if reached end of file/last block, but not done reading
  if((nbyte > 0) && (current_block == LAST_BLOCK)) {
    // finish reading if not past end of file
    if((nbyte + descriptor_table[fildes].offset) < directory[descriptor_table[fildes].index].size) {
      memcpy(buf + num_read, tmp_buf + offset_in_block, nbyte);
      num_read += nbyte;
      descriptor_table[fildes].offset += nbyte;
      nbyte = 0;

    // read all bytes until end of file
    } else {
      memcpy(buf + num_read, tmp_buf + offset_in_block, directory[descriptor_table[fildes].index].size - descriptor_table[fildes].offset);
      num_read += directory[descriptor_table[fildes].index].size - descriptor_table[fildes].offset;
      descriptor_table[fildes].offset += directory[descriptor_table[fildes].index].size - descriptor_table[fildes].offset;
    }
  }

  return num_read; // return number of bytes read
}

// attempts to write nbyte bytes of data from fildes into buffer buf
// returns number of bytes actually written, -1 on failure
int fs_write(int fildes, void* buf, size_t nbyte){
  // assume buffer buf holds at least nbyte bytes

  // check if fildes invalid
  if (fildes < 0 || fildes >= MAX_DESCRIPTORS
      || descriptor_table[fildes].index == -1) {
    printf("fs_close: Invalid file descriptor.\n");
    return -1;
  }

  char* buf_as_char = (char*) buf;
  int block_i = current_seeked(fildes);
  char block_buffer[BLOCK_SIZE - 2];
  int sub_i = descriptor_table[fildes].offset % (BLOCK_SIZE - 2);

  int num_written = 0; // contains number of bytes written
  while (num_written < nbyte) {
    // read current block into buffer
    if (read_block(block_i, block_buffer) == -1) {
      printf("fs_write: Couldn't read block from disk.\n");
      return -1;
    }

    // write input buffer to block buffer & increment pointer
    for (; sub_i < BLOCK_SIZE - 2 && num_written < nbyte; sub_i++, num_written++, descriptor_table[fildes].offset++) {
      block_buffer[sub_i] = buf_as_char[num_written];
    }

    // write block buffer to disk
    int new_block;
    if ((new_block = write_block(block_i, block_buffer)) == -1) {
      printf("fs_write: Couldn't write block to disk.\n");
      return -1;
    }

    // extend file if needed
    if (num_written < nbyte && new_block == LAST_BLOCK) {
      if ((new_block = allocate_block()) == -1) {
        printf("fs_write: Out of disk space.\n");
        break;
      }
      if (set_block_ptr(block_i, new_block)) {
        printf("fs_write: Block allocation failed.\n");
        break;
      }
      sub_i = 0;
    }
    block_i = new_block; // point to new block
  }
  
  // set size based on number of bytes written
  if (descriptor_table[fildes].offset >
      directory[descriptor_table[fildes].index].size) {
    directory[descriptor_table[fildes].index].size = descriptor_table[fildes].offset;
  }

  return num_written; // return number of bytes written
}

// return current size of file pointed to by file descriptor fildes, -1 on failure
int fs_get_filesize(int fildes){
  // check if fildes invalid or closed
  if (descriptor_table[fildes].index < 0) {
    printf("fs_get_filesize: File descriptor closed (fd %d)\n", fildes);
    return -1;
  }

  // return filesize
  return directory[descriptor_table[fildes].index].size;
}

// set file pointer (offset used for read/write) assoc with fildes to offset
// return 0 on success, -1 on failure
// example: fs_lseek(fd, fs_get_filesize(fd));
int fs_lseek(int fildes, off_t offset){
  // get filesize
  int fsize = fs_get_filesize(fildes);
  if (fsize == -1) {
    printf("fs_lseek: Cannot determine size of file.\n");
    return -1;
  }
  
  // check valid offset
  if (offset < 0 || offset > fsize) {
    printf("fs_lseek: Seek offset out of bounds.\n");
    return -1;
  }

  // set offset
  descriptor_table[fildes].offset = offset;
  return 0; // success
}

// truncate file pointed to by fildes to a size of length bytes
// return 0 on success, -1 on failure
int fs_truncate(int fildes, off_t length){
  // get filesize
  int fsize = fs_get_filesize(fildes);
  if (fsize == -1) {
    printf("fs_truncate: Cannot determine size of file.\n");
    return -1;
  }

  // impossible to extend a file using fs_truncate
  if (length > fsize) {
    printf("fs_truncate: Cannot truncate to length greater than file size.\n");
    return -1;
  } else if (length < fsize) { // valid; begin truncating
    int new_blocksize = length / (BLOCK_SIZE - 2);
    int block_i = directory[descriptor_table[fildes].index].head;
    int i;
    for (i = 0; i < new_blocksize; i++) {
      block_i = get_block_ptr(block_i);
    }
    int tail = get_block_ptr(block_i);

    // set truncated tail block
    if (set_block_ptr(tail, LAST_BLOCK)) {
      printf("fs_truncate: Couldn't set new file end block.\n");
      return -1;
    }

    // free corresponding data blocks & extra data
    free_list(tail);
  }

  return 0; // success
}

// search through directory table for first file with a given name
// return index of file in directory table, -1 on failure
int search_directory(char* fname) {
  int i;
  for (i = 0; i < MAX_FILES; i++) {
    if (directory[i].head != 0
        && !strcmp(fname, directory[i].filename)) {
      return i;
    }
  }
  return -1;
}

// get index of next block in chain (first two bytes of block on disk)
// return index of next block, -1 on failure
int get_block_ptr(int block) {
  char buffer[BLOCK_SIZE];

  if (block_read(block, buffer)) {
    printf("get_block_ptr: Error reading block %d.\n", block);
    return -1;
  }

  // :^)
  short* as_short = (short*) buffer;
  return (int) as_short[0];
}

// set index of next block in chain (first two bytes of block on disk)
// modifies ptr; doesn't alter content of block in question
// return 0 on success, -1 on failure
int set_block_ptr(int block, short ptr) {
  char buffer[BLOCK_SIZE];

  if (block_read(block, buffer)) {
    printf("set_block_ptr: Error reading block %d.\n", block);
    return -1;
  }

  memcpy(buffer, &ptr, 2); // get first 2 bytes

  if (block_write(block, buffer)) {
    printf("set_block_ptr: Error writing block %d.\n", block);
    return -1;
  }

  return 0; // success
}

// read (BLOCK_SIZE - 2) bytes from block into buf
// return index of next block on success, -1 on failure, -2 on end-of-file
int read_block(int block, char *buf) {
  char tmp_buf[BLOCK_SIZE];
  int next_block;

  if (block_read(block, tmp_buf)) {
    printf("read_block: Error reading the block %d.\n", block);
    return -1;
  }

  next_block = ((short *)tmp_buf)[0]; // the first two bytes are the next block

  // copy (BLOCK_SIZE - 2) bytes into buf, starting at tmp_buf+2
  memcpy(buf, tmp_buf + 2, BLOCK_SIZE - 2);

  return next_block;
}

// write (BLOCK_SIZE - 2) bytes into block from buf
// return index of next block on success, -1 on failure, -2 on (current) end-of-file
int write_block(int block, char *buf) {
  char tmp_buf[BLOCK_SIZE];
  short next_block;

  if(block_read(block, tmp_buf)) {
    printf("write_block: Error reading the block %d.\n", block);
    return -1;
  }
  next_block = ((short *)tmp_buf)[0]; // the first two bytes are the next block

  // copy (BLOCK_SIZE - 2) bytes into tmp_buf, starting at tmp_buf+2
  memcpy(tmp_buf + 2, buf, BLOCK_SIZE - 2);

  if(block_write(block, tmp_buf)) {
    printf("write_block: Error writing the block %d.\n", block);
    return -1;
  }

  return (int) next_block;
}

// allocate the first free block & empty old content
// return first free block on success, -1 on failure
int allocate_block(void) {
  int block_i = -1, i;
  char zeros[BLOCK_SIZE-2] = {0};

  // allocate first free block
  for (i = 1; i < DISK_BLOCKS; i++) {
    if (get_block_ptr(i) == BLOCK_FREE) {
      block_i = i;
      break;
    }
  }

  // if no blocks are free, disk is at capacity
  if (block_i == -1) {
    printf("allocate_block: Disk is at block capacity.\n");
    return -1;
  }

  // mark block as allocated
  if(set_block_ptr(block_i, LAST_BLOCK)) {
    printf("allocate_block: Block allocation on block %d failed.\n", block_i);
    return -1;
  }

  // clear out the block
  if(write_block(block_i, zeros) == -1) {
    printf("allocate_block: Block zeroing on block %d failed.\n", block_i);
    return -1;
  }

  return block_i;
}

// return the block that the file descriptor currently seeked to
// return the seeked block on success, -1 on failure, -2 if overseeked
int current_seeked(int fildes) {
  int next_block = directory[descriptor_table[fildes].index].head;
  int i = descriptor_table[fildes].offset;

  // don't fuck thsi up
  for(; i > BLOCK_SIZE - 2; i -= BLOCK_SIZE - 2) {
    if(next_block == LAST_BLOCK) {
      printf("current_seeked: Overseeked the file (fd %d)\n", fildes);
      return LAST_BLOCK;
    } else if((next_block = get_block_ptr(next_block)) == -1) {
      printf("current_seeked: Failed to get next block pointer (fd %d)\n", fildes);
      return -1;
    }
  }
  
  return next_block;
}

// frees every block in a linked list, starting with index head
void free_list(int head) {
  if(head == LAST_BLOCK || head == BLOCK_FREE) {
    return;
  }
  int tail = get_block_ptr(head);  
  set_block_ptr(head, BLOCK_FREE);
  free_list(tail);
}