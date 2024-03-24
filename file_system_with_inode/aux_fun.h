#pragma once

#include "simplefs.h"

typedef struct {
    int pos_in_dir;
    int block_in_disk;
} InfoBlock;

//to write a new index block in memory, return -1 if memory is full, 0 otherwise
int new_index_block(DirectoryHandle* d, IndexBlock* index_block, int pos);

//to write a first directory block in memory, return NULL if memory is full, otherwise point to new struct
FirstDirectoryBlock* new_dir_block(DirectoryHandle* d, const char* dirname);

//to write a first file block in memory, return NULL if memory is full, otherwise point to new struct
FirstFileBlock* new_file_block(DirectoryHandle* d, const char* filename);

//to write a data block in memory, return NULL if memory is full, otherwise point to new struct
FileBlock* file_block(FileHandle* pf, FileBlock* fb, int block_in_file);

//Initialization of file handle
FileHandle* f_handle(DirectoryHandle* d, FirstFileBlock* ffb);

//Initialization of directory handle (side-effect on dir_h)
void d_handle(DirectoryHandle* d, FirstDirectoryBlock* fdb, int pos_in_file, DirectoryHandle* dir_h);

//to color directory in ls command
void color_dir(FirstFileBlock* f, char* dest);

//to color file in ls command
void color_file(FirstFileBlock* f, char* dest);

//to check if filename exist, return negative if find file, 0 otherwise
int control_existence(const char* filename, FirstFileBlock* control, int is_dir);

//aux to control the existance of filename in direct block
//return 0 if file_block -1, 
int control_direct_block(char** names, DirectoryHandle* d, const char* filename, int file_block, int i, int n_file, int is_dir, int change);

//aux to control the existance of filename in direct block
int control_index_block(char** names, DirectoryHandle* d, const char* filename, int file_block, int i, int n_file, IndexBlock* iblock, int is_dir, int change);

//aux to control the existance of filename in single block
int control_single_block(char** names, DirectoryHandle* d, const char* filename, int single_block, int i, int n_file, int is_dir, int change);

//aux to control the existance of filename in double block
int control_double_block(char** names, DirectoryHandle* d, const char* filename, int double_block, int i, int k, int n_file, int is_dir, int change);

//aux to control the existance of filename in triple block
int control_triple_block(char** names, DirectoryHandle* d, const char* filename, int triple_block, int i, int k, int z, int n_file, int is_dir, int change);

//to find the last index != -1 in index block
int find_last_index(DirectoryHandle* d, int entry, int sost);

//to find the first index free in index block
int first_free_index(FileHandle* f, int new_block, int pos_to_search, int search);

//to search file to remove in direct block
InfoBlock* direct_rem(DirectoryHandle* d, const char* filename, int file_block, int i, int n_file, InfoBlock* info);

//to search file to remove in index block
InfoBlock* index_rem(DirectoryHandle* d, const char* filename, int file_block, int i, int n_file, IndexBlock* iblock, InfoBlock* info);

//to search file to remove in single block
InfoBlock* single_rem(DirectoryHandle* d, const char* filename, int single_block, int i, int n_file, InfoBlock* info);

//to search file to remove in double block
InfoBlock* double_rem(DirectoryHandle* d, const char* filename, int double_block, int i, int k, int n_file, InfoBlock* info);

//to search file to remove in triple block
InfoBlock* triple_rem(DirectoryHandle* d, const char* filename, int triple_block, int i, int k, int z, int n_file, InfoBlock* info);

//loop to remove the file data block
int remove_data(DiskDriver* disk, FirstFileBlock* file_to_rm);

//search of file to remove
//return > 0, with number of block to read
//return negative otherwise 
int search_file(DirectoryHandle* d, const char* filename, InfoBlock* info);

//loop to write file data block size DATA on empty file or in overwrite , return number of bytes written
int str_to_wrt(FileHandle* f, void* data, int to_write, int written, int num_block, int size);

//loop to write file data block size DATA in append, return number of bytes written
int str_to_wrt_app(FileHandle* f, void* data, int to_write, int written, int size);

//loop to read file data block, return number of bytes read
int str_to_read(FileHandle* f, void* data, int dim_read, int read_bytes, int block, int offset);