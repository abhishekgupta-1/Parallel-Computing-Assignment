all:
	gcc -g -c list.c
	mpicc -g -DDEBUG mst.c list.o -o mst
	mpirun -n 4 ./mst
