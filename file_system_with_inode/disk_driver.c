#include "disk_driver.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

void error(int ret, char* msg){
    if(ret == -1){
      perror(msg);
      return ;
    }
}
// opens the file (creating it if necessary)
// allocates the necessary space on the disk
// calculates how big the bitmap should be
// if the file was new
// compiles a disk header, and fills in the bitmap of appropriate size
// with all 0 (to denote the free space);

void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){

    if(disk == NULL || filename == NULL || num_blocks < 1){
        printf("Bad Parameter in Input in DiskDriver_init\n");
        return;
    } 

    int bmap_size = num_blocks / BLOCK_SIZE_BITMAP;     
    if((num_blocks % BLOCK_SIZE_BITMAP) != 0) bmap_size++;

    int fd = open(filename, O_RDWR , 0666);
    
    int size_mmap = sizeof(DiskHeader) + bmap_size + BLOCK_SIZE * num_blocks;

    if(DEBUG) printf("INFO FileSystem /%s/ of size %d\n", filename, size_mmap);

    if(fd == -1){
        if(errno == ENOENT){        //ENOENT--> no such file or directory
        
            if(DEBUG) printf("filesystem file NOT exists, create it, mmap e data initialization\n");
            
            fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
            error(fd, "error open_file in DiskDriver_init\n");

            if(posix_fallocate(fd, 0, size_mmap) != 0){
                close(fd);											                            //new file, 0
                error(-1, "error in posix_fallocate in DiskDriver_init\n");						//fallocate allocates necessary space to mmap  
            }												

            DiskHeader* disk_memory = (DiskHeader*) mmap(NULL, size_mmap, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
                
            if(disk_memory == MAP_FAILED){
                close(fd);
                error(-1, "Error mmap 1\n");
            }
            
            if(DEBUG) printf("MMAP\n data structer initialization\n");
            
            disk->header =  disk_memory;
            disk->bitmap_data = (char*)disk_memory + sizeof(DiskHeader);         //pointer disk_memory(mmap) e offset size disk header
            disk->fd = fd;

            disk_memory->num_blocks = num_blocks;  
            disk_memory->bitmap_blocks = num_blocks;
            disk_memory->bitmap_entries = bmap_size;
            disk_memory->free_blocks = num_blocks;
            disk_memory->first_free_block = 0;

            if(DEBUG) printf("zero bitmap initialization\n");
            memset(disk->bitmap_data, 0, bmap_size);
        
        }else{
            error(fd, "error open_file in DiskDriver_init\n");
        }
    }
    else{
        if(DEBUG) printf("filesystem file exists, mmap and update DiskHeader\n");
        
        if(posix_fallocate(fd, 0, size_mmap) != 0){
                close(fd);											            //File è nuovo, 0,
                error(-1, "error in posix_fallocate in DiskDriver_init\n");										//fallocate alloca lo spazio necessario alla mmap  
        }
        
        DiskHeader* disk_memory = (DiskHeader*) mmap(NULL, size_mmap, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        
        if(disk_memory == MAP_FAILED){
            close(fd);
            error(-1, "Error mmap 2\n");
        }

        if(DEBUG) printf(" disk_memory pointer %p\n", disk_memory);

        if(DEBUG) printf("MMAP\n data header initialization\n");
        
        disk->header =  disk_memory;
        disk->bitmap_data = (char*)disk_memory + sizeof(DiskHeader); 
        disk->fd = fd;
    }

    if(DEBUG) printf("memory contains %d blocks\nbitmap has %d blocks and %d bytes\nfirst free block is %d, total free blocks are %d\n", 
                            disk->header->num_blocks, disk->header->bitmap_blocks, 
                            disk->header->bitmap_entries, disk->header->first_free_block, disk->header->free_blocks);

}

// reads the block in position block_num
// returns -1 if the block is free according to the bitmap
// 0 otherwise
int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){  
                                                                        
    if(disk == NULL || dest == NULL || block_num < 0){
        printf("Bad parameters in Input in DiskDriver_readBlock\n");
        return -1;
    }

    if(block_num < disk->header->num_blocks){

            BitMapEntryKey e = BitMap_blockToIndex(block_num);

            int mask = 1 << e.bit_num;                              //create mask
            int masked_n = disk->bitmap_data[e.entry_num] & mask;   //apply mask
            int bit = masked_n >> e.bit_num;                        //now bit represent status of block in memory
            
            if(DEBUG) printf("bit in pos %d is %d \n", block_num, bit);

            if(bit == 0){
                printf("try to read an empty block\n");
                return -1;
            }
            
            off_t seek = lseek(disk->fd, (sizeof(DiskHeader)+disk->header->bitmap_entries+block_num*BLOCK_SIZE), 
                                SEEK_SET);
            error(seek, "error in lseek - DiskDriver_readBlock\n");

            if(DEBUG) printf("read block %d, seek %d\n", block_num, (int)seek); 
            
            int read_byte = 0, ret;
            while(read_byte < BLOCK_SIZE){
                ret = read(disk->fd, dest + read_byte, BLOCK_SIZE - read_byte);
                if(ret == -1 && errno == EINTR) continue;
                else if(ret == -1) error(ret, "error read - DiskDriver_readBlock\n");
                read_byte += ret;
            }
                        
            if(DEBUG) printf("read %d byte\n", read_byte);
            
            return 0;
    }
    else{
        printf("error block_num too high - DiskDriver_readBlock\n");
        return 0;
    }
}

// writes a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
                                                                            
    if(disk == NULL || src == NULL || block_num < 0){
        printf("Bad parameters in Input in - DiskDriver_writeBlock\n");
        return-1;
    }

    if(block_num >= disk->header->num_blocks){
        printf("error block_num too high - DiskDriver_writeBlock\n");
        return -1;
    }

    if(disk->header->free_blocks <= 0){
        printf("FULL MEMORY - I can't write new block w\n");
        return -1;
    }
  
    BitMap bmap;
    bmap.num_bits = disk->header->num_blocks;
    bmap.entries = disk->bitmap_data;

    if(BitMap_get(&bmap, block_num, 0) != block_num){                   //not overwrite
        printf("NOT - CHANGED, chosen block is not free\n");
        return -2;
    }

    off_t seek = lseek(disk->fd, (sizeof(DiskHeader)+disk->header->bitmap_entries+block_num*BLOCK_SIZE), 
                                SEEK_SET);

    if(DEBUG) printf("write block %d, seek %d\n", block_num, (int)seek);

    //before write in file, after set parameters
    int ret, write_byte = 0;
    while(write_byte < BLOCK_SIZE){
        ret = write(disk->fd, src + write_byte, BLOCK_SIZE - write_byte);
        if(ret == -1 && errno == EINTR) continue;
        else if (ret == -1) error(ret, "error write - DiskDriver_writeBlock\n");
        write_byte += ret;
    }
    
    //check bmap
    BitMap_set(&bmap, block_num, 1);
    disk->header->free_blocks--;

    if(block_num == disk->header->first_free_block)
        disk->header->first_free_block = DiskDriver_getFreeBlock(disk, block_num); //BitMap_get(&bmap, block_num, 0);
    return 0;
}

