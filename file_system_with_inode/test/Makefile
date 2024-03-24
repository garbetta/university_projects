CCOPTS= -Wall -g -std=gnu99 -Wstrict-prototypes
LIBS=
CC=gcc
AR=ar

#BINS= disk_driver

#OBJS = bitmap.c bitmap_test.c disk_driver.c

HEADERS=bitmap.h\
	disk_driver.h\
	aux_fun.h\
	simplefs.h\
	linenoise.h\
	shell.h


test_bitmap: bitmap.c  bitmap_test.c $(HEADERS)
	$(CC) $(CCOPTS) bitmap.c bitmap_test.c -o test_bitmap

test_disk_driver: bitmap.c disk_driver.c disk_test.c $(HEADERS)
	$(CC) $(CCOPTS) bitmap.c disk_driver.c disk_test.c -o test_disk_driver

test_simpleFS: bitmap.c disk_driver.c aux_fun.c simplefs.c simplefs_test.c $(HEADERS)
	$(CC) $(CCOPTS) bitmap.c disk_driver.c aux_fun.c simplefs.c simplefs_test.c -o test_simpleFS

shell: bitmap.c disk_driver.c aux_fun.c simplefs.c linenoise.c shell.c $(HEADERS)
	$(CC) $(CCOPTS) bitmap.c disk_driver.c aux_fun.c simplefs.c linenoise.c shell.c -o shell


#%.o:	%.c $(HEADERS)
#	$(CC) $(CCOPTS) -c -o $@  $<

.phony: clean 


#all:  $(BINS) 
#bitmap.c bitmap_test.c disk_driver.c
#so_game: disk_test.c $(OBJS) 
	#$(CC) $(CCOPTS)  -o $@ $^ $(LIBS)

clean:
	rm -rf *.o *~  $(BINS)
