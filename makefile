#regola make principale
all: peer discoveryserver

#regola make per peer
peer: peer.o ./utility/utility_peer.o
	gcc -Wall peer.o -o peer




#regola make per discoveryserver
discoveryserver: discoveryserver.o
		gcc -Wall discoveryserver.o -o discoveryserver

./utility/utility_peer.o: ./utility/utility_peer.c
	gcc -Wall -c ./utility/utility_peer.c -o ./utility/utility_peer.o

clean:
	-rm *o peer discoveryserver ./utility/*.o
