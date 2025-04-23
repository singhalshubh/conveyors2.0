#!/bin/bash
#SBATCH -A csc607
#SBATCH -J bale_tc
#SBATCH -o ss3.out
#SBATCH -t 0:60:00
#SBATCH -p batch
#SBATCH -S 0
#SBATCH -N 64


# Set problem size and execution paramters

# export SHMEM_SYMMETRIC_SIZE=2048M
# export SCOREP_ENABLE_PROFILING=true
# export SCOREP_ENABLE_TRACING=false

#export DATASET_PATH=/storage/scratch1/8/ssinghal74/imm-dataset

#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/storage/home/hcoda1/8/ssinghal74/p-vsarkar9-1/leveldb/build/libleveldb.a

#export OMP_NUM_THREADS=16
#export HPM_EVENT_LIST=PAPI_L2_DCA,PAPI_L2_DCM
#export LD_PRELOAD="/storage/home/hcoda1/8/ssinghal74/p-vsarkar9-1/mpitrace/src/libmpihpm.so"
#export LD_PRELOAD="/storage/home/hcoda1/8/ssinghal74/p-vsarkar9-1/mpitrace/src/libmpitrace.so"
#export LD_PRELOAD=/storage/home/hcoda1/8/ssinghal74/p-vsarkar9-1/papi/src/libpapi.so.7.0
# source ./oshmem-slurm.sh

#srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE $NODE_TYPE ./tc_set -n 800000&> debug.txt
# export SCOREP_EXPERIMENT_DIRECTORY="tc_set_rmat20_${NODES}"
# srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE $NODE_TYPE ./tc_set -f /lustre/orion/csc607/scratch/shubh/network_dataset/ActorGraphBenchmark/Graph_Serialization/medium.mtx &> debug.txt
# export SCOREP_EXPERIMENT_DIRECTORY="tc_set_erdoi_${NODES}"
# srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE $NODE_TYPE ./tc_set -n 10000 &> debug.txt

module load papi/7.1.0.4
export SCOREP_METRIC_PAPI_SEP=","
# export SCOREP_METRIC_PAPI="PAPI_TOT_CYC"
# export SCOREP_METRIC_PAPI_COMPONENTS="cray_zenl3"


# export SCOREP_VERBOSE=1
# export SCOREP_METRIC_PAPI="PAPI_BR_MSP,PAPI_BR_PRC"
#export SCOREP_METRIC_PAPI="cray_zenl3:::UNC_L3_MISS,cray_zenl3:::UNC_L3_CACHE_REQUESTS"
# export SCOREP_METRIC_PAPI="PAPI_L1_DCM,PAPI_L3_DCM,PAPI_TLB_DM,PAPI_L2_DCM"
#export SCOREP_METRIC_PAPI="PAPI_L1_DCM"
export SCOREP_METRIC_PAPI="coretemp:::craypm:memory_power"

#export DATA=100
# export MADV_NOHUGEPAGE=1
# cat /sys/kernel/mm/transparent_hugepage/enabled

#srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE $NODE_TYPE ./tc_set -f /lustre/orion/csc607/scratch/shubh/network_dataset/ActorGraphBenchmark/Graph_Serialization/small.mtx &> debug.txt
#srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE $NODE_TYPE ./ig -n 200000 -T 100000 &> debug.txt
#srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE $NODE_TYPE ../build_cray/bin/topo -n 20000 &> debug.txt
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./histo -n 20000000

# export SCOREP_VERBOSE=true

cube_dump -m visits apps_${NODES}_${CORES_PER_NODE}/profile.cubex &> debug.txt 
grep "push" debug.txt 

cube_dump -m time apps_${NODES}_${CORES_PER_NODE}/profile.cubex &> debug.txt 
grep "putp" debug.txt
scorep-score -r apps_${NODES}_${CORES_PER_NODE}/profile.cubex


export CORES_PER_NODE=24
export NODES=32
rm -rf apps_${NODES}_${CORES_PER_NODE}
export SCOREP_METRIC_PAPI="PAPI_L3_TCA"
export SCOREP_METRIC_PAPI_SEP=","
export SCOREP_FILTERING_FILE=/storage/home/hcoda1/8/ssinghal74/p-vsarkar9-1/bale/src/bale_classic/convey/scorep.filt
export SCOREP_ENABLE_TRACING=false
export SCOREP_ENABLE_PROFILING=true
export SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--user"
export SCOREP_TOTAL_MEMORY=3700MB
export SCOREP_EXPERIMENT_DIRECTORY="apps_${NODES}_${CORES_PER_NODE}"
export CORES=$(($NODES*$CORES_PER_NODE))
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./tc_set -n 100000 -M 8



