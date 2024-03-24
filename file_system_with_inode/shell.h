#include "simplefs.h"
#include "linenoise.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h> 

char* concat(char *dest, char *src, size_t n);
void completion(const char *buf, linenoiseCompletions *lc);
char *hints(const char *buf, int *color, int *bold);
void write_file(FileHandle* f, DiskDriver* disk);