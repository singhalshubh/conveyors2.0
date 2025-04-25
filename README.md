# Analysis of Small-Message Aggregation for state-of-art: (I) Conveyors
In this document, we will explain the reproduction of several figures of the paper. First, we will explain the tree structure and the installation steps. Then, we explain step-by-step procedure on how to achieve all three technical contributions of the paper.

## Contributions
- $\texttt{Instrumentation and Execution Tracing}$ using `Vampir`, `PAPI` and `score-p` in `bale` folder.
- Comparison between $\texttt{Termination}$ of conveyors and classical barriers offered by `SHMEM` and `MPI` in `termination` folder.
- $\texttt{Programmability}$ contribution in `all-reduce` folder.

## Requirements
For installing conveyors and analysing our contributions, stated modules are required - `gcc`, `python3`, `SHMEM` and `MPI`. Further we use tools - `score-p`, `PAPI` and `Vampir` for analysis. We recommend running this setup for clusters with high network bandwidth interconnects, such as InfiniBand, Slingshot, Tofu etc.

## Structure
```tree
.
├── bale (additional to official repository https://github.com/jdevinney/bale)
│   └── src
│       └── bale_classic
│           ├── apps
│           │   ├── histo.cpp
│           │   ├── ig.cpp
│           │   ├── Makefile
│           │   ├── randperm.cpp
│           │   ├── run.sh
│           │   ├── topo.cpp
│           │   └── triangles.cpp
│           ├── convey
│           │   ├── scorep.filt
└── termination
│   ├── main_01.cpp
│   ├── Makefile
│   ├── README.md
│   ├── theory.ipynb
│   ├── theory-termination.png
│   └── utility.h
├── all-reduce
│   ├── data_pe.cpp
│   ├── Makefile
│   ├── pe_data.cpp
│   └── README.md
├── convey-setup.sh
├── LICENSE
├── README.md
```


## Installation
Deployment and all dependent modules are loaded via a simple script which compiles the conveyors code and builds a library which can be linked at run-time. The default name for the build is `build_oshmem`. For enabling `score-p`, use `scorep-oshcc` and `scorep-oshcxx` for compiling apps. 
Setup the repository for deriving three conclusions. The first conclusion involves using both official bale repo and our repository, in particular `apps` folder in `conveyors_prof/bale/src/bale_classic/`.
```
git clone https://github.com/singhalshubh/conveyors_prof
git checkout sc25
cd conveyors_prof/
source convey-setup.sh
cd bale/src/bale_classic/apps
make
cd ../../../..
```

Replace conveyors library with our link provided for running official repository.
```
git clone https://github.com/jdevinney/bale
```

> For last two contributions, please refer to the `README.md` inside `termination` and `all-reduce` folder. We explain the first contribution here with steps for each figure. 

## First Contribution: Framework
We first note the weak scaling and strong scaling experiments for Histogram, Index Gather, Random Permutation, Topological Sort, Triangle Counting

We denote `$BALE` as `./bale/src/bale_classic/build_oshmem/bin/` for official `bale` kernels. For reproducing triangle counting use `./bale/src/bale_classic/apps/` instead, since this algorithm is efficient in the category of Wedge-Vertice approach.

> `n` stays the input-parameter for every application, with `-M` indicating model (1 for AGP and 8 for Conveyors).

### Weak Scaling
We showcase sample input commands for histogram application with conveyors with $n=$ 10M requests/core and $m=$ 10M rows/core.

```
export CORES_PER_NODE=24
export NODES=32
export DATA=10000000
export CORES=$(($NODES*$CORES_PER_NODE))
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./$BALE/histo -T 10000000 -n $DATA -M 8
```

$\texttt{Conclusion}:$ Effect of topology change; Within a given topology, time stays flat.

### Strong Scaling
We showcase sample input commands for histogram application with $n=$ 10B requests in total and $m=$ 10M rows/core on 32 nodes with ppn=24.