export DATA=50000000
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./topo -n $(($DATA/$CORES)) -M 8
export input_file="debug_1.txt"
cube_dump -x incl -z incl -m PAPI_L3_TCA apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "convey"  &> ${input_file}
awk '{ 
    func_name = $1; 
    sum = 0; count = 0;
    for (i = 2; i <= NF; i++) {
        if ($i ~ /^[0-9.-]+$/) {  # Ignore non-numeric values
            sum += $i; 
            count++;
        }
    } 
    if (count > 0) {
        avg = sum / count;
        printf "%s: %.2f\n", func_name, avg;
    }
}' "$input_file" &> out_1.txt

export SCOREP_FILTERING_FILE=/storage/home/hcoda1/8/ssinghal74/p-vsarkar9-1/bale/src/bale_classic/convey/scorep.filt

export CORES_PER_NODE=2
export NODES=2
rm -rf apps_${NODES}_${CORES_PER_NODE}
export SCOREP_METRIC_PAPI="PAPI_L3_TCM"
export SCOREP_METRIC_PAPI_SEP=","
export SCOREP_ENABLE_TRACING=false
export SCOREP_ENABLE_PROFILING=true
export SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--user"
export SCOREP_TOTAL_MEMORY=3700MB
export SCOREP_EXPERIMENT_DIRECTORY="apps_${NODES}_${CORES_PER_NODE}"
export CORES=$(($NODES*$CORES_PER_NODE))
export DATA=100000000
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./histo -n $(($DATA/$CORES)) -M 8


# export DATA=10000000000
# srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./histo -T 10000000 -n $(($DATA/$CORES)) -M 8 &> debug.txt

# export DATA=100000000000
# srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./ig -T 10000000 -n $(($DATA/$CORES)) -M 8 &> debug_1.txt


scorep-score -r apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "porter_send_buffer"
scorep-score -r apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "standard_ready"
scorep-score -r apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "putp_scan_receipts"
scorep-score -r apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "convey_advance"

cube_dump -x incl -z incl -m PAPI_L3_TCM apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "convey"

# randperm 4000000 req/pe, TCA 
# rand_permp_conveyor(id=29): Avg = 33,066,326.12, StdDev = 7115042.68 ~ 11M
# convey_push(id=146): Avg = 7,626,012.85, StdDev = 2678181.94 ~ 7.6M ~ 22M
# convey_pull(id=158): Avg = 3,252,669.97, StdDev = 2246588.87 ~ 3.2M
# convey_advance(id=94): Avg = 11,502,035.47, StdDev = 1451185.49 ~ 11M
#

# randperm 4000000 req/pe, TCM
# rand_permp_conveyor(id=29): Avg = 16,788,775.15, StdDev = 327748.08 ~ 8.5
# convey_push(id=146): Avg = 3,548,909.43, StdDev = 493525.77 ~ 3.5M = ~ 8.2
# convey_pull(id=158): Avg = 1,249,007.37, StdDev = 317054.11  ~ 1.2M
# convey_advance(id=94): Avg = 3,493,662.66, StdDev = 337270.99 ~ 3.5M

cat debug.txt
make ig
export CORES_PER_NODE=24
export NODES=16
export SCOREP_METRIC_PAPI="PAPI_L3_TCA"
export SCOREP_METRIC_PAPI_SEP=","
rm -rf apps_${NODES}_${CORES_PER_NODE}
export SCOREP_ENABLE_TRACING=false
export SCOREP_ENABLE_PROFILING=true
export SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--user"
export SCOREP_TOTAL_MEMORY=3700MB
export SCOREP_EXPERIMENT_DIRECTORY="apps_${NODES}_${CORES_PER_NODE}"
export CORES=$(($NODES*$CORES_PER_NODE))
export DATA=4000000
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./randperm -n $DATA -M 8 &> debug.txt

scorep-score -r apps_${NODES}_${CORES_PER_NODE}/profile.cubex

scorep-score -r apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "standard_ready"
scorep-score -r apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "putp_scan_receipts"


