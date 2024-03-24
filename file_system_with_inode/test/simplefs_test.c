#include "simplefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define TEXT1 "Ti proteggerò dalle paure delle ipocondrie Dai turbamenti che da oggi incontrerai per la tua via Dalle ingiustizie e dagli inganni del tuo tempo Dai fallimenti che per tua natura normalmente attirerai Ti solleverò dai dolori e dai tuoi sbalzi d'umore Dalle ossessioni delle tue manie Supererò le correnti gravitazionali Lo spazio e la luce per non farti invecchiare E guarirai da tutte le malattie Perché sei un essere speciale Ed io, avrò cura di te Vagavo per i campi del Tennessee Come vi ero arrivato, chissà Non hai fiori bianchi per me? Più veloci di aquile i miei sogni Attraversano il mare Ti porterò soprattutto il silenzio e la pazienza Percorreremo assieme le vie che portano all'essenza I profumi amore inebrieranno i nostri corpi  La bonaccia d'agosto non calmerà i nostri sensi Tesserò i tuoi capelli come trame di un canto Conosco le leggi del mondo, e te ne farò dono Supererò le correnti gravitazionali Lo spazio e la luce per non farti invecchiare Ti salverò da ogni malinconia Perché sei un essere speciale Ed io avrò cura di te Io sì, che avrò cura di te "
#define TEXT2 "First Law of Asimov: A robot may not injure a human being or, through inaction, allow a human being to come to harm. Second Law of Asimov: A robot must obey any orders given to it by human beings, except where such orders would conflict with the First Law. Third Law of Asimov: A robot must protect its own existence as long as such protection does not conflict with the First or Second Law. "
#define TEXT3 "Test write "

