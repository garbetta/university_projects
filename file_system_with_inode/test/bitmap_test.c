#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_BITS 64

int main(int argc, char **argv){

    BitMap* bmap = (BitMap*)malloc(sizeof(BitMap));

    bmap->num_bits = NUM_BITS;

    int n_entries = bmap->num_bits/BLOCK_SIZE_BITMAP;
    printf("\nBITMAP BITS %d, BITMAP BYTES %d \n",bmap->num_bits, n_entries);

    char* data_entries = (char*)malloc(n_entries*sizeof(char));

    memset(data_entries, 0, n_entries);

    bmap->entries = data_entries;

    printf("\nEmpty bitmap \n");
    print_bitmap_byte(bmap->entries, n_entries);
    
    BitMapEntryKey entry;
    
    int num = 50;
    entry = BitMap_blockToIndex(num);
    printf("\nBlock to index --> bit num: %d,  byte-> %d, offset-> %d\n", num, entry.entry_num, entry.bit_num);
    int ret = BitMap_indexToBlock(entry.entry_num, entry.bit_num);
    if(ret == num) printf("Index to Block --> %d\n", ret);
    else printf("error in conversion\n");

    bmap->entries[7] = 15;
    printf("\nset a last byte \nbyte 7-th stores value %d\n\n", bmap->entries[7]);

    printf("set bit 0-th to 1");
    BitMap_set(bmap, 0, 1);
    printf("\n");
    printf("set bit 25-th to 1");
    BitMap_set(bmap, 25, 1);
    printf("\n");
    printf("set bit 50-th to 1");
    BitMap_set(bmap, 50, 1);
    printf("\n");
    printf("set bit 60-th to 1");
    BitMap_set(bmap, 60, 1);
    printf("\n\n");
    printf("il byte 7 contiene il valore %d\n", bmap->entries[7]);
    printf("\n");
    
    print_bitmap_byte(bmap->entries, n_entries);    //print arry bitmap
    print_bitmap_bit(bmap->entries, n_entries);     //print bitmap
    
    printf("get first bit with status 1 from first bit --> ");
    ret = BitMap_get(bmap, 0, 1);   
    printf("%d\n", ret);
    printf("get frist bit eith status 0 from first bit --> ");
    ret = BitMap_get(bmap, 0, 0);
    printf("%d\n", ret);

    printf("get first bit with status 1 from 26-th bit --> ");
    ret = BitMap_get(bmap, 26, 1);   
    printf("%d\n", ret);
    printf("get frist bit eith status 0 from 26-th bit --> ");
    ret = BitMap_get(bmap, 26, 0);
    printf("%d\n", ret);


    printf("\nset bit 57-th to 0");
    BitMap_set(bmap, 57, 0);
    printf("\n");
    printf("set bit 58-th to 0\n\n");
    BitMap_set(bmap, 58, 0);

    print_bitmap_bit(bmap->entries, n_entries);

    printf("byte 7-th stores value %d\n", bmap->entries[7]);

    free(bmap->entries);
    free(bmap);
    return 0;
}
