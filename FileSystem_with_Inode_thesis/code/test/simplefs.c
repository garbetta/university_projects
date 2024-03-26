#include "simplefs.h"
#include "aux_fun.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk){
    
    if(fs == NULL || disk == NULL){
        printf("Bad Parameters in Input - SimpleFS_init\n");
        return NULL;
    } 
    fs->disk = disk;
    int ret;
    FirstDirectoryBlock* fdb = (FirstDirectoryBlock*)calloc(1, sizeof(FirstDirectoryBlock));
    if(disk->header->first_free_block == 0){ //disk is empty
        SimpleFS_format(fs);
    }
    ret = DiskDriver_readBlock(disk, fdb, 0);
    error(ret, "error in DiskDriver_readBlock in SimpleFS_init\n");
    DiskDriver_flush(disk);
    
    if(DEBUG) printf("allocate and set DirectoryHandle\n");
    DirectoryHandle* dir = (DirectoryHandle*)calloc(1, sizeof(DirectoryHandle));
    dir->sfs = fs;
    dir->dcb = fdb;
    dir->directory = NULL;                  //is top level dir
    dir->pos_in_dir = 0;
    dir->pos_in_block = 0;

    return dir;
}

// creates the initial structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs){
    
    printf("\nStart formatting File System \n");
    if(fs == NULL || fs->disk == NULL){
        printf(" FS not exist, bad parameters, error SimpleFS_format \n");  
        return;
    } 

    if(fs->disk->header == NULL){
        printf(" Bad parameter DiskHeader, error SimpleFS_format 1\n");  
        return;
    }

    if(fs->disk->bitmap_data == NULL){
        printf(" Bad parameter Bitmap, error SimpleFS_format 2\n");  
        return;
    }

    int ret = 0;

    memset(fs->disk->bitmap_data, 0, fs->disk->header->bitmap_entries);
    //initialization of first struct
    fs->disk->header->first_free_block = 0;
    fs->disk->header->free_blocks = fs->disk->header->num_blocks;
    FirstDirectoryBlock fdb = {0}; 
    fdb.header.block_in_disk = 0;
    fdb.header.is_data_block = 0;
    fdb.fcb.directory_block = -1;
    fdb.fcb.block_in_disk = 0;
    strcpy(fdb.fcb.name, "/");
    fdb.fcb.size_in_bytes = 0;
    fdb.fcb.size_in_blocks = 1;
    fdb.fcb.is_dir = 1;
    fdb.num_entries = 0;

    for(int i = 0; i < DIRECT_BLOCKS_NUM; i++) fdb.direct_blocks[i] = -1;
    for(int i = 0; i < INDIRECT_BLOCKS_NUM; i++) fdb.indirect_blocks[i] = -1;
    memset(fdb.null, 0, BLOCK_SIZE - sizeof(BlockHeader) - sizeof(FileControlBlock) - 4 - 4*DIRECT_BLOCKS_NUM - 4*INDIRECT_BLOCKS_NUM); 

    ret = DiskDriver_writeBlock(fs->disk, &fdb, 0);
    error(ret, "error in DiskDriver_writeBlock in SimpleFS_format\n");

    DiskDriver_flush(fs->disk);
    printf("File System formatted correctly\n");

}

// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename){

    if(d == NULL || filename == NULL || strlen(filename) >= 128 || (d->dcb->fcb.is_dir == 0) ){
        printf("Bad Parameter in input - SimpleFS_createFile\n");
        return NULL;
    }
    if(d->sfs->disk->header->free_blocks <= 0){
        printf(CR"FULL MEMORY, can't create new file"RES"\n");
        return NULL;
    }

    int ret = 0, file_block, single_block, double_block, triple_block, first_free_block, entries = 0, i = 0, k = 0, z = 0 ;
    FirstFileBlock* new_file = (FirstFileBlock*)calloc(1, sizeof(FirstFileBlock));
    FileHandle* file_h;
    ///// CONTROL DIRECT BLOCK
    file_block = d->dcb->direct_blocks[i];
    ret = control_direct_block(NULL, d, filename, file_block, i, entries, 0, 0);
    if(ret < 0){
        printf(CR"File with this name already exists"RES"\n");
        free(new_file);
        return NULL;
    }      
    if(ret > 0){
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, ret);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_createFile 1\n");
        file_h = f_handle(d, new_file);
        return file_h;
    }
    ////////

    ////// CONTROL SINGLE_BLOCK
    single_block = d->dcb->indirect_blocks[0];
    if(single_block == -1){
        free(new_file);
        return NULL;
    }
    i = 0;
    ret = control_single_block(NULL, d, filename, single_block, i, entries, 0, 0);
    if(ret < 0){
        printf(CR"File with this name already exists"RES"\n");
        free(new_file);
        return NULL;
    } 
    if(ret > 0){
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, ret);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_createFile 2\n");
        file_h = f_handle(d, new_file);
        return file_h;
    }
    if(ret == 0 && d->dcb->indirect_blocks[1] == -1 ){
        first_free_block = d->sfs->disk->header->first_free_block; 
        IndexBlock index_block2 = {0};  
        ret = new_index_block(d, &index_block2, first_free_block);
        if(ret == -1){
            free(new_file);
            return NULL;
        } 
        //printf("\nINDEX DOUBLE BLOCK %d\n", first_free_block);
        d->dcb->indirect_blocks[1] = first_free_block;
        ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
        error(ret, "error DiskDriver_rewriteBlock in SimpleFS_createFile 1\n");
    }
    ////////////////
    
    /////// CONTROL DOUBLE_BLOCK
    double_block = d->dcb->indirect_blocks[1];
    if(double_block == -1){
        free(new_file);
        return NULL;
    }       
    //printf("\nDOUBLE BLOCK %d\n", double_block);
    i = 0, k = 0;
    ret = control_double_block(NULL, d, filename, double_block, i, k, entries, 0, 0);    
    if(ret < 0){
        printf(CR"File with this name already exists"RES"\n");
        free(new_file);
        return NULL;
    } 
    if(ret > 0){
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, ret);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_createFile 3\n");
        file_h = f_handle(d, new_file);
        return file_h;
    }
    if(ret == 0 && d->dcb->indirect_blocks[2] == -1){       
        first_free_block = d->sfs->disk->header->first_free_block;     
        IndexBlock index_block3 = {0};
        ret = new_index_block(d, &index_block3, first_free_block);
        if(ret == -1){
            free(new_file);
            return NULL;
        } 
        //printf("\nINDEX DOUBLE BLOCK %d\n", first_free_block);
        d->dcb->indirect_blocks[2] = first_free_block;
        ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
        error(ret, "error DiskDriver_rewriteBlock in SimpleFS_createFile 2\n");        
    }
    //////////////////////
   
    ///////// CONTROL TRIPLE_BLOCK
    triple_block = d->dcb->indirect_blocks[2];
    if(triple_block == -1){
        free(new_file);
        return NULL;
    }
    i = 0, k = 0, z = 0;
    ret = control_triple_block(NULL, d, filename, triple_block, i, k, z, entries, 0, 0);
    if(ret < 0){
        printf(CR"File with this name already exists"RES"\n");
        free(new_file);
        return NULL;
    } 
    if(ret > 0){
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, ret);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_createFile 4\n");
        file_h = f_handle(d, new_file);
        return file_h;
    }
    //////////
    free(new_file);
    return NULL;
}

// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d){
    
    if(names == NULL || d == NULL){
        printf("Bad Parameter in input - SimpleFS_readDir\n");
        return -1;
    }
    if(d->dcb->num_entries == 0){
        printf(CR"Empty directory"RES"\n");
        return 0;
    }

    int file_block, single_block, double_block, triple_block, entries = 0, i = 0, k = 0, z = 0 ;

    ///// CONTROL DIRECT BLOCK
    file_block = d->dcb->direct_blocks[i];
    entries = control_direct_block(names, d, NULL, file_block, i, entries, 0, 0);
    if(entries < 0) return d->dcb->num_entries;      //if ret < 0 stop, there is a block -1
    ////////

    ////// CONTROL SINGLE_BLOCK
    single_block = d->dcb->indirect_blocks[0];
    if(single_block == -1) return d->dcb->num_entries;
    i = 0;
    entries = control_single_block(names, d, NULL, single_block, i, entries, 0, 0);
    if(entries < 0) return d->dcb->num_entries; 
    ////////////////
    
    /////// CONTROL DOUBLE_BLOCK
    double_block = d->dcb->indirect_blocks[1];
    if(double_block == -1) return d->dcb->num_entries;       
    i = 0, k = 0;
    entries = control_double_block(names, d, NULL, double_block, i, k, entries, 0, 0);
    if(entries < 0) return d->dcb->num_entries;
    //////////////////////
   
    ///////// CONTROL TRIPLE_BLOCK
    triple_block = d->dcb->indirect_blocks[2];
    if(triple_block == -1) return d->dcb->num_entries;
    i = 0, k = 0;
    entries = control_triple_block(names, d, NULL, triple_block, i, k, z, entries, 0, 0);
    //////////
    return d->dcb->num_entries;
}

// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){
    
    if(d == NULL || filename == NULL || strlen(filename) >= 128 || (d->dcb->fcb.is_dir == 0)){
        printf("Bad Parameter in input - SimpleFS_openFile\n");
        return NULL;
    }

    int ret = 0, file_block, single_block, double_block, triple_block, entries = 0, i = 0, k = 0, z = 0, pos_in_disk, first_free_block;
    FirstFileBlock* new_file = (FirstFileBlock*)calloc(1, sizeof(FirstFileBlock));
    FileHandle* file_h;
    ///// CONTROL DIRECT BLOCK
    file_block = d->dcb->direct_blocks[i];
    ret = control_direct_block(NULL, d, filename, file_block, i, entries, 0, 0);
    if(ret < 0){
        pos_in_disk = abs(ret);
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, pos_in_disk);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_openFile 1\n");
        if(new_file->fcb.is_dir == 1){
            printf(CR"There is a directory with this name "RES"\n");
            free(new_file);
            return NULL;
        } 
        file_h = f_handle(d, new_file);
        return file_h;
    }       
    if(ret > 0){
        printf(CY"There is not file, create and open it"RES"\n");
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, ret);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_openFile 2\n");
        file_h = f_handle(d, new_file);
        return file_h;
    }
    ////////
    ////// CONTROL SINGLE_BLOCK
    single_block = d->dcb->indirect_blocks[0];
    if(single_block == -1){
        printf(CR"File with this name already exists"RES"\n");
        free(new_file);
        return NULL;
    }
    i = 0;
    ret = control_single_block(NULL, d, filename, single_block, i, entries, 0, 0);
    if(ret < 0){
        pos_in_disk = abs(ret);
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, pos_in_disk);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_openFile 3\n");
        if(new_file->fcb.is_dir == 1){
            printf(CR"There is a directory with this name "RES"\n");
            free(new_file);
            return NULL;
        } 
        file_h = f_handle(d, new_file);
        return file_h;
    }  
    if(ret > 0){
        printf(CY"There is not file, create and open it"RES"\n");
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, ret);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_openFile 4\n");
        file_h = f_handle(d, new_file);
        return file_h;
    }
    if(ret == 0 && d->dcb->indirect_blocks[1] == -1 ){
        first_free_block = d->sfs->disk->header->first_free_block;
        IndexBlock index_block2 = {0};
        ret = new_index_block(d, &index_block2, first_free_block);
        if(ret == -1) {
            free(new_file);
            return NULL;
        }
        d->dcb->indirect_blocks[1] = first_free_block;
        ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
        error(ret, "error DiskDriver_rewriteBlock in SimpleFS_openFile 1\n");
    }
    ////////////////
    /////// CONTROL DOUBLE_BLOCK
    double_block = d->dcb->indirect_blocks[1];
    if(double_block == -1){
        printf(CR"File with this name already exists"RES"\n");
        free(new_file);
        return NULL;
    }
    //printf("\n DOUBLE BLOCK %d\n", double_block);
    i = 0, k = 0;
    ret = control_double_block(NULL, d, filename, double_block, i, k, entries, 0, 0); 
    if(ret < 0){
        pos_in_disk = abs(ret);
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, pos_in_disk);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_openFile 5\n");
        if(new_file->fcb.is_dir == 1){
            printf(CR"There is a directory with this name "RES"\n");
            free(new_file);
            return NULL;
        } 
        file_h = f_handle(d, new_file);
        return file_h;
    }  
    if(ret > 0){
        printf(CY"There is not file, create and open it"RES"\n");
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, ret);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_openFile 6\n");
        file_h = f_handle(d, new_file);
        return file_h;
    }

    if(ret == 0 && d->dcb->indirect_blocks[2] == -1){      
        ///////////////////
        first_free_block = d->sfs->disk->header->first_free_block;  
        IndexBlock index_block3 =  {0};
        ret = new_index_block(d, &index_block3, first_free_block);
        if(ret == -1){
            free(new_file);
            return NULL;
        } 
        d->dcb->indirect_blocks[2] = first_free_block;
        ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
        error(ret, "error DiskDriver_rewriteBlock in SimpleFS_openFile 2\n");
    }
    //////////////////////
    ///////// CONTROL TRIPLE_BLOCK
    triple_block = d->dcb->indirect_blocks[2];
    if(triple_block == -1){
        printf(CR"File with this name already exists"RES"\n");
        free(new_file);
        return NULL;
    }
    i = 0, k = 0, z = 0;
    ret = control_triple_block(NULL, d, filename, triple_block, i, k, z, entries, 0, 0);
    if(ret < 0){
        pos_in_disk = abs(ret);
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, pos_in_disk);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_openFile 7\n");
        if(new_file->fcb.is_dir == 1){
            printf(CR"There is a directory with this name "RES"\n");
            free(new_file);
            return NULL;
        } 
        file_h = f_handle(d, new_file);
        return file_h;
    }
    if(ret > 0){
        printf(CY"There is not file, create and open it"RES"\n");
        ret = DiskDriver_readBlock(d->sfs->disk, new_file, ret);
        error(ret, "error in DiskDriver_redBlock in SimpleFS_openFile 8\n");
        file_h = f_handle(d, new_file);
        return file_h;
    }
    //////////
    free(new_file);
    return NULL;
}

// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f){
    if(f == NULL){
        printf("File handle is already destroyed\n");
        return -1;
    }
    free(f->fcb);
    free(f);
    return 0;
}

// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){
    
    if(f == NULL || data == NULL || size < 0){
        printf("Bad Parameters in Input - SimpleFS_write\n");
        return -1;
    }
    if(size + f->pos_in_file  > (DIRECT_BLOCKS_NUM+(INDEXES)+(INDEXES*INDEXES)+(INDEXES*INDEXES*INDEXES))*BLOCK_SIZE){
        size = (DIRECT_BLOCKS_NUM+(INDEXES)+(INDEXES*INDEXES)+(INDEXES*INDEXES*INDEXES))*BLOCK_SIZE - f->pos_in_file;
        printf(CM"text size is too large, write only firsts %d bytes"RES"\n", size);
        printf(CR"BLOCK INDEXES FILE FULL"RES"\n");
    }
    //printf(CY"size %d, pos in file %d"RES"\n", size, f->pos_in_file);
    int ret, written = 0, free_space, to_write = 0, offset_p, offset_b, num_block, last;
    offset_b = (f->fcb->fcb.size_in_bytes - DATA_FIRST) % DATA;
    FileBlock* ff_block = (FileBlock*)calloc(1, sizeof(FileBlock));
    FirstFileBlock* ffb = f->fcb;
    if(DEBUG)printf("size in bytes: %d\n", f->fcb->fcb.size_in_bytes);
    if(f->fcb->fcb.size_in_bytes != 0 && (f->pos_in_file < f->fcb->fcb.size_in_bytes)){
        //overwritting
        if(f->pos_in_file < DATA_FIRST){    //write in FirstFileBlock
            free_space = DATA_FIRST - f->pos_in_file;
            if(size < free_space) to_write = size;
            else to_write = free_space;
            strncpy(ffb->data + f->pos_in_file, data, to_write);
            written += to_write;
            size -= to_write;
            ret = DiskDriver_rewriteBlock(f->sfs->disk, ffb, f->fcb->fcb.block_in_disk);
            error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_write 1\n");
            written = str_to_wrt(f, data, to_write, written, 0, size);
        }
        else{   //write in FileBlock
            offset_p = (f->pos_in_file - DATA_FIRST) % DATA;
            //printf(CY"\noffset pos in file: %d" RES"\n  \n", offset_p);
            num_block = f->pos_in_file - DATA_FIRST;
            if(num_block > DATA_FIRST) num_block = (num_block / DATA) + 1;
            else num_block = 1; 
            last = first_free_index(f, -1, num_block, 1);
            if(last == -1) {
                free(ff_block);
                return written;
            }
            //index last block to continue writing
            //printf(" index last block to continue: %d\n", last);
            ret = DiskDriver_readBlock(f->sfs->disk, ff_block, last);
            error(ret, "error in DiskDriver_readBlock in SimpleFS_write 1\n");
            free_space = DATA - offset_p;
            if(size < free_space) to_write = size;
            else to_write = free_space;
            //printf("size %d  e to_write %d\n", size, to_write);
            strncpy(ff_block->data + offset_p, data, to_write); 
            written += to_write;
            free_space -= to_write;
            size -= to_write;
            ret = DiskDriver_rewriteBlock(f->sfs->disk, ff_block, last);
            error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_write 2\n");
            ret = DiskDriver_rewriteBlock(f->sfs->disk, ffb, f->fcb->fcb.block_in_disk);
            error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_write 3\n");
            written = str_to_wrt(f, data, to_write, written, num_block, size);
        }
        //printf("BYTES WRITTEN SONO : %d", written);
        if(f->pos_in_file + written > f->fcb->fcb.size_in_bytes)
            f->fcb->fcb.size_in_bytes = (written + f->pos_in_file);
        free(ff_block);

        f->directory->fcb.size_in_bytes = (f->directory->fcb.size_in_bytes) + written;
        ret = DiskDriver_rewriteBlock(f->sfs->disk, f->directory, f->directory->fcb.block_in_disk);
        error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_write 4\n");
        return written;
    }
    //writing a new empty file o in append
    if((f->pos_in_file == 0 && f->fcb->fcb.size_in_blocks == 1) || f->pos_in_file == f->fcb->fcb.size_in_bytes){ 
        //if there is only first file block
        if(f->fcb->fcb.size_in_blocks == 1){
            free_space = DATA_FIRST - f->pos_in_file;
            if(size < free_space) to_write = size;
            else to_write = free_space;
            strncpy(ffb->data + f->pos_in_file, data, to_write);
            ffb->fcb.size_in_bytes += to_write; 
            //printf("written bytes %d\n", ffb->fcb.size_in_bytes);
            written += to_write;
            free_space -= to_write;
            size -= to_write;
            ret = DiskDriver_rewriteBlock(f->sfs->disk, ffb, f->fcb->fcb.block_in_disk);
            error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_write 5\n");
            written = str_to_wrt_app(f, data, to_write, written, size);
        }
        else{   //file is not empty
            FileBlock f_block;
            last = first_free_index(f, -1, (f->fcb->fcb.size_in_blocks)-1, 1);
            if(last == -1)  {
                free(ff_block);
                return written;
            }
            //index last block to continue writing
            //printf("index last block %d\n", last);
            ret = DiskDriver_readBlock(f->sfs->disk, &f_block, last);
            error(ret, "error in DiskDriver_readBlock in SimpleFS_write 2\n");
            offset_b = (f->fcb->fcb.size_in_bytes - DATA_FIRST) % DATA;
            free_space = DATA - offset_b;
            if(size < free_space) to_write = size;
            else to_write = free_space;
            //printf("size %d\n", size);
            strncpy(f_block.data + offset_b, data, to_write);
            ffb->fcb.size_in_bytes += to_write; 
            written += to_write;
            free_space -= to_write;
            size -= to_write;
            ret = DiskDriver_rewriteBlock(f->sfs->disk, &f_block, last);
            error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_write 6\n");
            ret = DiskDriver_rewriteBlock(f->sfs->disk, ffb, f->fcb->fcb.block_in_disk);
            error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_write 7\n");
            //printf("\nSIZE %d\n", size);
            written = str_to_wrt_app(f, data, to_write, written, size);
        }
    }
    //printf("TOTAL SIZE IN BYTES : %d\n", f->fcb->fcb.size_in_bytes);
    ret = DiskDriver_rewriteBlock(f->sfs->disk, f->fcb, f->fcb->fcb.block_in_disk);
    error(ret, "error in DiskDriver_rewrite in Simple_write 8\n");
    free(ff_block);
    f->directory->fcb.size_in_bytes = (f->directory->fcb.size_in_bytes) + written;
    ret = DiskDriver_rewriteBlock(f->sfs->disk, f->directory, f->directory->fcb.block_in_disk);
    error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_write 9\n");
    return written;
}

// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size){
    if(f == NULL || data == NULL || size < 0){
        printf("Bad Parameters in Input - SimpleFS_read\n");
        return -1;
    }
    if(f->pos_in_file + size > f->fcb->fcb.size_in_bytes) size = f->fcb->fcb.size_in_bytes - f->pos_in_file;
    FirstFileBlock* ffb = f->fcb;
    int block, ret = 0, read_bytes = 0, num_block = -1, to_read, read_space;
    if(f->pos_in_file < DATA_FIRST){   //read in FirstFileBlock
        read_space = DATA_FIRST - f->pos_in_file;
        if(size <= read_space) to_read = size;
        else to_read = read_space;
        strncpy(data, ffb->data + f->pos_in_file, to_read);
        read_bytes += to_read;
        size -= to_read;
        num_block = 0;
    }
    if(f->pos_in_file > DATA_FIRST){    //read in FileBlock
        int offset = (f->pos_in_file - DATA_FIRST) % DATA;
        num_block = f->pos_in_file - DATA_FIRST;
        if(num_block > DATA_FIRST) num_block = (num_block / DATA) + 1;
        else num_block = 1;
        block = first_free_index(f, -1, num_block, 1);
        if(block == -2) return read_bytes;
        if(block == -1) return read_bytes;
        if(size < (DATA - offset)) to_read = size;
        else to_read = DATA - offset;
        read_bytes = str_to_read(f, data, to_read, read_bytes, block, offset);
        size -= to_read;
    }
    int count = size / DATA;
    int j = 0;
    for(; j < count; j++){  //read full FileBlock
        block = first_free_index(f, -1, num_block+j+1, 1);
        if(block == -2) return read_bytes;
        if(block != -1){
            read_bytes = str_to_read(f, data, DATA, read_bytes, block, 0);
            size -= DATA;
        }
    }

    int rem = size % DATA;
    //printf(CY"rem %d"RES"\n", rem);
    if(rem != 0){       //read last FileBlock
        count++;
        block = first_free_index(f, -1, num_block+count, 1);
        if(block == -2) return read_bytes;
        if(block != -1){
            read_bytes = str_to_read(f, data, rem, read_bytes, block, 0);
            size -= rem; 
        }
    } 
    //printf(" \nTOTAL READ BYTES %d \n ", read_bytes);
    int seek;
    seek = read_bytes + f->pos_in_file;
    ret = SimpleFS_seek(f, seek);
    error(ret, "error in SimpleFS_seek in SimpleFS_read\n");
    
    return read_bytes;
}

// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos){
    
    if(f == NULL){
        printf("Bad Parameters in Input - SimpleFS_seek\n");
        return -1;
    }

    if(pos > f->fcb->fcb.size_in_bytes){
        printf(CR"POS too high, choose valid pos"RES"\n");
        return -1;
    }
    f->pos_in_file = pos;
    return pos;
}

// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
int SimpleFS_changeDir(DirectoryHandle* d, char* dirname){  //d is direcotry where we are
    
    if(d == NULL || dirname == NULL){
        printf("Bad Parameters in Input - SimpleFS_changeDir\n");
        return -1;
    }

    if((strcmp(dirname, "..") == 0)){       //to parent direcotry
        if(d->directory == NULL){
            printf(CR"Impossible to read parent directory, you are in root"RES"\n");
            return -1;
        }
        free(d->dcb);
        int block_parent = d->directory->fcb.directory_block;
        if(block_parent == -1){                  // d parent is root, root has not parent
            d->dcb = d->directory;
            d->directory = NULL;  
            d->pos_in_block = 0;
            //d->pos_in_dir;
            //d->current_block = &(d->dcb->header);
            return 0;  
        }
        int ret;
        FirstDirectoryBlock* parent = (FirstDirectoryBlock*)calloc(1, sizeof(FirstDirectoryBlock));
        ret = DiskDriver_readBlock(d->sfs->disk, parent, block_parent);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_changeDir 1\n");
        d->dcb = d->directory;
        d->pos_in_block = 0;
        //d->pos_in_dir;
        //d->current_block = &(d->dcb->header);
        d->directory = parent;
        return 0;
    }else if(d->dcb->num_entries == 0 ){
        printf(CR"This directory is empty, CAN'T change directory"RES"\n");
        return -1;
    }
    //HERE, I don't want to go in parent directory,
    //directory is not empty, I go to in subfolder, 
    //I need to check that the dirname is in d 
    int ret = 0, file_block, single_block, double_block, triple_block, entries = 0, i = 0, k = 0, z = 0, pos_in_disk;
    FirstDirectoryBlock* old_dir = (FirstDirectoryBlock*)calloc(1, sizeof(FirstDirectoryBlock));
    ///// CONTROL DIRECT BLOCK
    file_block = d->dcb->direct_blocks[i];
    ret = control_direct_block(NULL, d, dirname, file_block, i, entries, 1, 1);
    if(ret < 0){
        pos_in_disk = abs(ret);
        ret = DiskDriver_readBlock(d->sfs->disk, old_dir, pos_in_disk);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_changeDir 2\n");
        if(old_dir->fcb.is_dir == 0){
            printf(CR"Can't changedir %s is a file"RES"\n", old_dir->fcb.name);
            free(old_dir);
            return -1;
        }
        d->directory = d->dcb;
        d->dcb = old_dir;      
        //check other fields
        //d->pos_in_block = old_dir->header.block_in_block;
        return 0;
    }       
    if(ret > 0){
        printf(CR"The directory %s there is not in %s"RES"\n", dirname, d->dcb->fcb.name);
        free(old_dir);
        return -1;
    }
    ////////
    ////// CONTROL SINGLE_BLOCK
    single_block = d->dcb->indirect_blocks[0];
    if(single_block == -1){
        printf(CR"The directory %s there is not in %s"RES"\n", dirname, d->dcb->fcb.name);
        free(old_dir);
        return -1;
    } 
    i = 0;
    ret = control_single_block(NULL, d, dirname, single_block, i, entries, 1, 1);
    if(ret < 0){
        pos_in_disk = abs(ret);
        ret = DiskDriver_readBlock(d->sfs->disk, old_dir, pos_in_disk);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_changeDir 3\n");
        if(old_dir->fcb.is_dir == 0){
            printf(CR"Can't changedir %s is a file"RES"\n", old_dir->fcb.name);
            free(old_dir);
            return -1;
        }
        d->directory = d->dcb;
        d->dcb = old_dir;   
        //d->pos_in_block = old_dir->header.pos_in_block;   
        return 0;
    }  
    if(ret > 0){
        printf(CR"The directory %s there is not in %s"RES"\n", dirname, d->dcb->fcb.name);
        free(old_dir);
        return -1;
    }
    ////////////////   
    /////// CONTROL DOUBLE_BLOCK
    double_block = d->dcb->indirect_blocks[1];
    if(double_block == -1){
        printf(CR"The directory %s there is not in %s"RES"\n", dirname, d->dcb->fcb.name);
        free(old_dir);
        return -1;
    }      
    i = 0, k = 0;
    ret = control_double_block(NULL, d, dirname, double_block, i, k, entries, 1, 1);   
    if(ret < 0){
        pos_in_disk = abs(ret);
        ret = DiskDriver_readBlock(d->sfs->disk, old_dir, pos_in_disk);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_changeDir 4\n");
        if(old_dir->fcb.is_dir == 0){
            printf(CR"Can't changedir %s is a file"RES"\n", old_dir->fcb.name);
            free(old_dir);
            return -1;
        }
        d->directory = d->dcb;
        d->dcb = old_dir;   
        //d->pos_in_block = old_dir->header.block_in_disk;   
        return 0;
    }  
    if(ret > 0){
        printf(CR"The directory %s there is not in %s"RES"\n", dirname, d->dcb->fcb.name);
        free(old_dir);
        return -1;
    }
    //////////////////////
    ///////// CONTROL TRIPLE_BLOCK
    triple_block = d->dcb->indirect_blocks[2];
    if(triple_block == -1){
        printf(CR"The directory %s there is not in %s"RES"\n", dirname, d->dcb->fcb.name);
        free(old_dir);
        return -1;
    } 
    i = 0, k = 0, z = 0;
    ret = control_triple_block(NULL, d, dirname, triple_block, i, k, z, entries, 1, 1);
    if(ret < 0){
        pos_in_disk = abs(ret);
        ret = DiskDriver_readBlock(d->sfs->disk, old_dir, pos_in_disk);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_changeDir 5\n");
        if(old_dir->fcb.is_dir == 0){
            printf(CR"Can't changedir %s is a file"RES"\n", old_dir->fcb.name);
            free(old_dir);
            return -1;
        }
        d->directory = d->dcb;
        d->dcb = old_dir;
        //d->pos_in_block = old_dir->header.block_in_disk;
        return 0;
    }
    if(ret > 0){
        printf(CR"The directory %s there is not in %s"RES"\n", dirname, d->dcb->fcb.name);
        free(old_dir);
        return -1;
    }
    //////////
    free(old_dir);
    return -1;
}

// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* dp, char* dirname){

    if(dp == NULL || dirname == NULL || (dp->dcb->fcb.is_dir == 0)){
        printf("Bad Parameter in Input - SimpleFS_mkDir\n");
        return -1;
    }
    if(dp->sfs->disk->header->free_blocks <= 0){
        printf(CR"FULL MEMORY, I can't create new directory"RES"\n");
        return -1;
    }
    int ret = 0, file_block, single_block, double_block, triple_block, entries = 0, i = 0, k = 0, z = 0;
    /////////// CONTROL DIRECT BLOCK 
    file_block = dp->dcb->direct_blocks[i];
    ret = control_direct_block(NULL, dp, dirname, file_block, i, entries, 1, 0);
    if(ret > 0) return 0;
    if(ret < 0){
        printf(CR"File with this name already exists"RES"\n");
        return -1;
    } 
    ////// CONTROL SINGLE_BLOCK
    single_block = dp->dcb->indirect_blocks[0];
    if(single_block == -1) return -1;
    i = 0;
    ret = control_single_block(NULL, dp, dirname, single_block, i, entries, 1, 0);
    if(ret > 0) return 0;
    if(ret < 0){
        printf(CR"File with this name already exists"RES"\n");
        return -1;
    } 
    if(ret == 0 && dp->dcb->indirect_blocks[1] == -1 ){
        int first_free_block = dp->sfs->disk->header->first_free_block; 
        IndexBlock index_block2 = {0};  
        ret = new_index_block(dp, &index_block2, first_free_block);
        if(ret == -1) return -1;
        //printf("\nINDEX DOUBLE BLOCK %d\n", first_free_block);
        dp->dcb->indirect_blocks[1] = first_free_block;
        ret = DiskDriver_rewriteBlock(dp->sfs->disk, dp->dcb, dp->dcb->fcb.block_in_disk);
        error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_mkDir 1\n");
    }
    ////////////////
    
    /////// CONTROL DOUBLE_BLOCK
    double_block = dp->dcb->indirect_blocks[1];
    if(double_block == -1) return -1;
    i = 0, k = 0;
    ret = control_double_block(NULL, dp, dirname, double_block, i, k, entries, 1, 0); 
    if(ret > 0) return 0;
    if(ret < 0){
        printf(CR"File with this name already exists"RES"\n");
        return -1;
    } 
    if(ret == 0 && dp->dcb->indirect_blocks[2] == -1 ){  
        int first_free_block = dp->sfs->disk->header->first_free_block; 
        IndexBlock index_block3 = {0}; 
        ret = new_index_block(dp, &index_block3, first_free_block);
        if(ret == -1) return -1;
        dp->dcb->indirect_blocks[2] = first_free_block;
        ret = DiskDriver_rewriteBlock(dp->sfs->disk, dp->dcb, dp->dcb->fcb.block_in_disk);
        error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_mkDir 2\n");
    }
    //////////////////////
    ///////// CONTROL TRIPLE_BLOCK
    triple_block = dp->dcb->indirect_blocks[2];
    if(triple_block == -1) return -1;
    i = 0, k = 0, z = 0;
    ret = control_triple_block(NULL, dp, dirname, triple_block, i, k, z, entries, 1, 0);
    if(ret > 0) return 0;
    if(ret < 0){
        printf(CR"File with this name already exists"RES"\n");
        return -1;
    }
    //////////
    return -1;
}

// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(DirectoryHandle* d, char* filename){
        
    if(d == NULL || filename == NULL || sizeof(filename) > 128){
        printf("Bad Parameter in Input - SimpleFS_remove\n");
        return -1;
    }
    
    InfoBlock info = {0};
    int ret1;
    ret1 = search_file(d, filename, &info);
    if(ret1 == -1){
        return -1;
    } 
    
    if(info.block_in_disk < 0){
        //printf("block in disk < 0, value %d\n", info.block_in_disk);
        return -1;
    }
    
    FirstFileBlock file_to_rm;
    FirstDirectoryBlock dir_to_rm;
    DirectoryHandle handle = {0};
    int ret, ult, /* dim_bl, */ dim_by;
    ret = DiskDriver_readBlock(d->sfs->disk, &file_to_rm, info.block_in_disk);
    error(ret, "error in DiskDriver_readBlock in SimpleFS_remove 1\n");
    /*printf("info block in disk %d\n", info->block_in_disk);
    printf("0 name file to remove %s\n", file_to_rm.fcb.name);
    printf("0 file_to_rm  block in file %d\n", file_to_rm.header.block_in_file);
    printf("0file_to_rm  block in disk %d\n", file_to_rm.header.block_in_disk); */
    
    if(file_to_rm.fcb.is_dir == 0){    // remove file, check blocks in disk
        /* dim_bl = file_to_rm.fcb.size_in_blocks; */
        dim_by = file_to_rm.fcb.size_in_bytes;
        ret = remove_data(d->sfs->disk, &file_to_rm);
        if(ret != 0){
            printf("error in remove_data in SimpleFS_remove\n");
            return -1;
        }        
        ret = DiskDriver_freeBlock(d->sfs->disk, info.block_in_disk);
        error(ret, "Error in DiskDriver_freeBlock in SimpleFS_remove\n");
        //printf("num entries %d \n", d->dcb->num_entries);
        ult = find_last_index(d, d->dcb->num_entries-1, -1);
        if(info.pos_in_dir == d->dcb->num_entries-1 ) find_last_index(d, info.pos_in_dir, -1);
        else find_last_index(d, info.pos_in_dir, ult);
        d->dcb->num_entries--;
        d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) - 1 /* - dim_bl */; 
        d->dcb->fcb.size_in_bytes = (d->dcb->fcb.size_in_bytes) - BLOCK_SIZE - dim_by;
        ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
        error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_remove 1\n");   
    }
    else{       //remove directory recursively
        ret = DiskDriver_readBlock(d->sfs->disk, &dir_to_rm, file_to_rm.fcb.block_in_disk);
        error(ret, "error in DiskDriver_readBlock in SimpleFS_remove 3\n");
        int i = 0;
        while(dir_to_rm.num_entries > 0){
            ret = DiskDriver_readBlock(d->sfs->disk, &file_to_rm, dir_to_rm.direct_blocks[0]);
            d_handle(d, &dir_to_rm, i, &handle);
            SimpleFS_remove(&handle, file_to_rm.fcb.name);
            //printf(CB"remove %s"RES"\n", file_to_rm.fcb.name);
            i++;
        }
        // delete empty directory
        ret = DiskDriver_freeBlock(d->sfs->disk, info.block_in_disk);
        error(ret, "Error in DiskDriver_freeBlock in SimpleFS_remove\n");
        //printf("num entries cartella %d \n", d->dcb->num_entries);
        ult = find_last_index(d, d->dcb->num_entries-1, -1);
        if(info.pos_in_dir == d->dcb->num_entries-1) find_last_index(d, info.pos_in_dir, -1);
        else find_last_index(d, info.pos_in_dir, ult);
        d->dcb->num_entries--;
        d->dcb->fcb.size_in_blocks = d->dcb->fcb.size_in_blocks - 1; 
        d->dcb->fcb.size_in_bytes = d->dcb->fcb.size_in_bytes - BLOCK_SIZE;
        ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
        error(ret, "error in DiskDriver_rewriteBlock in SimpleFS_remove 2\n");
    }
    return 0;
}

void print_index_block(FirstDirectoryBlock* f, DiskDriver* disk){
    int i = 0, j = 0, k = 0, ret = 0;
    IndexBlock block_index1, block_index2, block_index3;
    printf("DirectBlock : \n");
    for(; i < DIRECT_BLOCKS_NUM; i++){
        printf("%d ", f->direct_blocks[i]);
    }
    printf("\n");
    for(i = 0; i < INDIRECT_BLOCKS_NUM; i++){
        printf("%d ", f->indirect_blocks[i]);
    }
    printf("\n");
    int single_block =  f->indirect_blocks[0];
    if(single_block != -1){
        ret = DiskDriver_readBlock(disk, &block_index1, single_block);
        error(ret, "error in DiskDriver_readBlock in print_index_block\n");
        printf("SingleBlock : \n");
        for(i = 0;  i < INDEXES; i++){
            printf("%d ", block_index1.file_blocks[i]);
        }
    }
    printf("\n");
    int double_block = f->indirect_blocks[1];
    if(double_block != -1){
        ret = DiskDriver_readBlock(disk, &block_index2, double_block);
        error(ret, "error in DiskDriver_readBlock in print_index_block\n");
        printf("DoubleBlock : \n");
        for(; j < INDEXES; j++){
            printf(CR"%d-> "RES, block_index2.file_blocks[j]);
            i = 0;
            while(block_index2.file_blocks[j] != -1 && i < INDEXES){
                ret = DiskDriver_readBlock(disk, &block_index1, block_index2.file_blocks[j]);
                error(ret, "error in DiskDriver_readBlock in print_index_block\n");
                printf("%d ", block_index1.file_blocks[i]);
                i++;
            }
            printf("\n");
        }
    }
    printf("\n");
    int triple_block = f->indirect_blocks[2];
    if(triple_block != -1){
        ret = DiskDriver_readBlock(disk, &block_index3, triple_block);
        error(ret, "error in DiskDriver_readBlock in print_index_block\n");
        printf("TripleBlock : \n");
        for(; k < INDEXES; k++){
            printf(CG"%d-> "RES, block_index3.file_blocks[k]);
            j = 0;
            while(block_index3.file_blocks[k] != -1 && j < INDEXES){
                ret = DiskDriver_readBlock(disk, &block_index2, block_index3.file_blocks[k]);
                error(ret, "error in DiskDriver_readBlock in print_index_block\n");
                printf(CR"%d-> "RES, block_index2.file_blocks[j]);
                i = 0;
                while(block_index2.file_blocks[j] != -1 && i < INDEXES){
                    ret = DiskDriver_readBlock(disk, &block_index1, block_index2.file_blocks[j]);
                    error(ret, "error in DiskDriver_readBlock in print_index_block\n");
                    printf("%d ", block_index1.file_blocks[i]);
                    i++;
                }
                j++;
                printf("\n     ");
            }
            printf("\n");
        }
    }
}