```
export CORES_PER_NODE=24
export NODES=32
export DATA=10000000000
export CORES=$(($NODES*$CORES_PER_NODE))
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./$BALE/histo -T 10000000 -n $(($DATA/$CORES)) -M 8
```
$\texttt{Conclusion}:$ Effect of topology change; Within a given topology, time scales linearly, until the algorithm reaches strong scaling limit, after-which one can observe overheads incurred purely by conveyors and system beneath.

### Instrumentation of Memory and Network
We first use a function filter file located in `bale/src/bale_classic/convey/scorep.filt`, which allows user to record specific portions of the program visa function names. We describe the interfaces for algo $\rightarrow$ conveyors and conveyors $\rightarrow$ network. We assume $\texttt{aD}$ topology, where a is {1,2,3}.

We bifurcate and analyse slices of execution of the program using network+polling+memory as primary resources from a hardware standpoint.

- algo $\rightarrow$ conveyors
    -  `convey_push`: User-algorithm (send packets) + `append packets` + `local_send buffer`. It sends only when a buffer slot for a destination core is full.
    -  `convey_pull`: User-algorithm (receive packets) + `pull packets` + `put_ACK signal`
    -  `convey_advance`: `pull+append packets` + `send buffer` + `put_ACK signal`. Note that here `send` is `non_blocking` followed by `quiet` after both levels (double buffering) of buffer slots for a core is filled entirely, restricted for column member cores. Rest for other axes in 3D (z-), it will employ `local` send.

- conveyors $\rightarrow$ network
    -  (Network) `local_send`: `memcpy` for data and `shmem_put64` for signal.
    -  (Network) `non_blocking_send`: `shmem_putmem_nbi` for data.
    -  (Network) `non_blocking_progress`: checks for `shmem_quiet` and if so, followed by `shmem_put64` for signal.
    -  (Network) `putp_return`: Sends acknowledgment `put_ACK signal` back to sender and update the flow-control related metadata. 
    -  (Polling, pull) `putp_scan_receipts`: Polls the incoming signal buffers in receive side.
    -  (Polling, push) `standard_ready`: Checks if the push or append is feasible. If not it returns the control back to user-algorithm.

We record the execution time of each function listed individually by mentioning the names in `scorep.filt` and running:

```
export CORES_PER_NODE=24
export NODES=32
export DATA=10000000000
rm -rf apps_${NODES}_${CORES_PER_NODE}
export SCOREP_FILTERING_FILE=$PWD/src/bale_classic/convey/scorep.filt
export SCOREP_ENABLE_TRACING=false
export SCOREP_ENABLE_PROFILING=true
export SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--user"
export SCOREP_TOTAL_MEMORY=3700MB
export SCOREP_EXPERIMENT_DIRECTORY="apps_${NODES}_${CORES_PER_NODE}"
export CORES=$(($NODES*$CORES_PER_NODE))
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./$BALE/histo -T 10000000 -n $(($DATA/$CORES)) -M 8
```

$\texttt{Conclusion}:$ Network and Polling both constitute about $\sim 10\%$ of the total time. With intra-node optimization, i.e. `memcpy` in `local_send`, total % rises to 20-30%. This factors the bottleneck to be in User-algorithm +  `append packets` +  `pull packets`, both being memory bound. Further, overlap between computation and communication is caluclated via ablation studies between asynchronous vs synchronous version. We highlight a simple change of `shmem_putmem_nbi` to `shmem_putmem` and commenting out `shmem_quiet`, and re-run the same weak scaling and strong scaling experiments. We conclude that the difference is $<$1%, allowing us to continue further.  


#### Memory as a bottleneck
We use `PAPI` counters for measuring total misses and access. We further use `filt` for individually measuring cache contribution by conveyors by using `convey_pull` + `convey_push` + `convey_advance`. First, we showcase how to collect the counters for conveyors version. Then we will showcase how to run `AGP` version. 

