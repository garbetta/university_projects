#include "aux_fun.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int new_index_block(DirectoryHandle* d, IndexBlock* index_block, int pos){

    if(pos <= 0){
        printf(CR"Full memory, I can't allocate a new index block"RES"\n");
        return -1;
    }

    index_block->header.block_in_disk = pos;
    index_block->header.block_in_file = (d->dcb->header.block_in_file) + 1;
    index_block->header.is_data_block = 0;
    for(int i = 0; i < INDEXES; i++) index_block->file_blocks[i] = -1;
    memset(index_block->null, 0, BLOCK_SIZE - 4*INDEXES);
    int ret = DiskDriver_writeBlock(d->sfs->disk, index_block, pos);
    error(ret, "error in DiskDriver_writeBlock in new_index_block\n");

    DiskDriver_flush(d->sfs->disk);
    return 0;
}

FirstDirectoryBlock* new_dir_block(DirectoryHandle* d, const char* dirname){

    int first_free_block = d->sfs->disk->header->first_free_block;
    if(first_free_block <= 0){
        printf(CR"Full memory, I can't allocate a new index block"RES"\n");
        return NULL;
    }

    FirstDirectoryBlock* fdb = (FirstDirectoryBlock*)calloc(1, sizeof(FirstDirectoryBlock));
    fdb->header.block_in_disk = first_free_block;
    fdb->header.block_in_file = 0;
    fdb->header.is_data_block = 0;

    fdb->fcb.directory_block = d->dcb->fcb.block_in_disk;
    fdb->fcb.block_in_disk = first_free_block;
    strcpy(fdb->fcb.name, dirname);
    fdb->fcb.size_in_bytes = BLOCK_SIZE;
    fdb->fcb.size_in_blocks = 1;
    fdb->fcb.is_dir = 1;
    fdb->num_entries = 0;

    for(int i = 0; i < DIRECT_BLOCKS_NUM; i++) fdb->direct_blocks[i] = -1;
    for(int i = 0; i < INDIRECT_BLOCKS_NUM; i++) fdb->indirect_blocks[i] = -1;
    memset(fdb->null, 0, BLOCK_SIZE - sizeof(BlockHeader) - sizeof(FileControlBlock) - 4 - 4*DIRECT_BLOCKS_NUM - 4*INDIRECT_BLOCKS_NUM);

    int ret = DiskDriver_writeBlock(d->sfs->disk, fdb, first_free_block);
    error(ret, "error in DiskDriver_writeBlock in new_dir_block \n");
    
    d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) + 1;
    d->dcb->fcb.size_in_bytes = BLOCK_SIZE*(d->dcb->fcb.size_in_blocks);
    d->dcb->num_entries++;
    ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
    error(ret, "error DiskDriver_rewriteBlock in new_dir_block\n");

    DiskDriver_flush(d->sfs->disk);
    return fdb;
}

FirstFileBlock* new_file_block(DirectoryHandle* d, const char* filename){

    int first_free_block = d->sfs->disk->header->first_free_block;
    if(first_free_block <= 0){
        printf(CR"Full memory, I can't allocate a new_file_block"RES"\n");
        return NULL;
    }

    FirstFileBlock* ffb = (FirstFileBlock*)calloc(1, sizeof(FirstFileBlock));
    ffb->header.block_in_disk = first_free_block;
    ffb->header.block_in_file = 0;
    ffb->header.is_data_block = 0;

    ffb->fcb.directory_block = d->dcb->fcb.block_in_disk;          //first block of directory d
    ffb->fcb.block_in_disk = first_free_block;
    strcpy(ffb->fcb.name, filename);               
    ffb->fcb.size_in_bytes = 0;
    ffb->fcb.size_in_blocks = 1;
    ffb->fcb.is_dir = 0;
    memset(ffb->data, 0, DATA_FIRST);
    for(int i = 0; i < DIRECT_BLOCKS_NUM; i++) ffb->direct_blocks[i] = -1;
    for(int i = 0; i < INDIRECT_BLOCKS_NUM; i++) ffb->indirect_blocks[i] = -1;

    int ret = DiskDriver_writeBlock(d->sfs->disk, ffb, first_free_block);
    error(ret, "error in DiskDriver_writeBlock in new_file_block \n");

    d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) + 1;
    d->dcb->num_entries++;
    ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
    error(ret, "error DiskDriver_rewriteBlock in new_file_block 5\n");

    DiskDriver_flush(d->sfs->disk);
    return ffb;
}

FileBlock* file_block(FileHandle* pf, FileBlock* fb, int block_in_file){

    int first_free_block = pf->sfs->disk->header->first_free_block; 
    if(first_free_block <= 0){
        printf(CR"Full memory, I can't allocate a file_block"RES"\n");
        return NULL;
    }

    fb->header.block_in_disk = first_free_block;
    fb->header.block_in_file = block_in_file;
    fb->header.is_data_block = 1;
    memset(fb->data, 0, DATA);

    int ret = DiskDriver_writeBlock(pf->sfs->disk, fb, first_free_block);
    error(ret, "error in DiskDriver_writeBlock in file_block \n");

    DiskDriver_flush(pf->sfs->disk);
    return fb;
}

FileHandle* f_handle(DirectoryHandle* d, FirstFileBlock* ffb){
    
    FileHandle* file_h = (FileHandle*)calloc(1, sizeof(FileHandle));
    file_h->sfs = d->sfs;
    file_h->directory = d->dcb;
    file_h->fcb = ffb;
    //file_h->current_block = &(ffb->header);
    file_h->pos_in_file = 0;  

    return file_h;
}

void d_handle(DirectoryHandle* d, FirstDirectoryBlock* fdb, int pos_in_file, DirectoryHandle* dir_h){
    
    dir_h->sfs = d->sfs;
    dir_h->directory = d->dcb;
    dir_h->dcb = fdb;
    //dir_h->current_block = &(fdb->header);
    dir_h->pos_in_dir = pos_in_file;  
}

