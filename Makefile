mst:
	gcc -g -c list.c
	mpicc -g -DDEBUG mst.c list.o -o mst
	make run

all:
	make debug
	make run 
nodebug:
	gcc -g -c list.c
	mpicc -g sack.c list.o -o sack
	make run 
debug:
	gcc -g -c list.c
	mpicc -g -DDEBUG sack.c list.o -o sack
run:
	mpirun -n 4 ./mst < input.txt
clean:
	rm mst
