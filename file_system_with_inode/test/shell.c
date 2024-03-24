#include "shell.h"

char* concat(char *dest, char *src, size_t n){
    size_t dest_len = strlen(dest);
    size_t i;
    for (i = 0 ; i < n && src[i] != '\0' ; i++) {
        dest[dest_len + i] = src[i];
        if(dest_len > 0 && dest[dest_len - 2 + i] == '>') dest[dest_len - 2 + i] = '/';
    }
    dest[dest_len + i] = '>';
    dest[dest_len + i + 1] = '\0';
    return dest;
}

void completion(const char *buf, linenoiseCompletions *lc) {
    if(buf[0] == 'm') {
        linenoiseAddCompletion(lc,"mkdir");
        linenoiseAddCompletion(lc,"mkfile");
        linenoiseAddCompletion(lc,"mem_info");
    }
    if(buf[0] == 'r'){
        linenoiseAddCompletion(lc, "rm");
    }
    if(buf[0] == 'E' || buf[0] == 'e'){
        linenoiseAddCompletion(lc, "EXIT");
    }
    if(buf[0] == 'o'){
        linenoiseAddCompletion(lc, "open");
    }
    if(buf[0] == 'l'){
        linenoiseAddCompletion(lc, "ls");
    }
    if(buf[0] == 'w') {
        linenoiseAddCompletion(lc, "write");
    }
    if(buf[0] == 'r') {
        linenoiseAddCompletion(lc, "read");
    }
    if(buf[0] == 's') {
        linenoiseAddCompletion(lc, "seek");
    }
    if(buf[0] == 'c') {
        linenoiseAddCompletion(lc, "close");
    }
}

char *hints(const char *buf, int *color, int *bold) {
    if(!strcasecmp(buf, "mkdir")){
        *color = 35;
        *bold = 0;        
        return " dir_name";
    }
    if(!strcasecmp(buf, "mkfile")){
        *color = 35;
        *bold = 0;        
        return " file_name";
    }
    if(!strcasecmp(buf, "cd")){
        *color = 35;
        *bold = 0; 
        return " dir_name";
    }
    if(!strcasecmp(buf, "rm")){
        *color = 35;
        *bold = 0; 
        return " file_name";
    }
    if(!strcasecmp(buf, "open")){
        *color = 35;
        *bold = 0; 
        return " file_name";
    }
    if(!strcasecmp(buf, "read")){
        *color = 35;
        *bold = 0; 
        return " size_to_read";
    }
    if(!strcasecmp(buf, "write")){
        *color = 35;
        *bold = 0; 
        return " text_to_write";
    }
    if(!strcasecmp(buf, "seek")){
        *color = 35;
        *bold = 0; 
        return " pos_cursor";
    }
    return NULL;
}

void write_file(FileHandle* f, DiskDriver* disk) {
    char* line;
    char* buffer_out;                    	    // buffer output read
    char* arg_buf_1;                            // buffer token line instruction
    char* arg_buf_2;                            // buffer token line param
    int ret_w, ret, k = 0;
    char input[128];
    for(int i = 0; i < 128; i++) input[i] = '\0';

    printf(CC"To"CM" write "CC"use form:"CM" write text\n");
    printf(CC"To"CM" read "CC"use form: "CM"read n_bytes"CC" to read,if"CM" n_bytes == all  "CC"read file until the end\n");
    printf("To"CM" move "CC"the seek use form: "CM"seek n_of_bytes"CC" to move the cursor to n bytes, if"CM" n_bytes == all "CC"move cursor until the end\n");
    printf("To"CM" close "CC"the file use form:"CM" close"RES"\n");
    printf(CC"To show"CM" info "CC" use:"CM" info\n");
    printf(CC"To show the"CM" index block"CC" use"CM" index"RES"\n");

    concat(input, f->fcb->fcb.name, strlen(f->fcb->fcb.name));

    linenoiseSetCompletionCallback(completion);                 //to complete words
    linenoiseSetHintsCallback(hints);                           //to add hint (suggerimento)
    linenoiseSetMultiLine(1); 
    linenoiseHistoryLoad("history.txt");

    while((line = linenoise(input)) != NULL) {
        linenoiseHistorySave("history.txt");                    // Save the history on disk.
        linenoiseHistoryAdd(line);                              // Add to the history.
        if(strlen(line) > 0){
            arg_buf_1 = strtok(line, " ");
            arg_buf_2 = strtok(NULL, "\n");

            if(!strcmp(arg_buf_1, "write")){
                if(arg_buf_2 != NULL){
                ret_w = SimpleFS_write(f, arg_buf_2, strlen(arg_buf_2));
                if(ret_w == -1) break;
                }else printf(CR"error parameter in input"RES"\n");
            }
            else if(!strcmp(arg_buf_1, "read")){
                if(arg_buf_2 != NULL){
                    if(!strcmp(arg_buf_2, "all")) k = f->fcb->fcb.size_in_bytes;
                    else k = atoi(arg_buf_2);

                    buffer_out = (char*)calloc(k, sizeof(char));
                    ret = SimpleFS_read(f, buffer_out, k);
                    if(ret == -1) break;
                    printf("\n"CG);
                    for(int i = 0; i < ret; i++){
                        printf("%c", buffer_out[i]);
                    }
                    printf(RES"\n");
                    free(buffer_out);
                }else printf(CR"error parameter in input"RES"\n");
            }
            else if(!strcmp(arg_buf_1, "seek")){
                if(arg_buf_2 != NULL){
                    if(!strcmp(arg_buf_2, "all")) k = f->fcb->fcb.size_in_bytes;
                    else k = atoi(arg_buf_2);
                    ret = SimpleFS_seek(f, k);
                }
                else printf(CR"error parameter in input"RES"\n");
            }
            else if(!strcmp(arg_buf_1, "close")){
                SimpleFS_close(f);
                free(arg_buf_1);
                break;
            }
            else if(!strcmp(arg_buf_1, "info")){
                printf(CM"size_in_bytes %d",f->fcb->fcb.size_in_bytes);
                printf("\npos_in_file %d"RES"\n", f->pos_in_file);
            }
            else if(!strcmp(arg_buf_1, "index")){
                print_index_block((FirstDirectoryBlock*)f->fcb, disk);
            }
            else printf(CR"Unreconized command"RES"\n"); 
        }
        free(line);
    }
    

    return;
}