// frees a block in position block_num, and alters the bitmap accordingly
// returns -1 if operation not possible
int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
    
    if(disk == NULL){
        printf("Bad Parameter in Input - DiskDriver_freeBlock\n");
        return-1;
    }

    if(block_num > disk->header->bitmap_blocks){
        printf("error block_num too high - DiskDriver_freeBlock\n");
        return -1;
    }

    BitMap bmap;
    bmap.num_bits = disk->header->num_blocks;
    bmap.entries = disk->bitmap_data;
    

    if(BitMap_get(&bmap, block_num, 0) == block_num ) return 0; //it's already free
    else if(BitMap_set(&bmap, block_num, 0) == -1) return -1;

    if (DEBUG) printf("freed block %d", block_num);
    memset(disk->bitmap_data + disk->header->bitmap_entries + block_num*BLOCK_SIZE, 0, BLOCK_SIZE);
    disk->header->free_blocks++;

    if(block_num < disk->header->first_free_block || disk->header->first_free_block == -1) // if bitmap doesn't find a free block it puts in first free block -1 
        disk->header->first_free_block = block_num;
    
    return 0;
}

// returns the first free block in the disk from position (checking the bitmap)
int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
    
    if(disk == NULL){
        printf("error disk DiskDriver_getfreeBlock\n");
        return -1;
    }
    if(disk->header->free_blocks <= 0){
        printf("FULL MEMORY, I can't create new block\n");
        return -1;
    }
    if(start > disk->header->bitmap_blocks){
        printf("error block_num too high - DiskDriver_getfreeBlock\n");
        return -1;
    }

    BitMap bmap;
    bmap.num_bits = disk->header->bitmap_blocks;
    bmap.entries = disk->bitmap_data;

    int free_block = BitMap_get(&bmap, start, 0);

    if(DEBUG) printf("first free block is %d\n", free_block);

    error(free_block, "error in DiskDriver_getFreeBlock\n");

    return free_block;
}

// writes the data (flushing the mmaps)
int DiskDriver_flush(DiskDriver* disk){     
    
    if(disk == NULL || disk->header == NULL ){
        printf("PROBLEMA CON IL DISCO HEADER");
        return -1;
    }
	int ret = msync(disk->header, sizeof(DiskHeader)+disk->header->bitmap_entries + disk->header->num_blocks*BLOCK_SIZE, MS_ASYNC); //Flush header and bitmap on file 
    error(ret, "error msync - DiskDriver_flush\n");
	
    if(DEBUG) printf("flush dim %ld\n", sizeof(DiskHeader)+disk->header->bitmap_entries+disk->header->num_blocks*BLOCK_SIZE);
    
    return 0;
}


int DiskDriver_rewriteBlock(DiskDriver* disk, void* src, int block_num){
                                                                            
    if(disk == NULL){
        printf("Bad Parameter in Input - DiskDriver_rewriteBlock\n");
        return-1;
    }

    if(block_num > disk->header->bitmap_blocks){
        printf("error block_num too high - DiskDriver_rewriteBlock\n");
        return -1;
    }
  
    BitMap bmap;
    bmap.num_bits = disk->header->num_blocks;
    bmap.entries = disk->bitmap_data;

    if(BitMap_get(&bmap, block_num, 1) != block_num){                           //this block is free, there is not already write 
        printf("choose block it is already write\n");
        return -1;
    }

    off_t seek = lseek(disk->fd, (sizeof(DiskHeader)+disk->header->bitmap_entries+block_num*BLOCK_SIZE), 
                                SEEK_SET);

    if(DEBUG) printf("overwrite block %d, seek %d\n", block_num, (int)seek);

    int ret, write_byte = 0;
    while(write_byte < BLOCK_SIZE){
        ret = write(disk->fd, src + write_byte, BLOCK_SIZE - write_byte);
        if(ret == -1 && errno == EINTR) continue;
        else if (ret == -1 )error(ret, "error write - DiskDriver_rewriteBlock\n");
        write_byte += ret;
    }
    
    return 0;
}