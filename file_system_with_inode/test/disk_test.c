//#include "disk_driver.h"
#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIM_MEM 16

void print_status_mem(DiskDriver* disk){

    printf("\nINFO DISK MEMORY\n");
    print_bitmap_bit(disk->bitmap_data, disk->header->bitmap_entries);
    printf("memory contains %d blocks\nbitmap num_blocks %d , %d bytes\nfirst free block %d, num free blocks %d\n", 
            disk->header->num_blocks, disk->header->bitmap_blocks, 
            disk->header->bitmap_entries, 
            disk->header->first_free_block, disk->header->free_blocks);
}

int main(int argc, char** argv){
    
    int ret;
    DiskDriver* mem = (DiskDriver*)malloc(sizeof(DiskDriver));
    FirstFileBlock* file = (FirstFileBlock*)calloc(1, sizeof(FirstFileBlock));
    FirstFileBlock out_struct = {0};//(FirstFileBlock*)calloc(1, sizeof(FirstFileBlock));

    DiskDriver_init(mem, "memory.txt", DIM_MEM);
    DiskDriver_flush(mem);
    print_status_mem(mem);

    file->header.block_in_disk = mem->header->first_free_block;
    file->header.is_data_block = 0;
    file->fcb.block_in_disk = mem->header->first_free_block;
    file->fcb.directory_block = -1;
    strcpy(file->fcb.name, "first_file");
    file->fcb.size_in_blocks = 1;
    file->fcb.size_in_bytes = 0;
    file->fcb.is_dir = 0;
    memset(file->data, 0, DATA_FIRST);
    for(int i = 0; i < DIRECT_BLOCKS_NUM; i++) file->direct_blocks[i] = -1;
    for(int i = 0; i < INDIRECT_BLOCKS_NUM; i++) file->indirect_blocks[i] = -1;

    printf("read 5-th block\n");
    ret = DiskDriver_readBlock(mem, file, 5);
    if(ret != -1){
        printf("not empty block\n");
    }

    char t[BLOCK_SIZE] = {'\0'};
    char* out = (char*)calloc(BLOCK_SIZE, sizeof(char));
    memset(t, 'a', BLOCK_SIZE);

    printf("write FirstFileBlock in 5-th block\n");
    ret = DiskDriver_writeBlock(mem, file, 5);
    error(ret, "error write");
    DiskDriver_flush(mem);

    printf("write FirstFileBlock in 10-th block\n");
    ret = DiskDriver_writeBlock(mem, file, 10);
    error(ret, "error write");
    DiskDriver_flush(mem);

    printf("write 0-th block\n");
    ret = DiskDriver_writeBlock(mem, t, 0);
    error(ret, "error write");

    printf("\nprint block 0\n");
    ret = DiskDriver_readBlock(mem, out, 0);
    error(ret, "error in read");
    DiskDriver_flush(mem);
    for(int i = 0; i < BLOCK_SIZE; i++)
        printf("%c", out[i]);
    printf("\n");
   
    print_bitmap_bit(mem->bitmap_data, mem->header->bitmap_entries);

    printf("free blocks: %d\n", mem->header->free_blocks);
    DiskDriver_readBlock(mem, &out_struct, 5);
    printf("\nfilename of file written on disk: %s\n", out_struct.fcb.name);
    
    
    printf("\n\nfirst free block: %d\n", mem->header->first_free_block);
    printf("free blocks: %d\n", mem->header->free_blocks);
    
    memset(t, '5', BLOCK_SIZE);
    printf("rewrite block 0\n");
    ret = DiskDriver_rewriteBlock(mem, t, 0);
    error(ret, "error in rewrite\n");

    DiskDriver_readBlock(mem, out, 0);
    printf("block 0: \n");
    for(int i = 0; i < BLOCK_SIZE; i++)
        printf("%c", out[i]);
    printf("\n");

    printf("free 5-th block\n");
    ret = DiskDriver_freeBlock(mem, 5);
    if(ret == -1) printf("block not freed 5\n");
    DiskDriver_flush(mem);
    printf("first free block: %d\n", mem->header->first_free_block);
    printf("free blocks: %d\n", mem->header->free_blocks);
    
    printf("\n");

    printf("free 0-th block\n");
    ret = DiskDriver_freeBlock(mem, 0);
    if(ret == -1) printf("error in freed block 0\n");
    DiskDriver_flush(mem);
    printf("free blocks: %d\n", mem->header->free_blocks);

    ret = DiskDriver_getFreeBlock(mem, 0);
    error(ret, "error in getfreeBlock\n");
    printf("getfreeBlock ret --> %d \n",ret);

    print_bitmap_bit(mem->bitmap_data, mem->header->bitmap_entries);

    free(out);
    free(mem);
    free(file);
    return 0;
}