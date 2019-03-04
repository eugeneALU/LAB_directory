# TDDC78

The repository contains the documentation and source code of the course - Programming of Parallel Computers(TDDC78).

Parallel programming using OpenMP, MPI, Pthreads.   [webpage][1]


Running on the supercomputer - _[Triolith][2]_

## Content

There are five labs: four coding lab and one tool usage lab

*  Lab1: Image filter (MPI)
*  Lab2: Image filter (PThreads)
*  Lab3: Heat Equation (OpenMP)
*  Lab4: Particles simulation (MPI)
    * **Caution** :<br>
    while initializing the particles in different worker, need to add some worker specific number (such as: worker rank). 
    In case the **pseudo random** with same random seed in computer that will create same particles pattern in different workers

    * In code _main_parallel.c_ **Line 106**
    ```
	srand(time(NULL) + 1234 + myrank);	
    // +myrank to prevent the same pattern generate in different thread because of the pseudo random
    ```

*  Lab Tools: TotalView, parallel debugger. Code is integrated in Lab1.

[1]: https://www.ida.liu.se/~TDDC78/labs/doc/labkomp.pdf
[2]: https://www.nsc.liu.se/systems/retired/triolith/