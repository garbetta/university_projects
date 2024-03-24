#pragma once
#include "disk_driver.h"

#define DIRECT_BLOCKS_NUM     12
#define INDIRECT_BLOCKS_NUM   3

#define INDEXES               ((BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int))

#define DATA_FIRST            (BLOCK_SIZE-sizeof(FileControlBlock)-sizeof(BlockHeader)-DIRECT_BLOCKS_NUM*sizeof(int)-INDIRECT_BLOCKS_NUM*sizeof(int))
#define DATA                  (BLOCK_SIZE-sizeof(BlockHeader))

#define MEM_FS                1024

#define CR      "\033[1;31m"
#define CG      "\033[1;32m"
#define CY      "\033[1;33m"
#define CB      "\033[1;34m"
#define CM      "\033[1;35m"
#define CC      "\033[1;36m"
#define RES     "\033[0m" 

/*these are structures stored on disk*/

// header, occupies the first portion of each block in the disk
// represents a chained list of blocks
typedef struct {  //non uso la lista concatenata, ma inode, elimino i campi 
  int block_in_disk;
  int block_in_file;  // position in the file, if 0 we have a file control block
  int is_data_block;  // 1 data block, 0 index block
} BlockHeader;

// this is in the first block of a chain, after the header
typedef struct {
  int directory_block; // first block of the parent directory
  int block_in_disk;   // repeated position of the block on the disk
  char name[128];
  int size_in_bytes;
  int size_in_blocks;
  int is_dir;          // 0 for file, 1 for dir
} FileControlBlock;

// this is the first physical block of a file
// it has a header
// an FCB storing file infos
// and can contain some data

/******************* stuff on disk BEGIN *******************/

// this is the first physical block of a directory
typedef struct {
  BlockHeader header;
  FileControlBlock fcb;
  int direct_blocks[DIRECT_BLOCKS_NUM];
  int indirect_blocks[INDIRECT_BLOCKS_NUM];
  int num_entries;
  char null[BLOCK_SIZE - sizeof(BlockHeader) - sizeof(FileControlBlock) - 4 - 4*DIRECT_BLOCKS_NUM - 4*INDIRECT_BLOCKS_NUM];
} FirstDirectoryBlock;

typedef struct{
  BlockHeader header;
  int file_blocks[INDEXES];
  char null[BLOCK_SIZE - 4*INDEXES];
} IndexBlock;

//*******************

typedef struct {
  BlockHeader header;
  FileControlBlock fcb;
  int direct_blocks[DIRECT_BLOCKS_NUM];
  int indirect_blocks[INDIRECT_BLOCKS_NUM];
  char data[DATA_FIRST];
} FirstFileBlock;

// this is one of the next physical blocks of a file
typedef struct {
  BlockHeader header;
  char  data[DATA];
} FileBlock;

/******************* stuff on disk END *******************/
typedef struct {
  DiskDriver* disk;
} SimpleFS;

// this is a file handle, used to refer to open files
typedef struct {
  SimpleFS* sfs;                   // pointer to memory file system structure
  FirstFileBlock* fcb;             // pointer to the first block of the file(read it)
  FirstDirectoryBlock* directory;  // pointer to the directory where the file is stored
  //BlockHeader* current_block;      // current block in the file -->il blocco dove ho il cursore
  int pos_in_file;                 // position of the cursor
} FileHandle;

typedef struct {
  SimpleFS* sfs;                   // pointer to memory file system structure
  FirstDirectoryBlock* dcb;        // pointer to the first block of the directory(read it)
  FirstDirectoryBlock* directory;  // pointer to the parent directory (null if top level)
  //BlockHeader* current_block;      // current block in the directory
  int pos_in_dir;                  // absolute position of the cursor in the directory
  int pos_in_block;                // relative position of the cursor in the block
} DirectoryHandle;

// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk);

// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs);

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename);

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d);

// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename);

// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f);

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size);

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size);

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos);

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
int SimpleFS_changeDir(DirectoryHandle* d, char* dirname);

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname);

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(DirectoryHandle* d/*SimpleFS* fs*/, char* filename);

//print block index
void print_index_block(FirstDirectoryBlock* db, DiskDriver* disk);