void color_dir(FirstFileBlock* f, char* dest){
    char res[7]= {'\0'}; 
    strcpy(dest, CM);
    strcpy(dest, RES);
    strcat(dest, CM);
    strcat(dest, f->fcb.name);
    strcat(dest, res);
}

void color_file(FirstFileBlock* f, char* dest){
    char res[7]= {'\0'}; 
    strcpy(dest, CB);
    strcpy(dest, RES);
    strcat(dest, CB);
    strcat(dest, f->fcb.name);
    strcat(dest, res);
}

int control_existence(const char* filename, FirstFileBlock* control, int is_dir){

    if(strcmp(filename, control->fcb.name) == 0) 
        return -(control->header.block_in_disk);
    return 0;
}

int control_direct_block(char** names, DirectoryHandle* d, const char* filename, int file_block, int i, int n_file, int is_dir, int change){
    
    FirstFileBlock control;
    int ret = 0;
    char col[128];
    while(file_block != -1 && i < DIRECT_BLOCKS_NUM){
        ret = DiskDriver_readBlock(d->sfs->disk, &control, file_block);
        error(ret, "error in DiskDriver_readBlock in control_direct_block 1\n");
        if(names != NULL){      //put filename in list to readDir
            if(control.fcb.is_dir == 1) color_dir(&control, col);
            else color_file(&control, col);
            strcpy(names[n_file], col);
        }else{
            ret = control_existence(filename, &control, is_dir);
            if(ret < 0) return ret; //if ret < 0, found a file otherwise countinue the search 
        }
        i++;
        n_file++;
        file_block = d->dcb->direct_blocks[i];
    }
    if(names != NULL){      //if names not null, called readDir
        if(file_block == -1) return -1;     //stop, file ended
        else return n_file;  //finished index block, continue in other block               
    }
    else{
        if(change == 0){ 
            if(i < DIRECT_BLOCKS_NUM){
                if(is_dir == 1){            //create directrory
                    FirstDirectoryBlock* dir = new_dir_block(d, filename);
                    if(dir == NULL) return 0; //full memory
                    d->dcb->direct_blocks[i] = dir->fcb.block_in_disk;
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_directory_block 1\n");
                    int block_in_disk = dir->fcb.block_in_disk;
                    free(dir);
                    return block_in_disk;
                }
                else{                       //create file
                    FirstFileBlock* file = new_file_block(d, filename);
                    if(file == NULL) return 0;  //full memory
                    d->dcb->direct_blocks[i] = file->fcb.block_in_disk;
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_directory_block 2\n");
                    int  block_in_disk = file->fcb.block_in_disk;
                    free(file);
                    return block_in_disk;
                }
            }else if(d->dcb->indirect_blocks[0] == -1){
                int first_free_block = d->sfs->disk->header->first_free_block;
                IndexBlock index_block1 = {0};
                ret = new_index_block(d, &index_block1, first_free_block);
                if(ret == -1){
                    return 0;   //full memory
                }
                d->dcb->indirect_blocks[0] = first_free_block;
                ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
                error(ret, "error DiskDriver_rewriteBlock in control_directory_block 3\n");
                return 0;
            }
        }
    }
    return 0;    
}

int control_index_block(char** names, DirectoryHandle* d, const char* filename, int file_block, int i, int n_file, IndexBlock* iblock, int is_dir, int change){
    
    FirstFileBlock control;
    int ret = 0;
    char col[128];

    while((i < INDEXES) && (file_block != -1)){
        ret = DiskDriver_readBlock(d->sfs->disk, &control, file_block);
        error(ret, "error in DiskDriver_readBlock in control_index_block 1\n");
        if(names != NULL){
            if(control.fcb.is_dir == 1) color_dir(&control, col);
            else color_file(&control, col);
            strcpy(names[n_file], col);
            if(DEBUG) printf("%s num entries %d\n", names[n_file], n_file);
        }
        else{
            ret = control_existence(filename, &control, is_dir);
            if(ret < 0) return ret;
        }
        i++;
        n_file++;
        if(i < INDEXES) file_block = iblock->file_blocks[i];
    }
    if(names != NULL){          //if names not null, called readDir
        if(file_block == -1) return -1;                 //stop, file ended
        else return n_file;             //finished index block, continue in other block               
    }
    else{
        if(change == 0){
            if(i < INDEXES){
                if(is_dir == 1){            //create directory
                    FirstDirectoryBlock* dir = new_dir_block(d, filename);
                    if(dir == NULL) return 0; //full memory
                    iblock->file_blocks[i] = dir->fcb.block_in_disk;
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, iblock, iblock->header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_index_block 1\n");
                    int block_in_disk = dir->fcb.block_in_disk;
                    free(dir);
                    return block_in_disk;
                }
                else{                       //create file
                    FirstFileBlock* file = new_file_block(d, filename);
                    if(file == NULL) return 0; //full memory
                    iblock->file_blocks[i] = file->fcb.block_in_disk;
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, iblock, iblock->header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_index_block 2\n");
                    int block_in_disk = file->fcb.block_in_disk;
                    free(file);
                    return block_in_disk;
                }
            }
        }
    }
    return 0;
}

int control_single_block(char** names, DirectoryHandle* d, const char* filename, int single_block, int i, int n_file, int is_dir, int change){
    IndexBlock index_block1;
    int ret = 0;
    ret = DiskDriver_readBlock(d->sfs->disk, &index_block1, single_block);
    error(ret, "error in DiskDriver_readBlock in control_single_block 1 \n");
    int file_block = index_block1.file_blocks[0];
    ret = control_index_block(names, d, filename, file_block, 0, n_file, &index_block1, is_dir, change);
    return ret;
}

