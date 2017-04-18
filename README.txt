Student Detais:-
Abhishek Gupta - 2014A7PS026P
Harshit Jain - 2014A7PS101P

Submission Details:-


Problems chosen:-
1.) Knapsack
2.) Travelling Salesman problem


Two solutions to each of the above problem have been coded in C using OpenMP and MPI respectively.
Code files are the present in the following subfolders:-
1.) tsp_mpi
2.) tsp_openmp
3.) knapsack_mpi
4.) knapsack_openmp
The name of code file is same as of that of subfolder.

FOR MPI:-
Code files in the subfolders use blocking send and blocking receive. We modified the same code to use
non-blocking send and blocking receive as well to calculate time values. Non-blocking send and blocking
receive code is kept in EXTRA folder

FOR OpenMP:-
For the OpenMP assignment, serial code was taken from the web and changes were done on them to
parallize them using OpenMP.


For 1,2,3 optimal value of the solution and the solution has been calculated. For 4, only optimal
value has been calculated


To test the code, move the subfolder in terminal and run the following command:-
make < [testcase_file_name.in]
E.g. make < t1.in
To print the communication messages in case of MPI as use, make debug < [testase_file_name.in]


Number of threads can be changed by changing the number in "omp_set_num_threads" function call. It
is the first statement in the main function.
Number of processors in MPI can be changed by changing the argument n in the Makefile


Testing was done on test_cases taken from web as well as some randomly generated test cases.
Testcases for each problem is present is the respective subfolder


Execution times were recorded for each problem. You can find them in records.txt in the problem
subfolder.
