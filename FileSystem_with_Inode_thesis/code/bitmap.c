#include "bitmap.h"
#include <stdio.h>

// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num){

    BitMapEntryKey e = {0};
    if(num < 0){
        printf("Bad Parameter in input - BitMap_blockToIndex\n");
        return e;
    }

    e.entry_num = num / BLOCK_SIZE_BITMAP;              //index array
    e.bit_num = num % BLOCK_SIZE_BITMAP;                //offset
    
    if(DEBUG) printf("block %d has index in array %d and offset %d\n", num, e.entry_num, e.bit_num);
    
    return e;
}

// converts a bit to a linear index
int BitMap_indexToBlock(int entry, uint8_t bit_num){
   
    if(entry < 0 || bit_num < 0){
        printf("Bad Parameter in input - BitMap_indexToBlock\n");
        return -1;
    } 
    int index = entry*BLOCK_SIZE_BITMAP + bit_num;              //8 = bit in byte

    if(DEBUG) printf("index %d and offset %d generate block %d\n", entry, bit_num, index);

    return index;
}

// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(BitMap* bmap, int start, int status){
    
    if(start >= bmap->num_bits){
        printf("Out of range - BitMap_get\n");
        return -1;
    }

    while(start < bmap->num_bits){
        //index of block start 
        BitMapEntryKey e = BitMap_blockToIndex(start);

        for(;e.bit_num < BLOCK_SIZE_BITMAP; e.bit_num++){
            int mask = 0x01 << e.bit_num;
            int v_block = bmap->entries[e.entry_num] & mask;       
            //(v_block >> e.bit_num) the value at the e.bit_num position of e.entry_num index
            if ((v_block >> e.bit_num) == status){

                if(DEBUG) printf("first block with status %d is %d\n", status, e.entry_num*BLOCK_SIZE_BITMAP + e.bit_num);
                
                return e.entry_num*BLOCK_SIZE_BITMAP + e.bit_num;
            } 
        }
        start = e.entry_num*BLOCK_SIZE_BITMAP + e.bit_num;
    }
    //here only if there is not a bit with value equal to status
    return -1;
}

// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status){
    
    if(pos >= bmap->num_bits){
        printf("Out of range - BitMap_set\n");
        return -1;
    }
    
    BitMapEntryKey e = BitMap_blockToIndex(pos);

    char mask = 0x01 << e.bit_num;

    if(status){
        // status == 1
        bmap->entries[e.entry_num] |= mask;

        if(DEBUG) printf("status %d, entry %d\n", status, bmap->entries[e.entry_num]);

        return bmap->entries[e.entry_num];
    }
    else{
        // status == 0
        bmap->entries[e.entry_num] &= ~mask;

        if(DEBUG) printf("status %d, entry %d\n", status,  bmap->entries[e.entry_num]);

        return bmap->entries[e.entry_num];
    }
   
}

//print byte bitmap
void print_bitmap_byte(char* bdata, int n_byte){
    printf("Print array bitmap\n");
    for(int i = 0; i < n_byte; i++){
        printf("cell %d-th is : %d \n", i, bdata[i]);
    }
    printf("\n");
}

//print bit bitmap
void print_bitmap_bit(char* bdata, int n_byte){
    int c = 0;
    printf("Print bit of bitmap\n");
    for(int j = 0; j < n_byte; j++){
        for(int i = 0; i < BLOCK_SIZE_BITMAP; i++){
                int mask = 1 << i;                              //create mask
                int masked_n = bdata[j] & mask;                 //apply maschera
                int bit = masked_n >> i;  
                printf("byte %d : bit n° %d is : %d , block %d \n", j, i, bit, c);
                c++; 
        }
        printf("\n");
    }
}
//print bitmap
void print_bitmap(char* bdata, int n_byte){
    int c = 0;
    printf("Print bit of bitmap\n");
    for(int j = 0; j < n_byte; j++){
        for(int i = 0; i < BLOCK_SIZE_BITMAP; i++){
            int mask = 1 << i;                              //create mask
            int masked_n = bdata[j] & mask;                 //apply maschera
            int bit = masked_n >> i;  
            printf("%d ", bit);
            c++; 
        }
        printf("\n");    
    }
}