int control_double_block(char** names, DirectoryHandle* d, const char* filename, int double_block, int i, int k, int n_file, int is_dir, int change){
    
    int index2;
    int ret = 0; i = 0;
    IndexBlock index_block2;
    ret = DiskDriver_readBlock(d->sfs->disk, &index_block2, double_block);
    error(ret, "error in DiskDriver_readBlock - control_double_block 1\n");
    index2 = index_block2.file_blocks[k];
    while(index2 != -1 && k < INDEXES){
        i = 0;
        ret = control_single_block(names, d, filename, index2, i, n_file, is_dir, change);
        // if > 0 returns the number of total entries 
        if(ret < 0 ) return ret;
        k++;
        n_file = ret;
        index2 = index_block2.file_blocks[k];   
    }
    if(names != NULL){                      //if names not null, called readDir
        if(index2 == -1) return -1;         //stop, file ended
        else return n_file;                 //finished index block, continue in other block               
    }else{
        if(ret > 0) return ret;
        if(change == 0){
            if(k < INDEXES){
                int first_free_block = d->sfs->disk->header->first_free_block; 
                IndexBlock index_block1 = {0};
                ret = new_index_block(d, &index_block1, first_free_block);
                if(ret == -1) return 0; //full memory
                index_block2.file_blocks[k] = first_free_block;
                d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) + 1;
                if(is_dir == 1){                    //create directory
                    FirstDirectoryBlock* new_dir = new_dir_block(d, filename);
                    if(new_dir == NULL) return 0; //full memory
                    index_block1.file_blocks[0] = new_dir->fcb.block_in_disk;
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block1, index_block1.header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_double_block 1\n");
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block2, index_block2.header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_double_block 2\n");
                    int block_in_disk = new_dir->header.block_in_disk;
                    free(new_dir);
                    return block_in_disk;
                }
                else{                                  //create file
                    FirstFileBlock* new_file = new_file_block(d, filename);
                    if(new_file == NULL) return 0; //full memory
                    index_block1.file_blocks[0] = new_file->fcb.block_in_disk;
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block1, index_block1.header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_double_block 3\n");
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block2, index_block2.header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_double_block 4\n");
                    int block_in_disk = new_file->header.block_in_disk;
                    free(new_file);
                    return block_in_disk;
                }
            }
        }
    }
    return 0;
}

int control_triple_block(char** names, DirectoryHandle* d, const char* filename, int triple_block, int i, int k, int z, int n_file, int is_dir, int change){
    IndexBlock index_block3;
    int ret = 0, index3; 
    ret = DiskDriver_readBlock(d->sfs->disk, &index_block3, triple_block);
    error(ret, "error in DiskDriver_readBlock in control triple block 1\n");
    index3 = index_block3.file_blocks[z];
    while(index3 != -1 && z < INDEXES){
        k=0;
        ret = control_double_block(names, d, filename, index3, i, k, n_file, is_dir, change);
        if(ret < 0 ) return ret;
        z++;
        n_file = ret;
        index3 = index_block3.file_blocks[z]; 
    }
    if(names != NULL){                      //if names not null, called readDir
        if(index3 == -1) return -1;         //stop, file ended
        else return n_file;                 //finished index block, continue in other block
    }else{
        if(ret > 0) return ret;
        if(change == 0){
            if(z < INDEXES){
                int first_free_block = d->sfs->disk->header->first_free_block;
                IndexBlock index_block2 = {0};
                ret = new_index_block(d, &index_block2, first_free_block);
                if(ret == -1) return 0; // full memory
                index_block3.file_blocks[z] = first_free_block;
                d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) + 1;

                first_free_block = d->sfs->disk->header->first_free_block;
                IndexBlock index_block1 = {0}; 
                ret = new_index_block(d, &index_block1, first_free_block);
                if(ret == -1) return 0; // full memory
                index_block2.file_blocks[0] = first_free_block;
                d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) + 1;
                if(is_dir == 1){                    //create directory
                    FirstDirectoryBlock* new_dir = new_dir_block(d, filename);
                    if(new_dir == NULL) return 0; // full memory
                    index_block1.file_blocks[0] = new_dir->fcb.block_in_disk;
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block1, index_block1.header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_triple_block 1\n");
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block2, index_block2.header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_triple_block 2\n");
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block3, index_block3.header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_trile_block 3\n");
                    int block_in_disk = new_dir->header.block_in_disk;
                    free(new_dir);
                    return block_in_disk;
                }
                else{                                  //create file
                    FirstFileBlock* new_file = new_file_block(d, filename);
                    if(new_file == NULL) return 0; //full memory
                    index_block1.file_blocks[0] = new_file->fcb.block_in_disk;
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block1, index_block1.header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_triple_block 4\n");
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block2, index_block2.header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_triple_block 5\n");
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block3, index_block3.header.block_in_disk);
                    error(ret, "error DiskDriver_rewriteBlock in control_triple_block 6\n");
                    int block_in_disk = new_file->header.block_in_disk;
                    free(new_file);
                    return block_in_disk;
                }
            }
            printf(CR"Indexes full"RES"\n");
            return 0;
        }
    }
    return 0;
}

