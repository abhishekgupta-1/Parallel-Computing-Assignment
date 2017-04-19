all:
	gcc -c list.c
	mpicc knapsack_mpi.c list.o -o knapsack_mpi
	mpirun -n 4 ./knapsack_mpi
debug:
	gcc -c list.c
	mpicc -DDEBUG knapsack_mpi.c list.o -o knapsack_mpi
	mpirun -n 4 ./knapsack_mpi
	
