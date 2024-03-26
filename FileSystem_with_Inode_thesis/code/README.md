# Simple File System with Inode
Il progetto implementa un File System "semplificato" con Inode. Il File System si interfaccia con un disco simulato da un file.

### Bitmap
La Bitmap è un vettore che tiene riferimento dello stato di memoria dei blocchi del disco. Ogni bit rappresenta un blocco, se 0 il blocco è libero, se 1 è occupato. Vengono implementate tutte le operazioni di gestione dei bit.

### Disk Driver
L'inizializzazione del DiskDriver crea il file, per la simulazione del disco, con lo spazio richiesto ed inizializza la bitmap. La bitmap è salvata ad inizio file.
Inoltre sono implementate le operazioni di lettura/scrittura dei blocchi del disco. La scrittura si occuperà della gestione della bitmap, e i controlli fanno in modo che non ci siano curruzioni della memoria.

### File System
Il File System viene inizializzato con la creazione della directory principale, la root.
Ci sono due tipi di blocchi, indici e data. I blocchi indici contengono vettori con gli indici dei blocchi dove sono salvati i dati del file al quale si riferiscono. I blocchi data contengono le informazioni e i dati del file.
Vengono implementate tutte le operazioni di creazione/rimozione di file e cartelle, di navigazione, e di lettura/scrittura.

### Shell
La Shell permette un'interfaccia con il client per la simulazione del FileSystem. 

## Come eseguire
```
	make shell 
	./shell				#shell
   nella cartella test
	make test_bitmap
	./test_bitmap			# test bitmap
	make test_disk_driver
	./test_disk_driver		# test disk driver
	make test_simpleFS
    	./test_simpleFS     		# test file system
```

## Autore
**Alessandro Garbetta** - [Garbetta](https://gitlab.com/Garbetta)