int find_last_index(DirectoryHandle* d, int entry, int sost){
    //in this case the block already exists, do not create a new block, 
    //add an index in specific position
    int pos = 0, offset, single_block, double_block, triple_block, ret = 0, ret2 = 0;
    
    if(entry < DIRECT_BLOCKS_NUM){
        //new entry in direct block
        pos = entry % DIRECT_BLOCKS_NUM;
        if(sost == -1){
            ret2 = d->dcb->direct_blocks[pos];
            d->dcb->direct_blocks[pos] = -1;
            ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
            error(ret, "error in DiskDriver_rewriteBlock in find_pos 1\n");
            return ret2;
        }
        d->dcb->direct_blocks[pos] = sost;
        ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
        error(ret, "error in DiskDriver_rewriteBlock in find_pos 2\n");
        return sost;
    }
    int rem = entry - DIRECT_BLOCKS_NUM;
    if(rem < INDEXES){
        //new entry in single indirect block
        pos = rem % INDEXES;
        single_block = d->dcb->indirect_blocks[0];
        if(single_block == -1) return -1;   
        IndexBlock index_block1;
        ret = DiskDriver_readBlock(d->sfs->disk, &index_block1, single_block);
        error(ret, "Error in DiskDriver_readBlock in find_last_free 1\n");
        if(sost == -1){
            ret2 = index_block1.file_blocks[pos];
            index_block1.file_blocks[pos] = -1;
            if(pos == 0){       //put -1 in first position, all index block contains -1, free index block
                d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) - 1;
                ret = DiskDriver_freeBlock(d->sfs->disk, index_block1.header.block_in_disk);
                error(ret, "Error in DiskDriver_freeBlock in find_last_index\n");
                d->dcb->indirect_blocks[0] = -1;
                ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
                error(ret, "error in DiskDriver_rewriteBlock in find_pos 3\n");
            } 
            else {
            ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block1, single_block);
            error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 4\n");
            }

            return ret2;
        }
        index_block1.file_blocks[pos] = sost;
        ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block1, single_block);
        error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 5\n");
        return sost;
    }
    rem = rem - INDEXES;
    if(rem < INDEXES*INDEXES){
        //new entry in double indirect block
        pos = rem / INDEXES;
        offset = rem % INDEXES;
        double_block = d->dcb->indirect_blocks[1];
        IndexBlock index_block2; 
        if(double_block == -1) return -1;
        ret = DiskDriver_readBlock(d->sfs->disk, &index_block2, double_block);
        error(ret, "Error in DiskDriver_readBlock in find_last_index 2\n");
        int index2 = index_block2.file_blocks[pos];
        if(index2 == -1) return -1;
        IndexBlock index_block1;
        ret = DiskDriver_readBlock(d->sfs->disk, &index_block1, index2);
        error(ret, "Error in DiskDriver_readBlock in find_last_index 3\n");
        if(sost == -1){
            ret2 = index_block1.file_blocks[offset];
            index_block1.file_blocks[offset] = -1;
            if(offset == 0){             //put -1 in first position, all index block contains -1, free index block
                d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) - 1;
                ret = DiskDriver_freeBlock(d->sfs->disk, index_block1.header.block_in_disk);
                error(ret, "Error in DiskDriver_freeBlock in find_last_index\n");
                index_block2.file_blocks[pos] = -1;
                ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block2, double_block);
                error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 6\n");
                if(pos == 0){           //put -1 in first position, all index block contains -1, free index block
                    d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) - 1;
                    ret = DiskDriver_freeBlock(d->sfs->disk, index_block2.header.block_in_disk);
                    error(ret, "Error in DiskDriver_freeBlock in find_last_index\n");
                    d->dcb->indirect_blocks[1] = -1;
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
                    error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 7\n");
                }
            }
            else{
                ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block1, index2);
                error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 8\n");
            }            
            return ret2;
        }
        index_block1.file_blocks[offset] = sost;
        ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block1, index2);
        error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 9\n");
        return sost;
    }
    rem = rem - INDEXES*INDEXES; 
    //printf("rem %d \n", rem);
        if(rem < INDEXES*INDEXES*INDEXES){
        //new entry in triple indirect block
        int pos_b = rem / (INDEXES*INDEXES);
        int prov = rem / INDEXES;
        pos = prov % INDEXES;
        offset = rem % INDEXES;

        triple_block = d->dcb->indirect_blocks[2];
        if(triple_block == -1) return -1;
        IndexBlock index_block3;
        ret = DiskDriver_readBlock(d->sfs->disk, &index_block3, triple_block);
        error(ret, "Error in DiskDriver_readBlock in find_last_index 4\n");
        int index3 = index_block3.file_blocks[pos_b];

        IndexBlock index_block2;
        ret = DiskDriver_readBlock(d->sfs->disk, &index_block2, index3);
        error(ret, "Error in DiskDriver_readBlock in find_last_index 5\n");
        int index2 = index_block2.file_blocks[pos];

        IndexBlock index_block1;
        ret = DiskDriver_readBlock(d->sfs->disk, &index_block1, index2);
        error(ret, "Error in DiskDriver_readBlock in find_last_index 6\n");
        if(sost == -1){
            ret2 = index_block1.file_blocks[offset];
            index_block1.file_blocks[offset] = -1;
            if(offset == 0){    //put -1 in first position, all index block contains -1, free index block
                d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) - 1;
                ret = DiskDriver_freeBlock(d->sfs->disk, index_block1.header.block_in_disk);
                error(ret, "Error in DiskDriver_freeBlock in find_last_index\n");
                index_block2.file_blocks[pos] = -1;
                ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block2, index3);
                error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 10\n");
                if(pos == 0){   //put -1 in first position, all index block contains -1, free index block
                    d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) - 1;
                    ret = DiskDriver_freeBlock(d->sfs->disk, index_block2.header.block_in_disk);
                    error(ret, "Error in DiskDriver_freeBlock in find_last_index\n");
                    index_block3.file_blocks[pos_b] = -1;
                    ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block3, triple_block);
                    error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 11\n");
                    if(pos_b == 0){ //put -1 in first position, all index block contains -1, free index block
                        d->dcb->fcb.size_in_blocks = (d->dcb->fcb.size_in_blocks) - 1;
                        ret = DiskDriver_freeBlock(d->sfs->disk, index_block3.header.block_in_disk);
                        error(ret, "Error in DiskDriver_freeBlock in find_last_index\n");
                        d->dcb->indirect_blocks[2] = -1;
                        ret = DiskDriver_rewriteBlock(d->sfs->disk, d->dcb, d->dcb->fcb.block_in_disk);
                        error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 12\n");
                    }
                }
            }
            else{
                ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block1, index2);
                error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 13\n");
            }
            return ret2;
        }
        index_block1.file_blocks[offset] = sost;
        ret = DiskDriver_rewriteBlock(d->sfs->disk, &index_block1, index2);
        error(ret, "Error in DiskDriver_rewriteBlock in find_last_index 14\n");        
        return sost;
    }

    return sost;
}

