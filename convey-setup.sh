#!/bin/bash
module load gcc python openmpi/4.1.5 papi
export CC=oshcc
export CXX=oshc++

#### SCORE-P measurements ####
# export CC=scorep-oshcc
# export CXX=scorep-oshcxx
# export CPP=cpp
# export PATH=$PATH:/storage/coda1/p-vsarkar9/0/ssinghal74/scorep/install/bin/
##############################

cd bale/
cd src/bale_classic/
./bootstrap.sh
PLATFORM=oshmem ./make_bale -s -f
cd ../../../

export BALE_INSTALL=$PWD/bale/src/bale_classic/build_oshmem
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BALE_INSTALL/lib