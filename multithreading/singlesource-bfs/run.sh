#!/bin/bash

#nodes, scale, number of graphs 
# rm -rf bin && mkdir bin
sbatch script.sh 8 16 20
# sbatch script.sh 16 16 20
# sbatch script.sh 32 16 20
# sbatch script.sh 64 16 20
#source plot.sh 16 

# rm -rf bin && mkdir bin 
# sbatch script.sh 8 20 10
# sbatch script.sh 16 20 10
# sbatch script.sh 32 20 10
# sbatch script.sh 64 20 10
#source plot.sh 20 

# rm -rf bin && mkdir bin 
# sbatch script.sh 16 24 5
# sbatch script.sh 32 24 5 
# sbatch script.sh 64 24 5
# source plot.sh 24