int first_free_index(FileHandle* f, int new_block, int pos_to_search, int search){      //in write to find index block in specific pos
    //in this case the block already exists, do not create a new block, 
    //add an entry in specific position
    pos_to_search--;
    //printf("size in block first free index %d\n", pos_to_search);
    int pos = 0, offset, single_block, double_block, triple_block, ret = 0;
    IndexBlock index_block = {0};
    if(pos_to_search < DIRECT_BLOCKS_NUM){
        //new entry in direct block
        pos = pos_to_search % DIRECT_BLOCKS_NUM;
        if(search) return f->fcb->direct_blocks[pos];
        f->fcb->direct_blocks[pos] = new_block;
        //f->current_block = &(f->fcb->header);
        f->fcb->fcb.size_in_blocks = (f->fcb->fcb.size_in_blocks)+1;
        ret = DiskDriver_rewriteBlock(f->sfs->disk, f->fcb, f->fcb->fcb.block_in_disk);
        error(ret, "Error in DiskDriver_readBlock in find_first_free 1\n");
        return new_block;
    }
    int rem = pos_to_search - DIRECT_BLOCKS_NUM;
    if(rem < INDEXES){
        //new entry in single indirect block
        pos = rem % INDEXES;
        single_block = f->fcb->indirect_blocks[0];
        if(single_block  == -1){ //new single indirect block
            ret = new_index_block((DirectoryHandle*)f, &index_block, f->sfs->disk->header->first_free_block);
            if(ret == -1) return -2;    //full memory
            f->fcb->indirect_blocks[0] = index_block.header.block_in_disk;
            single_block = f->fcb->indirect_blocks[0];
            ret = DiskDriver_rewriteBlock(f->sfs->disk, f->fcb, f->fcb->fcb.block_in_disk);
            error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 2\n");
            f->directory->fcb.size_in_blocks = (f->directory->fcb.size_in_blocks) + 1;
        } 
        IndexBlock index_block1; 
        ret = DiskDriver_readBlock(f->sfs->disk, &index_block1, single_block);
        error(ret, "Error in DiskDriver_readBlock in find_first_free 1\n");
        if(search) return index_block1.file_blocks[pos];
        index_block1.file_blocks[pos] = new_block;
        //f->current_block = &(f->fcb->header);
        f->fcb->fcb.size_in_blocks = (f->fcb->fcb.size_in_blocks)+1;
        
        ret = DiskDriver_rewriteBlock(f->sfs->disk, f->fcb, f->fcb->fcb.block_in_disk);
        error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 3\n");
        ret = DiskDriver_rewriteBlock(f->sfs->disk, &index_block1, single_block);
        error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 4\n");
        return new_block;
    }
    rem = rem - INDEXES;
    if(rem < INDEXES*INDEXES){
        //new entry in doube indirect block
        pos = rem / INDEXES;
        offset = rem % INDEXES;
        double_block = f->fcb->indirect_blocks[1];
        if(double_block == -1){ //new double indirect block
            ret = new_index_block((DirectoryHandle*)f, &index_block, f->sfs->disk->header->first_free_block);
            if(ret == -1) return -2;    //full memory
            if(DEBUG) printf("\n wrote index block in pos : %d\n", index_block.header.block_in_disk);

            f->fcb->indirect_blocks[1] = index_block.header.block_in_disk;
            double_block = f->fcb->indirect_blocks[1];
            ret = DiskDriver_rewriteBlock(f->sfs->disk, f->fcb, f->fcb->fcb.block_in_disk);
            error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 5\n");

        } 
        IndexBlock index_block2; 
        ret = DiskDriver_readBlock(f->sfs->disk, &index_block2, double_block);
        error(ret, "Error in DiskDriver_readBlock in find_first_free 2\n");
        int index2 = index_block2.file_blocks[pos];
        if(index2 == -1){
            ret = new_index_block((DirectoryHandle*)f, &index_block, f->sfs->disk->header->first_free_block);
            if(ret == -1) return -2;   //full memory
            if(DEBUG) printf("\n wrote index block in pos : %d\n", index_block.header.block_in_disk);
            index_block2.file_blocks[pos] = index_block.header.block_in_disk;
            index2 = index_block2.file_blocks[pos];
            ret = DiskDriver_rewriteBlock(f->sfs->disk, &index_block2, index_block2.header.block_in_disk);
            error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 6\n");
        }
        IndexBlock index_block1;
        ret = DiskDriver_readBlock(f->sfs->disk, &index_block1, index2);
        error(ret, "Error in DiskDriver_readBlock in find_first_free 3\n");
        if(search) return index_block1.file_blocks[offset];
        index_block1.file_blocks[offset] = new_block;
        f->fcb->fcb.size_in_blocks = (f->fcb->fcb.size_in_blocks) + 1;
        f->fcb->header.is_data_block = 1;
        //f->current_block = &(f->fcb->header);
        ret = DiskDriver_rewriteBlock(f->sfs->disk, f->fcb, f->fcb->fcb.block_in_disk);
        error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 7\n");
        ret = DiskDriver_rewriteBlock(f->sfs->disk, &index_block1, index2);
        error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 8\n");
        return new_block;

    }
    rem = rem - INDEXES*INDEXES;
    if(rem < INDEXES*INDEXES*INDEXES){
        //new entry in triple indirect block
        int pos_b = rem / (INDEXES*INDEXES);
        int prov = rem / INDEXES;
        pos = prov % INDEXES;
        offset = rem % INDEXES;
        triple_block = f->fcb->indirect_blocks[2];
        if(triple_block == -1){ // new triple indirect block
            ret = new_index_block((DirectoryHandle*)f, &index_block, f->sfs->disk->header->first_free_block);
            if(ret == -1) return -2;    //full memory
            f->fcb->indirect_blocks[2] = index_block.header.block_in_disk;
            triple_block = f->fcb->indirect_blocks[2];
            ret = DiskDriver_rewriteBlock(f->sfs->disk, f->fcb, f->fcb->fcb.block_in_disk);
            error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 9\n");
        }
        IndexBlock index_block3;
        ret = DiskDriver_readBlock(f->sfs->disk, &index_block3, triple_block);
        error(ret, "Error in DiskDriver_readBlock in find_first_free 4\n");
        int index3 = index_block3.file_blocks[pos_b];
        if(index3 == -1){
            ret = new_index_block((DirectoryHandle*)f, &index_block, f->sfs->disk->header->first_free_block);
            if(ret == -1) return -2;    //full memory

            index_block3.file_blocks[pos_b] = index_block.header.block_in_disk;
            index3 = index_block3.file_blocks[pos_b];

            ret = DiskDriver_rewriteBlock(f->sfs->disk, &index_block3, index_block3.header.block_in_disk);
            error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 10\n");
            //free(index_block);
        }

        IndexBlock index_block2;
        ret = DiskDriver_readBlock(f->sfs->disk, &index_block2, index3);
        error(ret, "Error in DiskDriver_readBlock in find_first_free 5\n");
        int index2 = index_block2.file_blocks[pos];
        if(index2 == -1){
            ret = new_index_block((DirectoryHandle*)f, &index_block, f->sfs->disk->header->first_free_block);
            if(ret == -1) return -2;    //full memory
            index_block2.file_blocks[pos] = index_block.header.block_in_disk;
            index2 = index_block2.file_blocks[pos];
            ret = DiskDriver_rewriteBlock(f->sfs->disk, &index_block2, index_block2.header.block_in_disk);
            error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 11\n");
            //free(index_block);
        }
        IndexBlock index_block1;
        ret = DiskDriver_readBlock(f->sfs->disk, &index_block1, index2);
        error(ret, "Error in DiskDriver_readBlock in find_first_free 6\n");
        if(search) return index_block1.file_blocks[offset]; //
        index_block1.file_blocks[offset] = new_block;   //
        f->fcb->fcb.size_in_blocks = (f->fcb->fcb.size_in_blocks) + 1;
        //f->current_block = &(f->fcb->header);
        
        ret = DiskDriver_rewriteBlock(f->sfs->disk, f->fcb, f->fcb->fcb.block_in_disk);
        error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 12\n");
        ret = DiskDriver_rewriteBlock(f->sfs->disk, &index_block1, index2);
        error(ret, "Error in DiskDriver_rewriteBlock in find_first_free 13\n");
        return new_block;
    }

    return new_block;
}

