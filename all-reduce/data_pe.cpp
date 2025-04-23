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
#include <sstream>
#include <cassert>
#include <numeric>
#include <utility>

#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

/*
###############################################
rdtscp records TIME as cycles in uint64_t format
###############################################
*/

struct PACKET {
    uint64_t index;
    uint64_t data_item;
};

void reset_local_buffer(std::vector<uint64_t> *LOCAL_BUFFER) {
    for(uint64_t data_index = 0; data_index < LOCAL_BUFFER->size(); data_index++) {
        (*LOCAL_BUFFER)[data_index] = 0;
    }
}

void ALL_REDUCE(std::vector<uint64_t> *res, std::vector<uint64_t> *a, uint64_t dataSize) {
    std::vector<uint64_t> *LOCAL_BUFFER = new std::vector<uint64_t>(dataSize);
    std::vector<uint64_t> *GLOBAL_BUFFER = new std::vector<uint64_t>(dataSize, 0);
    for(uint64_t tracker; tracker < LOCAL_BUFFER->size(); tracker++) {
        (*LOCAL_BUFFER)[tracker] = (*a)[tracker];
    }
    convey_t* conveyor = convey_new(SIZE_MAX, 0, NULL, 0);
    convey_begin(conveyor, sizeof(PACKET), 0);
    double t1 = wall_seconds();
    uint64_t data_index = 0;
    uint64_t pe = 0;
    struct PACKET pkt, packet;

    while (convey_advance(conveyor, data_index == LOCAL_BUFFER->size())) {
        while (data_index < LOCAL_BUFFER->size()) {
            packet.index = data_index;
            packet.data_item = (*LOCAL_BUFFER)[data_index];
            while (pe < THREADS) {
                if (convey_push(conveyor, &packet, pe) == convey_OK) {
                    pe++;
                }
                else {
                    break;
                }
            }
            if (pe == THREADS) {
                pe = 0;
                data_index++; 
            } 
            else {
                break;
            }
        }
        while (convey_pull(conveyor, &pkt, NULL) == convey_OK) {
            (*GLOBAL_BUFFER)[pkt.index] += pkt.data_item;
        }
    }

    shmem_barrier_all();
    T0_fprintf(stderr, "Time: %8.3f sec\n", wall_seconds() - t1);
    convey_free(conveyor);
    for(uint64_t tracker; tracker < GLOBAL_BUFFER->size(); tracker++) {
        (*res)[tracker] = (*GLOBAL_BUFFER)[tracker];
    }
    delete LOCAL_BUFFER;
}

class MAIN {
    public: 
        uint64_t _dataSize;
        void set_CLI(int argc, char *argv[]) {
            int opt;
            while( (opt = getopt(argc, argv, "hr:s:")) != -1 ) {
                switch(opt) {
                    case 'h': fprintf(stderr, "[HELP]: Microbenchmarking: -s <data-size>"); break;
                    case 's': sscanf(optarg, "%ld" , &_dataSize); break;
                    default:  break;
                }
            }
            T0_fprintf(stderr, "[Application]: All-Reduce for %ld \n", _dataSize);
        }
};

int main(int argc, char *argv[]) {
    lgp_init(argc, argv);
    MAIN *driver = new MAIN;
    driver->set_CLI(argc, argv);

    std::vector<uint64_t> *LOCAL_BUFFER = new std::vector<uint64_t>(driver->_dataSize, 1);
    std::vector<uint64_t> *GLOBAL_BUFFER = new std::vector<uint64_t>(driver->_dataSize, 0);
    
    ALL_REDUCE(GLOBAL_BUFFER, LOCAL_BUFFER, driver->_dataSize);
    
    for(uint64_t tracker = 0; tracker < driver->_dataSize; tracker++) {
        assert((*GLOBAL_BUFFER)[tracker] == THREADS);
    }
    T0_fprintf(stderr, "OK Passed\n");
    shmem_finalize();
    return EXIT_SUCCESS;
}
