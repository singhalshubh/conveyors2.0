/*
Habanero Labs, US
Shubhendra Pal Singhal 2024
*/

#include <math.h>
#include <shmem.h>
extern "C" {
#include <spmat.h>
}
#include <string>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctime> 
#include <cstdlib> 
#include <sys/time.h>
#include <cmath>
#include <cstddef>
#include <limits>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <cassert>
#include <numeric>
#include <utility>
#include <mpi.h>
#include <iostream>

#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()
#define TRIALS uint64_t
#define ROUNDS uint64_t
#define TIME std::pair<UNIT_OF_TIME, UNIT_OF_TIME>
#define UNIT_OF_TIME uint64_t
#define PE uint64_t

#include "utility.h"

class MAIN {
    public: 
        std::vector<TIME> *_RAM = new std::vector<TIME>;
        ROUNDS _numberOfRounds = 0;
        void set_RAM() {
            _RAM = new std::vector<TIME>;;
        }
        void set_CLI(int argc, char *argv[]) {
            int opt;
            while( (opt = getopt(argc, argv, "hr:s:")) != -1 ) {
                switch(opt) {
                    case 'h': fprintf(stderr, "[HELP]: Microbenchmarking: -r <number of rounds>"); break;
                    case 'r': sscanf(optarg, "%ld" , &_numberOfRounds); break;
                    default:  break;
                }
            }
            ASSERT_WITH_MESSAGE(_numberOfRounds > 0, "Please specify the number of rounds, else program wouldn't execute\n");
            T0_fprintf(stderr, "[Application]: Measurement of your functions for %ld rounds\n", _numberOfRounds);
        }

        void _shmem_barrier_MEASURE() {
            int64_t var;
            for(ROUNDS r = 0; r < _numberOfRounds; r++) {
                shmem_barrier_all();
                    UNIT_OF_TIME start = _rdtsc();
                        lgp_barrier();
                    UNIT_OF_TIME end = _rdtsc();
                shmem_barrier_all();
                _RAM->push_back(std::make_pair(start, end));
            }
        }

        void _convey_done_MEASURE() {
            for(ROUNDS r = 0; r < _numberOfRounds; r++) {
                int64_t pop;
                convey_t* conveyor = convey_new(SIZE_MAX, 0, NULL, 0);
                convey_begin(conveyor, sizeof(int64_t), 0);
                shmem_barrier_all();
                    UNIT_OF_TIME start = _rdtsc();
                        while(convey_advance(conveyor, true)) {
                            convey_pull(conveyor, &pop, NULL);
                        }
                    UNIT_OF_TIME end = _rdtsc();
                shmem_barrier_all();
                convey_free(conveyor);
                _RAM->push_back(std::make_pair(start, end));
            }
        }

        void mpi() {
            for (ROUNDS r = 0; r < _numberOfRounds; r++) {
                MPI_Request request;
                MPI_Status status;
                shmem_barrier_all(); 
                UNIT_OF_TIME start = _rdtsc();
                MPI_Barrier(MPI_COMM_WORLD);
                UNIT_OF_TIME end = _rdtsc();
                shmem_barrier_all(); 
                _RAM->push_back(std::make_pair(start, end));
            }
        }

        void mpi_non_block() {
            for (ROUNDS r = 0; r < _numberOfRounds; r++) {
                MPI_Request request;
                MPI_Status status;
                shmem_barrier_all(); 
                UNIT_OF_TIME start = _rdtsc();
                MPI_Ibarrier(MPI_COMM_WORLD, &request);
                MPI_Wait(&request, &status);
                UNIT_OF_TIME end = _rdtsc();
                shmem_barrier_all(); 
                _RAM->push_back(std::make_pair(start, end));
            }
        }
};

void CALCULATE(std::vector<TIME> *RAM, std::string type) {
    std::vector<UNIT_OF_TIME> *RESULT_TIME = new std::vector<UNIT_OF_TIME>;
    std::vector<UNIT_OF_TIME> *RESULT_NOISE = new std::vector<UNIT_OF_TIME>;
    for(uint64_t tracker = 0; tracker < RAM->size(); tracker++) {
        uint64_t RESULT_1 = lgp_reduce_max_l((*RAM)[tracker].first);
        uint64_t RESULT_2 = lgp_reduce_min_l((*RAM)[tracker].first);
        if(MYTHREAD == 0) {
            RESULT_NOISE->push_back(RESULT_1 - RESULT_2);
        }
        uint64_t RESULT_WORK = lgp_reduce_max_l((*RAM)[tracker].second - (*RAM)[tracker].first);
        if(MYTHREAD == 0) {
            RESULT_TIME->push_back(RESULT_WORK);
        }
    }
    if(MYTHREAD == 0) {
        UNIT_OF_TIME avg_x = AVG(RESULT_TIME);
        UNIT_OF_TIME min_x = MIN(RESULT_TIME);
        UNIT_OF_TIME max_x = MAX(RESULT_TIME);
        UNIT_OF_TIME med_x = MEDIAN(RESULT_TIME);
        //fprintf(stderr, "System(%s): Time, min:%ld, avg: %ld, max: %ld, median: %ld\n", type.c_str(), min_x, avg_x, max_x, med_x);
        fprintf(stderr, "System(%s): Time:%ld\n", type.c_str(), min_x);
    }
    shmem_barrier_all();
    delete RESULT_TIME;
    delete RESULT_NOISE;
    RAM->clear();
}

int main(int argc, char *argv[]) {
    MPI_Init(NULL, NULL);
    lgp_init(argc, argv);

    MAIN *driver = new MAIN;
    driver->set_CLI(argc, argv);
    driver->set_RAM();

    shmem_barrier_all();
    driver->_shmem_barrier_MEASURE();
    CALCULATE(driver->_RAM, "shmem_block");
    shmem_barrier_all();

    shmem_barrier_all();
    driver->_convey_done_MEASURE();
    CALCULATE(driver->_RAM, "conveyors");
    shmem_barrier_all();

    shmem_barrier_all();
    driver->mpi();
    CALCULATE(driver->_RAM, "mpi_block");
    shmem_barrier_all();

    shmem_barrier_all();
    driver->mpi();
    CALCULATE(driver->_RAM, "mpi_non_block");
    shmem_barrier_all();

    shmem_finalize();
    return EXIT_SUCCESS;
}