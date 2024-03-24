CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes
LIBS= 
CC=gcc
AR=ar


#BINS= simplefs_test

#OBJS = #add here your object files //bitmap.c

HEADERS=bitmap.h\
	disk_driver.h\
	aux_fun.h\
	simplefs.h\
	linenoise.h\
	shell.h


shell: bitmap.c disk_driver.c aux_fun.c simplefs.c linenoise.c shell.c $(HEADERS)
	$(CC) $(CCOPTS) bitmap.c disk_driver.c aux_fun.c simplefs.c linenoise.c shell.c -o shell

#%.o:	%.c $(HEADERS)
#	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean all


all:	$(BINS) 

so_game: simplefs_test.c $(OBJS) 
	$(CC) $(CCOPTS)  -o $@ $^ $(LIBS)

clean:
	rm -rf *.o *~  $(BINS)
