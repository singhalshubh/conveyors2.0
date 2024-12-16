#!/bin/bash
module load gcc python openmpi/4.1.4
export CC=oshcc
export CXX=oshc++

#### SCORE-P measurements ####
# export CC=scorep-oshcc
# export CXX=scorep-oshcxx
# export CPP=cpp
# export PATH=$PATH:/storage/coda1/p-vsarkar9/0/ssinghal74/scorep/install/bin/


export CC=oshcc
export CXX=oshc++

# if [ ! -d bale ]; then
# #   git clone https://github.com/jdevinney/bale.git
#     cd bale/src/bale_classic/
#     ./bootstrap.sh
#     PLATFORM=oshmem ./make_bale -s -f
#     cd ../../../
# fi

# cd bale/src/bale_classic/
# ./bootstrap.sh
# PLATFORM=oshmem ./make_bale -s -f
# cd ../../../

export BALE_INSTALL=$PWD/bale/src/bale_classic/build_oshmem
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BALE_INSTALL/lib