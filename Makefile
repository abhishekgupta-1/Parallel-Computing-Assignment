all:
	make debug
	make run < input.txt
nodebug:
	gcc -g -c list.c
	mpicc -g sack.c list.o -o sack
	make run < input.txt
debug:
	gcc -g -c list.c
	mpicc -g -DDEBUG sack.c list.o -o sack
run:
	mpirun -n 4 ./sack