int main(int argc, char** argv){
    char* line;                                 //input
    char* cmd_buf_1;                            //buffer to token line instruction
    char* cmd_buf_2;                            //buffer to token line param
    int ret = 0;
    linenoiseSetCompletionCallback(completion); //to complete words
    linenoiseSetHintsCallback(hints);           //to add hint (suggerimento)
    linenoiseHistoryLoad("history.txt");        //to add a history a operation done (to use arrow to have old operations)
    linenoiseSetMultiLine(1);                   //the text not continue in one line, but on more lines


    //// Iinitializing FS ///////////////////
    SimpleFS fs;
    DiskDriver disk;
    DirectoryHandle* root;

    DiskDriver_init(&disk, "myfs.txt", MEM_FS);
    printf("INITIZIALITATION DISCO DONE\n");
    root = SimpleFS_init(&fs, &disk);
    printf("INITIZIALITATION FS DONE\n");

    printf("INFO FS \n general directory: %s  contains %d file\n", root->dcb->fcb.name, root->dcb->num_entries);
    
    //////////////////////////////////////
    char prompt[500];
    for(int i = 0; i < 500; i++) prompt[i] = '\0';
    
    concat(prompt, CG"mysmpFS:"RES"/", sizeof(CG"mysmpFS:/"RES));

    //// main loop to call function
    while((line = linenoise(prompt)) != NULL) {

        linenoiseHistoryAdd(line);                              // Add to the history.
        linenoiseHistorySave("history.txt");                    // Save the history on disk.
        
        if(strlen(line) != 0){

            cmd_buf_1 = strtok(line, " ");
            cmd_buf_2 = strtok(NULL, " ");

            if(!strcmp(cmd_buf_1, "format")){
                SimpleFS_format(&fs);
                DiskDriver_readBlock(&disk, root->dcb, 0);
            }
            else if(!strcmp(cmd_buf_1, "mkdir")){
                SimpleFS_mkDir(root, cmd_buf_2);
            }
            else if(!strcmp(cmd_buf_1, "mkfile")){
                FileHandle* file = SimpleFS_createFile(root, cmd_buf_2);
                if(file != NULL){
                    free(file->fcb);
                    free(file);
                }
            }
            else if(!strcmp(cmd_buf_1, "open")){
                FileHandle* file = SimpleFS_openFile(root, cmd_buf_2);
                if(file != NULL) write_file(file, &disk);
            }
            else if(!strcmp(cmd_buf_1, "ls")){
                char** list = (char**)malloc(root->dcb->num_entries * sizeof(char*));
                for(int i = 0; i<root->dcb->num_entries; i++){
                    list[i] = (char*)malloc(128);
                }
                SimpleFS_readDir(list, root);
                for(int i = 0; i < root->dcb->num_entries; i++) 
                    printf(CY"%s"RES" : %s\n", root->dcb->fcb.name, list[i]);
                
                for(int i = 0; i < root->dcb->num_entries; i++) free(list[i]);
                free(list);
            }        
            else if(!strcmp(cmd_buf_1, "rm")){
                SimpleFS_remove(root, cmd_buf_2);
            }
            else if(!strcmp(cmd_buf_1, "cd")){
                ret = SimpleFS_changeDir(root, cmd_buf_2);
                if((ret != -1) && ((strcmp(cmd_buf_2, "..") != 0))) concat(prompt, cmd_buf_2, strlen(cmd_buf_2));
                if(strcmp(cmd_buf_2, "..") == 0){   //to remove directory from position
                    int len = strlen(prompt);
                    while(prompt[len] != '/'){
                        prompt[len] = '\0';
                        len--;
                    }
                    prompt[len] = '>';
                }
            } 
            else if(!strcmp(cmd_buf_1, "mem_info")){
                print_bitmap(disk.bitmap_data, MEM_FS/8);
            }
            else if(!strcmp(cmd_buf_1, "EXIT")){
                free(cmd_buf_1);
                break;
            } 
            else printf(CR"Unreconized command"RES"\n");
        
        free(line); 
        } 
         
    }
    
    
    free(root->directory);
    free(root->dcb);
    free(root);
    
    return 0;
}