InfoBlock* direct_rem(DirectoryHandle* d, const char* filename, int file_block, int i, int n_file, InfoBlock* info){
         
    FirstFileBlock control;
    int ret = 0;
    while(file_block != -1 && i < DIRECT_BLOCKS_NUM){
        ret = DiskDriver_readBlock(d->sfs->disk, &control, file_block);
        error(ret, "error in DiskDriver_readBlock in direct_rem\n");
        if(strcmp(filename, control.fcb.name) == 0){
            info->pos_in_dir = n_file;
            info->block_in_disk = control.header.block_in_disk;
            return info;
        }   
        n_file++;
        i++;
        file_block = d->dcb->direct_blocks[i];
    }
    if(file_block == -1) return NULL;   //return -1 if there are no more file
    else{
        info->pos_in_dir = n_file;      //return 0 if finished index block, but there is more files
        info->block_in_disk = -1;
        return info;
    }                   
}

InfoBlock* index_rem(DirectoryHandle* d, const char* filename, int file_block, int i, int n_file, IndexBlock* iblock, InfoBlock* info){
    
    FirstFileBlock control;
    int ret = 0;
    while((file_block != -1) && (i < INDEXES) ){
        //printf(" index_rem %d\n", file_block);
        ret = DiskDriver_readBlock(d->sfs->disk, &control, file_block);
        error(ret, "error in DiskDriver_readBlock in index_rem\n");
        if(strcmp(filename, control.fcb.name)==0){
            info->pos_in_dir = n_file;
            info->block_in_disk = control.header.block_in_disk;
            return info;
        }   
        i++;
        n_file++;
        if(i < INDEXES) file_block = iblock->file_blocks[i];
    }
    if(file_block == -1 || info == NULL) return NULL; //return -1 if there are no more file
    else{
        info->pos_in_dir = n_file;      //return 0 if finished index block, but there is more files
        info->block_in_disk = -1;
        return info;
    }
}
    
InfoBlock* single_rem(DirectoryHandle* d, const char* filename, int single_block, int i, int n_file, InfoBlock* info){
    
    IndexBlock index_block1;
    int ret = 0;
    ret = DiskDriver_readBlock(d->sfs->disk, &index_block1, single_block);
    error(ret, "error in DiskDriver_readBlock - single_rem\n");
    int file_block = index_block1.file_blocks[0];
    info = index_rem(d, filename, file_block, 0, n_file, &index_block1, info);
    return info;
}