int main(int argc, char** argv){
    
    SimpleFS fs;
    DiskDriver disk;
    DirectoryHandle* root;

    DiskDriver_init(&disk, "myfs_test.txt", MEM_FS);
    printf("INITIZIALITATION DISCO DONE\n");
    root = SimpleFS_init(&fs, &disk);
    printf("INITIZIALITATION FS DONE\n");
    SimpleFS_format(&fs);
    DiskDriver_readBlock(&disk, root->dcb, 0);

    DiskDriver_flush(&disk);
    
    print_bitmap(disk.bitmap_data, MEM_FS/8);
    int ret;

    printf("INFO FS \n\nname root--> %s, is_dir %d, num_files --> %d file\n", root->dcb->fcb.name, root->dcb->fcb.is_dir, root->dcb->num_entries);
    
    printf("\nPrint ROOT Index \n");
    print_index_block(root->dcb, &disk);
    printf("\n");

    FileHandle* file;

    printf("///////////////// Loop to create 55 directories /////////////////\n");
    for(int k = 0; k<550; k++){
        char s[10];
        sprintf(s, "%d", k);
        ret = SimpleFS_mkDir(root, s);
        if(ret != -1) printf("create : %s\n", s);
    }
    printf("///////////////// Loop to create 25 files /////////////////\n");
    for(int k = 0; k<250; k++){
        char s[10];
        sprintf(s, "%d.txt", k);
        file = SimpleFS_createFile(root, s);
        if(file != NULL){
            printf("create : %s\n", s);
            SimpleFS_close(file);
        }
        
    } 
    printf("\n");
    print_index_block(root->dcb, &disk);
    printf("num entries %d\n", root->dcb->num_entries);
    
    IndexBlock* index_block = (IndexBlock*)malloc(sizeof(IndexBlock));

    printf("\ncreate 6 \n");
    file = SimpleFS_createFile(root, "6");  //there is already a directory with this name
    if(file != NULL) SimpleFS_close(file);

    printf("crete README\n");
    file = SimpleFS_createFile(root, "README");
    if(file != NULL) SimpleFS_close(file);
     
    FileHandle* file1;
    printf("\nopen README --> ");
    file1 = SimpleFS_openFile(root, "README");
    if(file1 != NULL){
        printf("is dir %d filename --> %s\n", file1->fcb->fcb.is_dir, file1->fcb->fcb.name);
        SimpleFS_close(file1);
    }
    
    printf("\nopen 0.txt\n");
    file1 = SimpleFS_openFile(root, "0.txt");
    if(file1 != NULL){
        printf("is dir %d filename --> %s\n", file1->fcb->fcb.is_dir, file1->fcb->fcb.name);
        SimpleFS_close(file1);
    }
    
    printf("\nopen 6.txt\n");
    file1 = SimpleFS_openFile(root, "6.txt");
    if(file1)printf("opened 6.txt\n");
    else printf("not opened 6.txt\n");
    
    FirstDirectoryBlock* prov = (FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));

    ret = DiskDriver_readBlock(&disk, root->dcb, 0);
    char** list1 = (char**)malloc(root->dcb->num_entries * sizeof(char*));
    for(int i = 0; i<root->dcb->num_entries; i++){
        list1[i] = (char*)malloc(128);
    } 
   
    /////////////////////////////
    printf("\n///////// ls - list dir 1 ///////// \n");
    ret = DiskDriver_readBlock(&disk, prov, 0);
    error(ret, "error test\n");
    ret = SimpleFS_readDir(list1, root);
    if(ret == -1) printf("error in SimpleFS_readDir in test\n");

    for(int i = 0; i < prov->num_entries; i++) printf(CY"%s"RES" : "RES"%s"RES"\n", prov->fcb.name, list1[i]);
    printf("\n");
    for(int i = 0; i < root->dcb->num_entries; i++) free(list1[i]);
    
    ret = DiskDriver_readBlock(&disk, prov, 3);
    error(ret, "error in DiskDriver_readBlock");

    DirectoryHandle* sec_handle = (DirectoryHandle*) malloc(sizeof(DirectoryHandle));
    sec_handle->sfs = &fs;
    sec_handle->dcb = prov;
    sec_handle->directory = root->dcb;

    printf("/// create dir in %s ///\n", sec_handle->dcb->fcb.name);

    ret = SimpleFS_mkDir(sec_handle, "paperino");
    if(ret == -1)printf("Directory paperino not create\n");
    ret = SimpleFS_mkDir(sec_handle, "pippo");
    if(ret == -1)printf("Directory pippo not create\n");
    ret = SimpleFS_mkDir(sec_handle, "pluto");
    if(ret == -1)printf("Directory pluto not create\n");
    file = SimpleFS_createFile(sec_handle, "pippo.txt");
    if(file == NULL)printf("file pippo.txt not create\n");
    else SimpleFS_close(file);

    printf("\n dirname : %s, \npos_in_disk %d,\nnum_entries : %d,\n first file block: %d\n\n", prov->fcb.name, prov->fcb.block_in_disk, prov->num_entries, prov->direct_blocks[0]);
    print_index_block(prov, &disk);

    char** list2 = (char**)malloc(sec_handle->dcb->num_entries * sizeof(char*));
    for(int i = 0; i<sec_handle->dcb->num_entries; i++){
        list2[i] = (char*)malloc(128);
    } 
    
    printf("\n///////// ls - list dir 2 /////////\n");
    ret = SimpleFS_readDir(list2, sec_handle);
    if(ret == -1) printf("error nella readDir in test\n");
    printf("\n");
    for(int i = 0; i < sec_handle->dcb->num_entries; i++) printf(CY"%s"RES" : "RES"%s"RES"\n", sec_handle->dcb->fcb.name, list2[i]);
    for(int i = 0; i < sec_handle->dcb->num_entries; i++) free(list2[i]);
    ///////////////////////////
    
    printf("\n%s --> root\n", root->dcb->fcb.name);
    FirstDirectoryBlock* block_paperino = (FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
    
    if(prov->direct_blocks[0] != -1){
        ret = DiskDriver_readBlock(&disk, block_paperino, prov->direct_blocks[0]);
        error(ret, "error in read in test\n");
        DirectoryHandle* dir = (DirectoryHandle*)calloc(1, sizeof(DirectoryHandle));
        dir->sfs = &fs;
        dir->directory = sec_handle->dcb;
        dir->dcb = block_paperino;
        
        printf("\ncreate two dir and one file in directory : %s\n", dir->dcb->fcb.name);
        ret = SimpleFS_mkDir(dir, "esame");       //add in paperino
        error(ret, "error in SimpleFS_mkDir\n");
        file = SimpleFS_createFile(dir, "voti.txt");
        SimpleFS_close(file);
        ret = SimpleFS_mkDir(dir, "ricevute");
        error(ret, "error in SimpleFS_mkDir\n");
        print_index_block(block_paperino, &disk);
    
        free(dir);
    }

    ret = SimpleFS_changeDir(sec_handle, "paperino");   //works with paperino
    if(ret == -1) printf("errore nella changeDir\n");
    else{
        printf("changedir from 2 to : %s ", sec_handle->dcb->fcb.name);
        if(sec_handle->directory != NULL) printf("->parent : %s\n",  sec_handle->directory->fcb.name);
        else printf(" /root\n");
    }
       
    ret = SimpleFS_changeDir(sec_handle, "..");   //works ..
    if(ret == -1) printf("errore nella changeDir\n");
    else{
        printf("\ncd .. --> return in : %s ", sec_handle->dcb->fcb.name);
        if(sec_handle->directory != NULL) printf("->parent : %s\n",  sec_handle->directory->fcb.name);
        else printf(" /root\n"); 
    }

    ret = SimpleFS_changeDir(sec_handle, "bollettini");   //works, not changed
    if(ret != -1){
        printf("\ncd  %s ", sec_handle->dcb->fcb.name);
        if(sec_handle->directory != NULL) printf("parent : %s\n",  sec_handle->directory->fcb.name);
        else printf("/root \n");
    } 

    printf("\n///// Remove  /////");
    ret = SimpleFS_remove(root, "10.txt");
    if(ret != 0){
        printf("\nerror in rm 10.txt\n");
    }
    else printf("\nremoved 10.txt\n"); 
    
    ret = SimpleFS_remove(root, "100");
    if(ret != 0){
        printf("\nerror in rm 100\n");
    }
    else printf("\nremoved 100\n");
    
    ret = SimpleFS_remove(root, "2");
    if(ret != 0){
        printf("\nerror in rm 2\n");
    }
    else printf("\nremoved 2\n"); 

    print_bitmap_bit(disk.bitmap_data, MEM_FS/8);
    printf("/// remove files from 50.txt to 150.txt ///\n");
    for(int k = 50; k<150; k++){
        char s[10];
        sprintf(s, "%d.txt", k);
        ret = SimpleFS_remove(root, s);
        if(ret == -1) {
            printf("errore nella rimozione : %d.txt\n", k);
        }
    }
    printf("/// remove directories from 0 to 25 ///\n");
    for(int k = 0; k<25; k++){
        char s[10];
        sprintf(s, "%d", k);
        ret = SimpleFS_remove(root, s);
        if(ret == -1) {
            printf("errore nella rimozione : %d\n", k);
        }
    }

    /////////////////////////////
    
    ret = DiskDriver_readBlock(&disk, prov, 0);
    char** list3 = (char**)malloc(prov->num_entries * sizeof(char*));
    for(int i = 0; i<root->dcb->num_entries; i++){
        list3[i] = (char*)malloc(128);
    } 
    
    printf("\n\n ls - list dir 3\n\n");
    ret = SimpleFS_readDir(list3, root);
    if(ret == -1) printf("errore nella readDir in test\n");
    for(int i = 0; i < prov->num_entries; i++){
        printf(CY"%s"RES" : "RES"%s"RES"\n", prov->fcb.name, list3[i]);
    }
    for(int i = 0; i < prov->num_entries; i++) free(list3[i]);
    free(list3);

    ///////////////////
    printf("Open file\n");
    file = SimpleFS_openFile(root, "20.txt");
    if(file != NULL) {
        printf("file 20.txt exists, open it\n");
        SimpleFS_close(file);
    }
    printf("Open and create file \n");
    file = SimpleFS_openFile(root, "85.txt");
    if(file != NULL){
        printf("file 85.txt exists, open it\n");
        SimpleFS_close(file);
    } 
    else printf("return null, error, there is not empty block\n");
    

    printf("\n\n////////////////////// WRITE and READ a FILE //////////////////////\n\n");
    char buffer[1100] = TEXT1;
    char buffer1[500] = TEXT2;
    char buffer2[50] = TEXT3;
    char bufferout[5000];
    if(file1 != NULL){
    printf("filename %s\n", file1->fcb->fcb.name);
    printf("\nTest - WRITE 1 - write in empty file to 2184 bytes\n");
    printf("write of %ld\n", sizeof(TEXT1)-1);
    ret = SimpleFS_write(file1, &buffer, sizeof(TEXT1)-1);
    if(ret == sizeof(TEXT1)-1)printf("first write OK\n");

    //ret = DiskDriver_readBlock(file1->sfs->disk, file1->fcb, file1->fcb->fcb.block_in_disk);
    ret = SimpleFS_seek(file1, 1092);
    ret = SimpleFS_write(file1, &buffer, sizeof(TEXT1)-1);
    printf("write of %ld\n", sizeof(TEXT1)-1);
    if(ret == sizeof(TEXT1)-1)printf("second write OK\n");
    
    printf("\nINDEX BLOCK FILE : \n");
    for(int j = 0; j < DIRECT_BLOCKS_NUM; j++)
        printf("%d ", file1->fcb->direct_blocks[j]);   
 
    printf("\n");
    //////////////
    printf("\nTest - READ 1 - read from pos->in_file 1092 to EOF\n");
    ret = SimpleFS_read(file1, &bufferout, 2184);
    printf("read size %d\n", ret);
    for(int i = 0; i < ret; i++) printf("%c", bufferout[i]);
    printf("\n\n");

    ///////////
    ret = DiskDriver_readBlock(file1->sfs->disk, file1->fcb, file1->fcb->fcb.block_in_disk);
    printf("\nTest - WRITE 2 - write in append to Law of Asimov \n");
    ret = SimpleFS_write(file1, &buffer1, sizeof(TEXT2)-1);
    error(ret, "error in write in test\n");
    ret = DiskDriver_readBlock(file1->sfs->disk, file1->fcb, file1->fcb->fcb.block_in_disk); 
    error(ret, "error in readblock in test \n");
    printf("\nINDEX BLOCK FILE : \n");
    for(int j = 0; j < DIRECT_BLOCKS_NUM; j++)
        printf("%d ", file1->fcb->direct_blocks[j]);   
 
    printf("\n");   

    printf("\nTest - Write in overwrite - \n");
    ret = SimpleFS_write(file1, &buffer2, sizeof(TEXT3)-1);
    printf("\nTest -  Read - \n");
    ret = SimpleFS_read(file1, &bufferout, 2050);
    printf("read_size --> %d\n", ret);
    for(int i = 0; i < ret; i++){
        printf("%c", bufferout[i]);
    } 
    printf("\n\n");
    ret = SimpleFS_write(file1, &buffer1, sizeof(TEXT2)-1);
    ret = DiskDriver_readBlock(file1->sfs->disk, file1->fcb, file1->fcb->fcb.block_in_disk);
    printf("written bytes : %d\n", file1->fcb->fcb.size_in_bytes);

    ret = SimpleFS_read(file1, &bufferout, 892);
    printf("read_size --> %d\n", ret);
    for(int i = 0; i < ret; i++) printf("%c", bufferout[i]);
    printf("\n\n");

    print_index_block((FirstDirectoryBlock*)file1->fcb, &disk);
    
    printf("TEST - WRITE and READ all file \n");

    ret = SimpleFS_seek(file1, file1->fcb->fcb.size_in_bytes);
    error(ret, "error in SimpleFS_seek in test\n");
    ret = SimpleFS_write(file1, &buffer, sizeof(TEXT1)-1);
    error(ret, "error in SimpleFS_write in test\n");
    ret = SimpleFS_seek(file1, file1->fcb->fcb.size_in_bytes);
    error(ret, "error in SimpleFS_seek in test\n");
    ret = SimpleFS_write(file1, &buffer, sizeof(TEXT1)-1);
    error(ret, "error in SimpleFS_write in test\n");
    ret = SimpleFS_seek(file1, file1->fcb->fcb.size_in_bytes);
    error(ret, "error in SimpleFS_seek in test\n");
    ret = SimpleFS_write(file1, &buffer, sizeof(TEXT1)-1);
    error(ret, "error in SimpleFS_write in test\n");
    ret = SimpleFS_seek(file1, file1->fcb->fcb.size_in_bytes);
    error(ret, "error in SimpleFS_seek in test\n");
    ret = SimpleFS_write(file1, &buffer, sizeof(TEXT1)-1);
    error(ret, "error in SimpleFS_write in test\n");
    ret = SimpleFS_seek(file1, file1->fcb->fcb.size_in_bytes);
    error(ret, "error in SimpleFS_seek in test\n");
    ret = SimpleFS_write(file1, &buffer, sizeof(TEXT1)-1);
    error(ret, "error in SimpleFS_write in test\n");
    ret = SimpleFS_seek(file1, file1->fcb->fcb.size_in_bytes);
    error(ret, "error in SimpleFS_seek in test\n");
    ret = SimpleFS_write(file1, &buffer, sizeof(TEXT1)-1);
    error(ret, "error in SimpleFS_write in test\n");
    ret = SimpleFS_seek(file1, file1->fcb->fcb.size_in_bytes);
    error(ret, "error in SimpleFS_seek in test\n");
    ret = SimpleFS_write(file1, &buffer, sizeof(TEXT1)-1); 
    error(ret, "error in SimpleFS_write in test\n");
    ret = SimpleFS_seek(file1, 0);
    error(ret, "error in SimpleFS_seek in test\n");
    char* buffer_out = (char*)calloc(file1->fcb->fcb.size_in_bytes, sizeof(char));
    ret = SimpleFS_read(file1, buffer_out, file1->fcb->fcb.size_in_bytes);
    error(ret, "error in SimpleFS_read in test\n");
    for(int i = 0; i < ret; i++)
        printf("%c", buffer_out[i]);
    printf("\n");

    print_index_block((FirstDirectoryBlock*)file1->fcb, &disk);

    ret = DiskDriver_readBlock(&disk, root->dcb, 0);
    error(ret, "error in DiskDrvier_readBlock in test\n");       
    SimpleFS_close(file1);
    free(buffer_out);
    }

    ret = SimpleFS_remove(root, "2");
    if(ret != 0){
        printf("\nerror in rm 2\n");
    }
    else printf("\nremoved 2\n");

    ret = SimpleFS_remove(root, "6.txt");
    if(ret != 0)printf("\nerror remove di 6.txt\n");
    else printf("\nremoved 6.txt\n");  
    printf("\n/// PRINT INDEX BLOCK ROOT  ///\n");
    print_index_block(root->dcb, &disk);
    
    //ret = DiskDriver_readBlock(&disk, root->dcb, 0);
    printf("num_entries %d\n", root->dcb->num_entries); 
    
    
    free(list1);
    free(list2);
    free(index_block);
    
    free(sec_handle->dcb);
    free(sec_handle->directory);
    free(sec_handle);
    free(block_paperino);
    free(root->dcb);
    free(root->directory); 
    free(root);

    return 0;
}