```
export CORES_PER_NODE=24
export NODES=32
export DATA=10000000000
rm -rf apps_${NODES}_${CORES_PER_NODE}
export SCOREP_METRIC_PAPI="PAPI_L3_TCA,PAPI_L3_TCM"
export SCOREP_METRIC_PAPI_SEP=","
export SCOREP_FILTERING_FILE=$PWD/src/bale_classic/convey/scorep.filt
export SCOREP_ENABLE_TRACING=false
export SCOREP_ENABLE_PROFILING=true
export SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--user"
export SCOREP_TOTAL_MEMORY=3700MB
export SCOREP_EXPERIMENT_DIRECTORY="apps_${NODES}_${CORES_PER_NODE}"
export CORES=$(($NODES*$CORES_PER_NODE))
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./$BALE/histo -T 10000000 -n $(($DATA/$CORES)) -M 8
```

`AGP` version involves using different model parameter. 

```
export CORES_PER_NODE=24
export NODES=32
export DATA=10000000000
rm -rf apps_${NODES}_${CORES_PER_NODE}
export SCOREP_METRIC_PAPI="PAPI_L3_TCA,PAPI_L3_TCM"
export SCOREP_METRIC_PAPI_SEP=","
export SCOREP_FILTERING_FILE=$PWD/src/bale_classic/convey/scorep.filt
export SCOREP_ENABLE_TRACING=false
export SCOREP_ENABLE_PROFILING=true
export SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--user"
export SCOREP_TOTAL_MEMORY=3700MB
export SCOREP_EXPERIMENT_DIRECTORY="apps_${NODES}_${CORES_PER_NODE}"
export CORES=$(($NODES*$CORES_PER_NODE))
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./$BALE/histo -T 10000000 -n $(($DATA/$CORES)) -M 1
```

> For visualizing the traces, we use `Vampir` official licence. Use `export SCOREP_ENABLE_TRACING=true` and `export SCOREP_ENABLE_PROFILING=false`. We recommend running it for fewer cores since trace `.otf2` files can span 10GBs with just 16 cores for the data scales under observation.

$\texttt{Conclusion}:$ For large scales of user-data on scale, observe upto 25-30% cache miss rate L3 for AGP; vs conveyors reaching peaks 60-77%. A suprise since conveyors improved memory footprint thereotically by cube root growth of the number of cores. Yet, it faces memory as a challenge and intra-node optimizations leads to network efficiency in the three-hop design proposed first by YGM and TRAM.


#### Analysing score-p profiles
Change `grep "convey"` and `-m PAPI_L3_TCA` according to the functions profiled for.

```
export input_file="debug.txt"
cube_dump -x incl -z incl -m PAPI_L3_TCA apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "convey"  &> ${input_file}
awk '{ 
    func_name = $1; 
    sum = 0; count = 0; sum_sq = 0;
    for (i = 2; i <= NF; i++) {
        if ($i ~ /^[0-9.-]+$/) {  # Check for numeric values
            sum += $i;
            sum_sq += ($i)^2;
            count++;
        }
    } 
    if (count > 0) {
        avg = sum / count;
        variance = (sum_sq / count) - (avg)^2;
        stddev = (variance >= 0) ? sqrt(variance) : 0; # Avoid negative sqrt
        printf "%s: Avg = %.2f, StdDev = %.2f\n", func_name, avg, stddev;
    }
}' "$input_file" &> out_1.txt
cat out_1.txt
```

# Highlights
![Memory interference costs relative to ground truth with no aggregation](final_result_c1.png)
Please reference the detailed analysis and traces in the paper to see how we conclude and reach to this simple and effective demomstration of the bottleneck - memory interference costs, due to interleaving patterns of access of memory of aggregation buffers and user data.

### Discussion
- SOTA library, conveyors reveal that with upcoming trends of uniform data distributions and run-time complexities of new algorithms, memory interference will potentially be "next plausible" candidate of bottleneck! To this day, none of the papers report this as a problem, since network costs have always been found to dominate, as a general notion. We promote usage of near-memory specialised devices with frugal cost of adoption and opportunity to hide device latency.
- Second, for standardization, separation of polling from the network and bifurcating profile as network and memory dominant parts or functions of the entire program are two key factors. Further dig-in for individual profiles for precise understanding is essential. We showcase the effectiveness and simplicity of model and framework. We need more such transparent models for creating a co-design space for aggregation sub-systems.
- Third, problems lying with termination on scale and programmability are revealed, thereby guiding the HPC community on the performant decisions one should take moving forward. 