InfoBlock* double_rem(DirectoryHandle* d, const char* filename, int double_block, int i, int k, int n_file, InfoBlock* info){
    
    int ret = 0, index2;
    IndexBlock index_block2;
    
    ret = DiskDriver_readBlock(d->sfs->disk, &index_block2, double_block);
    error(ret, "error in DiskDriver_readBlock - double_rem\n");
    index2 = index_block2.file_blocks[k];
    while(index2 != -1 && k < INDEXES){
        info = single_rem(d, filename, index2, 0, n_file, info);
        // if > 0 returns the number of total entries 
        if((info != NULL && info->block_in_disk > 0) || info == NULL) return info;
        k++;
        n_file = info->pos_in_dir;
        index2 = index_block2.file_blocks[k];   
        //printf("\n index %d\n", index2);
    }
    if(index2 == -1 || info == NULL) return NULL;
    else return info;
}

InfoBlock* triple_rem(DirectoryHandle* d, const char* filename, int triple_block, int i, int k, int z, int n_file, InfoBlock* info){
    int ret = 0, index3;
    IndexBlock index_block3;

    ret = DiskDriver_readBlock(d->sfs->disk, &index_block3, triple_block);
    error(ret, "error in DiskDriver_readBlock - triple_rem\n");
    index3 = index_block3.file_blocks[z];
    while(index3 != -1 && z < INDEXES){
        info = double_rem(d, filename, index3, 0, 0, n_file, info);
        if((info != NULL && info->block_in_disk > 0) || info == NULL) return info;
        z++;
        n_file = info->pos_in_dir;
        index3 = index_block3.file_blocks[z];
    }
    if(index3 == -1 || info == NULL) return NULL;
    else return info;
}

int remove_data(DiskDriver* disk, FirstFileBlock* file_to_rm){
    int i = 0, block_num, ret;
    while(i < DIRECT_BLOCKS_NUM && file_to_rm->direct_blocks[i] != -1){
        //printf("name file to remove  %s\n", file_to_rm.fcb.name);
        //printf("file_to_rm  block in disk %d\n", file_to_rm.header.block_in_disk);
        block_num = file_to_rm->direct_blocks[i];
        ret = DiskDriver_freeBlock(disk, block_num);
        error(ret, "Error in DiskDriver_freeBlock in remove_data\n");
        i++;
    }
    IndexBlock index1, index2, index3;
    if(file_to_rm->indirect_blocks[0] != -1){
        ret = DiskDriver_readBlock(disk, &index1, file_to_rm->indirect_blocks[0]);
        error(ret, "error in DiskDriver_readBlock in Simple_remove 1\n");
        int j = 0;
        i = 0;
        while(i < INDEXES && index1.file_blocks[i] != -1){
            block_num = index1.file_blocks[i];
            ret = DiskDriver_freeBlock(disk, block_num);
            error(ret, "Error in DiskDriver_freeBlock in remove_data\n");
            i++;
        }
        ret = DiskDriver_freeBlock(disk, file_to_rm->indirect_blocks[0]);
        error(ret, "Error in DiskDriver_freeBlock in remove_data\n");
        i = 0;
        if(file_to_rm->indirect_blocks[1] != -1){
            ret = DiskDriver_readBlock(disk, &index2, file_to_rm->indirect_blocks[1]);
            error(ret, "error in DiskDriver_readBlock in Simple_remove 2\n");
            j = 0;
            int k = 0;

            while(j < INDEXES && index2.file_blocks[j] != -1){
                block_num = index2.file_blocks[j];
                ret = DiskDriver_readBlock(disk, &index1, block_num);
                error(ret, "error in DiskDriver_readBlock in Simple_remove 3\n");
                i=0;
                while(i < INDEXES && index1.file_blocks[i] != -1){
                    block_num = index1.file_blocks[i];
                    ret = DiskDriver_freeBlock(disk, block_num);
                    error(ret, "Error in DiskDriver_freeBlock in remove_data\n");
                    i++;
                }
                ret = DiskDriver_freeBlock(disk, index1.header.block_in_disk);
                error(ret, "Error in DiskDriver_freeBlock in remove_data\n");
                j++;
            }
            ret = DiskDriver_freeBlock(disk, index2.header.block_in_disk);
            error(ret, "Error in DiskDriver_freeBlock in remove_data\n");
            if(file_to_rm->indirect_blocks[2] != -1){
                ret = DiskDriver_readBlock(disk, &index3, file_to_rm->indirect_blocks[2]);
                error(ret, "error in DiskDriver_readBlock in Simple_remove 4\n");
                k = 0;
                while(k < INDEXES && index3.file_blocks[k] != -1){
                    block_num = index3.file_blocks[k];
                    ret = DiskDriver_readBlock(disk, &index2, block_num);
                    error(ret, "error in DiskDriver_readBlock in Simple_remove 5\n");
                    j = 0;
                    while(j < INDEXES && index2.file_blocks[j] != -1){
                        block_num = index2.file_blocks[j];
                        ret = DiskDriver_readBlock(disk, &index1, block_num);
                        error(ret, "error in DiskDriver_readBlock in Simple_remove 6\n");
                        i = 0;
                        while(i < INDEXES && index1.file_blocks[i] != -1){
                            block_num = index1.file_blocks[i];
                            ret = DiskDriver_freeBlock(disk, block_num);
                            error(ret, "Error in DiskDriver_freeBlock in remove_data\n");
                            i++;
                        }
                        ret = DiskDriver_freeBlock(disk, index1.header.block_in_disk);
                        error(ret, "Error in DiskDriver_freeBlock in remove_data\n");
                        j++;
                    }
                    ret = DiskDriver_freeBlock(disk, index2.header.block_in_disk);
                    error(ret, "Error in DiskDriver_freeBlock in remove_data\n");
                    k++;
                }
                ret = DiskDriver_freeBlock(disk, index3.header.block_in_disk);
                error(ret, "Error in DiskDriver_freeBlock in remove_data\n");
            }
        }
    }
    return 0;
}

