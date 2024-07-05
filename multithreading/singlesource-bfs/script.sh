#!/bin/bash
#SBATCH --job-name=actor-1
#SBATCH -A gts-vsarkar9-forza
#SBATCH -N64
#SBATCH --ntasks-per-node=24
#SBATCH -t90
#SBATCH -qinferno        
#SBATCH -ojob.out

echo "Started on `/bin/hostname`"   # prints name of compute node job was started on
cd $SLURM_SUBMIT_DIR 

workers=(1 2 4 6 12 18 24)

nodes=$1
scale=$2
graph_no=$3
for w in ${workers[@]}
do
    #HCLIB_WORKERS=$w srun -N 16 -n $((256/$w)) ./main_02 -s 16 -d 100 -c $c -g 10 -o $PWD/bin/a$c-$w.txt
    for t in 0 1 2 3 4;
    do
        HCLIB_WORKERS=$w srun -N $nodes -n $((24*$nodes/$w)) ./main_02 -s $scale -d 30 -g $graph_no -o $PWD/bin/a$w-$nodes.txt
    done

done
for t in 0 1 2 3 4;
    do
        HCLIB_WORKERS=1 srun -N $nodes -n $((24*$nodes)) ./main_03 -s $scale -d 30 -g $graph_no -o $PWD/bin/b-$nodes.txt
    done

# for nodes in 8 16 32 64;
# do
#     for w in ${workers[@]}
#     do
#         #HCLIB_WORKERS=$w srun -N 16 -n $((256/$w)) ./main_02 -s 16 -d 100 -c $c -g 10 -o $PWD/bin/a$c-$w.txt
#         for t in 0 1 2 3 4;
#         do
#             HCLIB_WORKERS=$w srun -N $nodes -n $((24*$nodes/$w)) -c $w ./main_02 -s 25 -d 30 -g 10 -c 0 -o $PWD/bin/c$w-$nodes.txt
#         done
#     done
#     for t in 0 1 2 3 4;
#         do
#             HCLIB_WORKERS=1 srun -N $nodes -n $((24*$nodes)) -c $w ./main_03 -s 25 -d 30 -g 10 -c 0 -o $PWD/bin/d-$nodes.txt
#         done
# done
