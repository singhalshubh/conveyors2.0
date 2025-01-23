#!/bin/bash

# Env set up
module use /ccs/proj/csc607/cray-openshmemx/modulefiles
module purge
module load PrgEnv-cray/8.3.3
module load craype-x86-trento
module load cray-openshmemx/11.7.2.3
module load cray-pmi
module load cray-mrnet
module load xpmem
#module load perftools-base
module load cray-python/3.10.10
module load valgrind4hpc
module unload darshan-runtime
module unload hsi
module unload DefApps
module unload cray-libsci
module load cray-fix
# module load rocm/5.7.1
# module load cubew
# module load cubelib
# module load cubegui
# module load cray-libsci_acc

PROJ_DIR=/ccs/proj/csc607


#PAPI
# export PAPI_ROCM_ROOT=/opt/rocm-5.3.0
# export PAPI_ROCMSMI_ROOT=/opt/rocm-5.3.0/rocm_smi
# PAPI_DIRS=$PROJ_DIR/papi-master/src/install
# export PATH=$PAPI_DIRS/bin:$PATH
# export LD_LIBRARY_PATH=$PAPI_DIRS/lib:$LD_LIBRARY_PATH
# export C_INCLUDE_PATH=$PAPI_DIRS/include

# #Scorep
# export PATH=$PROJ_DIR/sources.06ba1e9b/install/bin:$PATH

# #Scorep plugins
# export LD_LIBRARY_PATH=$PROJ_DIR/scorep_plugin_x86_energy/build:$LD_LIBRARY_PATH
# export LD_LIBRARY_PATH=$PROJ_DIR/scorep_plugin_apapi/build:$LD_LIBRARY_PATH

# Export environment variables
export PLATFORM=cray

# Setup Bale
if [ ! -d bale ]; then
    #git clone https://github.com/jdevinney/bale.git
    cd bale/src/bale_classic/
    ./bootstrap.sh
    nice python3 make_bale --shmem --config_opts "CC=cc ac_cv_search_shmemx_team_alltoallv=no" -j
    cd ../../..
fi

export BALE_INSTALL=$PWD/bale/src/bale_classic/build_cray
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$BALE_INSTALL/lib