int search_file(DirectoryHandle* d, const char* filename, InfoBlock* info){

    if(d == NULL || filename == NULL){
        printf("Bad Parameters in Input - search_file\n");
        return -1;
    }

    if(d->dcb->num_entries == 0 ){
        printf(CR"This directory is empty, there are no files to remove"RES"\n");
        return -1;
    } 
    //I need to check that the dirname is in d 
    int file_block, single_block, double_block, triple_block, entries = 0, i = 0, k = 0, z = 0;
    ///// CONTROL DIRECT BLOCK
    file_block = d->dcb->direct_blocks[i];
    info = direct_rem(d, filename, file_block, i, entries, info);
    if(info != NULL && info->block_in_disk > 0){
        return 0;
    }       
    if(info == NULL){
        printf(CR"The directory %s there is not in %s"RES"\n", filename, d->dcb->fcb.name);
        //free(info);
        return -1;
    }
    ////////
    ////// CONTROL SINGLE_BLOCK
    single_block = d->dcb->indirect_blocks[0];
    if(single_block == -1){
        printf(CR"The directory %s there is not in %s"RES"\n", filename, d->dcb->fcb.name);
        return -1;
    } 
    i = 0;
    info = single_rem(d, filename, single_block, i, info->pos_in_dir, info);
    if(info != NULL && info->block_in_disk > 0){
        return 0;
    }       
    if(info == NULL){
        printf(CR"The directory %s there is not in %s"RES"\n", filename, d->dcb->fcb.name);
        return -1;
    }
    ////////////////
    /////// CONTROL DOUBLE_BLOCK
    double_block = d->dcb->indirect_blocks[1];
    if(double_block == -1){
        printf(CR"The directory %s there is not in %s"RES"\n", filename, d->dcb->fcb.name);
        return -1;
    }      
    i = 0, k = 0;
    info = double_rem(d, filename, double_block, i, k, info->pos_in_dir, info);    
    if(info != NULL && info->block_in_disk > 0){
        return 0;
    }      
    if(info == NULL){
        printf(CR"The directory %s there is not in %s"RES"\n", filename, d->dcb->fcb.name);
        return -1;
    }
    //////////////////////
    ///////// CONTROL TRIPLE_BLOCK
    triple_block = d->dcb->indirect_blocks[2];
    if(triple_block == -1){
        printf(CR"The directory %s there is not in %s"RES"\n", filename, d->dcb->fcb.name);
        //free(info);
        return -1;
    }
    i = 0, k = 0, z=0;
    info = triple_rem(d, filename, triple_block, i, k, z, info->pos_in_dir, info);
    if(info != NULL && info->block_in_disk > 0){
        return 0;
    } 
    if(info == NULL){
        printf(CR"The directory %s there is not in %s"RES"\n", filename, d->dcb->fcb.name);
        return -1;
    } 
    //////////
    return -1;
}

int str_to_wrt(FileHandle* f, void* data, int to_write, int written, int num_block, int size){

    int i = 0, block_in_file , ret, next_block;
    while(size > 0){ 
        FileBlock* ff_block = (FileBlock*)calloc(1, sizeof(FileBlock));
        i++;
        //in empty block
        next_block = first_free_index(f, -1, num_block+i, 1);
        //printf("next block %d \n", next_block);
        if(next_block == -1){ //need new block
            block_in_file = f->fcb->fcb.size_in_blocks-1;
            ff_block = file_block(f, ff_block, block_in_file);
            if(ff_block == NULL){
                free(ff_block);
                return written;
            } 
        }
        else{
            printf(CY"next_block %d " RES"\n", next_block);
            ret = DiskDriver_readBlock(f->sfs->disk, ff_block, next_block);
            error(ret, "error in DiskDriver_readBlock in str_to_wrt 1\n");
        }
        if(size <= DATA) to_write = size;
        else to_write = DATA;
        strncpy(ff_block->data, data + written, to_write);
        written += to_write;
        size -= to_write;
        ret = DiskDriver_rewriteBlock(f->sfs->disk, ff_block, ff_block->header.block_in_disk);
        error(ret, "error in DiskDriver_rewriteBlock in str_to_wrt 21\n");
        if(next_block == -1){
            // put index of new data block in index block
            ret = first_free_index(f, ff_block->header.block_in_disk, f->fcb->fcb.size_in_blocks, 0);
            if(ret == -2){
                free(ff_block); 
                return written;
            }
        }
        free(ff_block);
    }
    return written;
}

int str_to_wrt_app(FileHandle* f, void* data, int to_write, int written, int size){

    int block_in_file, ret = 0;
    while(size > 0){
        FileBlock* f_block = (FileBlock*)calloc(1, sizeof(FileBlock));
        block_in_file = f->fcb->fcb.size_in_blocks-1;
        f_block = file_block(f, f_block, block_in_file);
        if(f_block == NULL){
            free(f_block);
            return written; 
        } 
        if(size <= DATA) to_write = size;
        else to_write = DATA;
        strncpy(f_block->data, data + written, to_write);
        written += to_write;
        f->fcb->fcb.size_in_bytes += to_write;
        size -= to_write;
        ret = DiskDriver_rewriteBlock(f->sfs->disk, f_block, f_block->header.block_in_disk);
        error(ret, "error in DiskDriver_rewriteBlock in str_to_wrt_app\n");
        // put index of new data block in index block
        ret = first_free_index(f, f_block->header.block_in_disk, f->fcb->fcb.size_in_blocks, 0);
        if(ret == -2){
          free(f_block); 
          return written;
        }
        free(f_block);
    }
    return written;
}

int str_to_read(FileHandle* f, void* data, int dim_read, int read_bytes, int block, int offset){
    int ret;
    FileBlock f_block;
    ret = DiskDriver_readBlock(f->sfs->disk, &f_block, block);
    error(ret, "error in DiskDriver_Block in str_to_read\n");
    strncpy(data + read_bytes, f_block.data + offset, dim_read);
    read_bytes += dim_read;
    return read_bytes;
}