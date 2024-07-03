#!/bin/bash
#SBATCH --job-name=actor-1
#SBATCH -A gts-vsarkar9-forza
#SBATCH -N64
#SBATCH --ntasks-per-node=16
#SBATCH --mem-per-cpu=12gb
#SBATCH -t60
#SBATCH -qinferno        
#SBATCH -ojob.out

echo "Started on `/bin/hostname`"   # prints name of compute node job was started on
cd $SLURM_SUBMIT_DIR 

corruption=(0 10 30 50 70)
workers=(1 2 4 8 16)

for c in ${corruption[@]}
do
    for w in ${workers[@]}
    do
        #HCLIB_WORKERS=$w srun -N 16 -n $((256/$w)) ./main_02 -s 16 -d 100 -c $c -g 10 -o $PWD/bin/a$c-$w.txt
        HCLIB_WORKERS=$w srun -N 64 -n $((1024/$w)) ./main_02 -s 20 -d 100 -c $c -g 100 -o $PWD/bin/a$c-$w.txt
    done
done