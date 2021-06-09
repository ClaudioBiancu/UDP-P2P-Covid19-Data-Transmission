#regola make principale
all: peer ds

#regola make per peer
peer: peer.o ./utility/utility_peer.o
	gcc -Wall peer.o -o peer

#regola make per ds
ds: ds.o ./utility/utility_ds.o
		gcc -Wall ds.o -o ds

./utility/utility_peer.o: ./utility/utility_peer.c
	gcc  -c ./utility/utility_peer.c -o ./utility/utility_peer.o

./utility/utility_ds.o: ./utility/utility_ds.c
	gcc  -c ./utility/utility_ds.c -o ./utility/utility_ds.o

clean:
	-rm *o peer ds ./utility/*.o ./txtDS/bootedPeers.txt #./txtPeer/*.txt
