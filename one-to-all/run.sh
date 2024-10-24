#!/bin/bash
#SBATCH --job-name=one-to-all
#SBATCH -A gts-vsarkar9-coda20
#SBATCH -N64
#SBATCH --ntasks-per-node=16
#SBATCH -t300
#SBATCH --mem-per-cpu=8gb
#SBATCH -qinferno         
#SBATCH -ojob.out

echo "Started on `/bin/hostname`"   # prints name of compute node job was started on
cd $SLURM_SUBMIT_DIR                # changes into directory where script was submitted from

source ./oshmem-slurm.sh

data=26
# while [ $data -le 28 ];
# do
    node=64
    while [ $node -le 64 ];
    do
        #srun -N $node -n $((16*$node)) ./main_01 -s $((2 ** $data))
        #srun -N $node -n $((16*$node)) ./main_02 -s $((2 ** $data))
        srun -N $node -n $((16*$node)) ./main_03 -s $((2 ** $data))
        node=$(($node*2))
    done
    data=$(($data+2))
# done
