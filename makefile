#regola make principale
all: peer discoveryserver

#regola make per peer
peer: peer.o ./utility/utility_peer.o
	gcc -Wall peer.o -o peer

#regola make per discoveryserver
discoveryserver: discoveryserver.o ./utility/utility_ds.o
		gcc -Wall discoveryserver.o -o discoveryserver

./utility/utility_peer.o: ./utility/utility_peer.c
	gcc -Wall -c ./utility/utility_peer.c -o ./utility/utility_peer.o

./utility/utility_ds.o: ./utility/utility_ds.c
	gcc -Wall -c ./utility/utility_ds.c -o ./utility/utility_ds.o

clean:
	-rm *o peer discoveryserver ./utility/*.o ./txtDS/bootedPeers.txt #./txtPeer/*.txt
