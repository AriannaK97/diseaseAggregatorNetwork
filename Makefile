CC	=	gcc
FLAGS   =	-Wall	-g	-c	-D_POSIX_C_SOURCE=199309L

all: master diseaseMonitor_worker whoClient whoServer

master:   master.o  diseaseAggregator.o list_lib.o	 data_io.o   redBlackTree.o  command_lib.o   hashTable.o	communication.o
	$(CC)   -o master master.o diseaseAggregator.o list_lib.o	 data_io.o   redBlackTree.o  command_lib.o   hashTable.o	communication.o

diseaseMonitor_worker:  worker.o data_io.o   redBlackTree.o  command_lib.o   hashTable.o list_lib.o	diseaseAggregator.o	communication.o
	$(CC)   -o	diseaseMonitor_worker	worker.o data_io.o   redBlackTree.o  command_lib.o   hashTable.o list_lib.o	diseaseAggregator.o	communication.o
	$(CC)   -o	diseaseMonitor_worker	worker.o data_io.o   redBlackTree.o  command_lib.o   hashTable.o list_lib.o	diseaseAggregator.o	communication.o

whoClient: whoClient.o
	$(CC)	-o	whoClient	whoClient.o

whoServer: whoServer.o	serverIO.o
	$(CC)	-o whoServer	whoServer.o	serverIO.o	-lpthread

whoServer.o: src/whoServer/whoServer.c
	$(CC) $(FLAGS)	src/whoServer/whoServer.c

serverIO.o: src/whoServer/serverIO.c
	$(CC)	$(FLAGS)	src/whoServer/serverIO.c

whoClient.o:	src/whoClient/whoClient.c
	$(CC)	$(FLAGS)	src/whoClient/whoClient.c

list_lib.o:   src/worker/list_lib.c
	$(CC)	$(FLAGS)	src/worker/list_lib.c

data_io.o:    src/worker/data_io.c
	$(CC)	$(FLAGS)	src/worker/data_io.c

hashTable.o:    src/worker/hashTable.c
	$(CC)	$(FLAGS)	src/worker/hashTable.c

redBlackTree.o:    src/worker/redBlackTree.c
	$(CC)	$(FLAGS)	src/worker/redBlackTree.c

command_lib.o:    src/worker/command_lib.c
	$(CC)	$(FLAGS)	src/worker/command_lib.c

diseaseAggregator.o:    src/master/diseaseAggregator.c
	$(CC)	$(FLAGS)	src/master/diseaseAggregator.c

master.o:    src/master/master.c
	$(CC)	$(FLAGS)	src/master/master.c

worker.o: src/worker/worker.c
	$(CC)	$(FLAGS)	src/worker/worker.c

communication.o:	src/master/communication.c
	$(CC)	$(FLAGS)	src/master/communication.c

clean:
	rm	-rf *.o diseaseMonitor_worker master whoClient whoServer
	rm -rf worker*
	rm -rf aggregator_server
	rm -rf log_file.*

count:
	wc	$(SOURCE)	$(HEADER)