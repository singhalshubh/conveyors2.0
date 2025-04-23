## Third contribution: Programmability in Conveyors
This folder consists of a simple All-Reduce sum algorithm where every core sends a data item to all other pe's directly, from a user-perspective. This way we generate two variants

- When a core sends all data items to single core, followed by next core and so on.. (pe_data)
- When a core sends a data item to all cores, followed by next data item and so on.. (data_pe)

Naming convention follows from ordering of `for` loops in the send operation of the program.

### Installation and Execution
Makefile will generate executables for both programs.
```
make
```

### Sample Output

The first program `pe_data` incurs nearly $~2-3\times$ performance drop relative to `data_pe`. This indicates the concern with programmability since interchanging the for-loops has no effect on algorithm yet implementation suffers!

```
[system@all-reduce]$ srun -N 16 -n 256 ./pe_data -s 1000000
[Application]: All-Reduce for 1000000 
Time:   29.626 sec
OK Passed
```

```
[ssinghal74@atl1-1-02-008-9-1 all-reduce]$ srun -N 16 -n 256 ./data_pe -s 1000000
[Application]: All-Reduce for 1000000 
Time:   11.951 sec
OK Passed
```