awk '/push =/ {
    val = $3;
    push_sum += val;
    push_sum_sq += val * val;
    push_count++;
} 
/pull =/ {
    val = $3;
    pull_sum += val;
    pull_sum_sq += val * val;
    pull_count++;
} 
END {
    if (push_count > 0) {
        push_avg = push_sum / push_count;
        push_std = sqrt(push_sum_sq / push_count - push_avg^2);
        printf "Push Avg: %.2f, Push StdDev: %.2f\n", push_avg, push_std;
    }
    if (pull_count > 0) {
        pull_avg = pull_sum / pull_count;
        pull_std = sqrt(pull_sum_sq / pull_count - pull_avg^2);
        printf "Pull Avg: %.2f, Pull StdDev: %.2f\n", pull_avg, pull_std;
    }
}' debug.txt



# srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./histo -T 10000000 -n $(($DATA/$CORES)) -M 8 &> debug.txt

export CORES_PER_NODE=24
export NODES=32
export DATA=10000000000
export CORES=$(($NODES*$CORES_PER_NODE))
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./histo -T 10000000 -n $(($DATA/$CORES)) -M 1


# export DATA=/storage/home/hcoda1/8/ssinghal74/scratch/imm-dataset/com-Youtube/com-Youtube.mtx
# export SCOREP_FILTERING_FILE=/storage/home/hcoda1/8/ssinghal74/p-vsarkar9-1/conveyors2.0/bale/src/bale_classic/convey/scorep.filt

export CORES_PER_NODE=2
export NODES=2
export DATA=200000
export CONVEY_BUFFER_SIZE=1000
export SCOREP_METRIC_PAPI="PAPI_L2_DCM"
export SCOREP_METRIC_PAPI_SEP=","
export SCOREP_ENABLE_TRACING=true
export SCOREP_ENABLE_PROFILING=false
export SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--user"
export SCOREP_TOTAL_MEMORY=3700MB
export SCOREP_EXPERIMENT_DIRECTORY="apps_${NODES}_${CORES_PER_NODE}_${CONVEY_BUFFER_SIZE}"
export CORES=$(($NODES*$CORES_PER_NODE))
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./histo -n $DATA -M 8


#srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./triangles -f $DATA -M 8



export input_file="debug.txt"
cube_dump -x incl -z incl -m PAPI_L3_TCM apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "convey"  &> ${input_file}
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




export input_file="debug.txt"
cube_dump -x incl -z incl -m PAPI_L3_TCM apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "agp"  &> ${input_file}
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
export input_file="debug.txt"
cube_dump -x incl -z incl -m PAPI_L3_TCA apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "agp"  &> ${input_file}
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




export CORES_PER_NODE=24
export NODES=2
export DATA=10000000
export CORES=$(($NODES*$CORES_PER_NODE))
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./histo -T 100000 -n $DATA -M 8



export CORES_PER_NODE=24
export NODES=16
export DATA=~/scratch/gaps_dataset/GAP-web/GAP-web.mtx
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./tc_set -f $DATA -M 8



export CORES_PER_NODE=24
export NODES=2
export DATA=2000000
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./randperm_conveyor -n $DATA-M 8 &> debug.txt




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
cube_dump -x incl -z incl -m visits apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "ready"  &> ${input_file}
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


export input_file="debug.txt"
cube_dump -x incl -z incl -m PAPI_L3_TCA apps_${NODES}_${CORES_PER_NODE}/profile.cubex | grep "topo"  &> ${input_file}
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
export CORES_PER_NODE=24
export NODES=128
export SCOREP_METRIC_PAPI="PAPI_L3_TCA"
export SCOREP_METRIC_PAPI_SEP=","
rm -rf apps_${NODES}_${CORES_PER_NODE}
export SCOREP_ENABLE_TRACING=false
export SCOREP_ENABLE_PROFILING=true
export SCOREP_WRAPPER_INSTRUMENTER_FLAGS="--user"
export SCOREP_TOTAL_MEMORY=3700MB
export SCOREP_EXPERIMENT_DIRECTORY="apps_${NODES}_${CORES_PER_NODE}"
export CORES=$(($NODES*$CORES_PER_NODE))
export DATA=50000000
srun --hint=nomultithread --nodes=$NODES --exclusive --ntasks=$(($NODES*$CORES_PER_NODE)) --ntasks-per-node=$CORES_PER_NODE ./topo -n $(($DATA/$CORES)) -M 8 &> debug.txt
