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
#include "selector.h"
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

#include "../src/convey.h"


/*
###############################################
rdtscp records TIME as cycles in uint64_t format
###############################################
*/

class PACKET {
    public: 
        uint64_t index;
        uint64_t data_item;
        PACKET(uint64_t _index, uint64_t _data_item) {
            index = _index;
            data_item = _data_item;
        }
        PACKET() {
            index = -1;
            data_item = -1;
        }
};

class AllToAllSelector: public hclib::Selector<1, PACKET> {
  public: 
  std::vector<uint64_t> *LOCAL_BUFFER;
  std::vector<uint64_t> *GLOBAL_BUFFER;
  CONVEYORS *convey;

    void process0(PACKET pkt, int sender_rank) {
        (*GLOBAL_BUFFER)[pkt.index] += pkt.data_item;
    }

    void all_reduce(int phase) {

        /*####################################################################
        ####### sending the data item to proxy neighbors of Conveyors#########
        ####################################################################*/
        PORTER *p = &(convey->porter[phase]);
        for(uint64_t data_index = 0; data_index < LOCAL_BUFFER->size(); data_index++) {
            // Send data item to all neighboring pe's.
            for(uint64_t pe_index = 0; pe_index < p->_SIZE; pe_index++) {
                if(p->NEIGH[pe_index] != -1) {
                    PACKET packet(data_index, (*LOCAL_BUFFER)[data_index]);  
                    send(0, packet, p->NEIGH[pe_index]);
                }
            }
        } 
    }
public:
  AllToAllSelector(std::vector<uint64_t> *_GLOBAL_BUFFER, std::vector<uint64_t> *_LOCAL_BUFFER, CONVEYORS *_convey) :
         LOCAL_BUFFER(_LOCAL_BUFFER), convey(_convey), GLOBAL_BUFFER(_GLOBAL_BUFFER)   {
    mb[0].process = [this] (PACKET pkt, int sender_rank) { this->process0(pkt, sender_rank); };
  }
};

void reset_local_buffer(std::vector<uint64_t> *LOCAL_BUFFER) {
    for(uint64_t data_index = 0; data_index < LOCAL_BUFFER->size(); data_index++) {
        (*LOCAL_BUFFER)[data_index] = 0;
    }
}

void ALL_REDUCE(std::vector<uint64_t> *res, std::vector<uint64_t> *a, uint64_t dataSize, CONVEYORS *convey) {
    std::vector<uint64_t> *LOCAL_BUFFER = new std::vector<uint64_t>(dataSize);
    std::vector<uint64_t> *GLOBAL_BUFFER = new std::vector<uint64_t>(dataSize, 0);
    for(uint64_t tracker; tracker < LOCAL_BUFFER->size(); tracker++) {
        (*LOCAL_BUFFER)[tracker] = (*a)[tracker];
    }
    double t1 = wall_seconds();
    for(uint64_t phase = 0; phase < convey->_NUM_PORTERS; phase++) {
        AllToAllSelector *RUNTIME = new AllToAllSelector(GLOBAL_BUFFER, LOCAL_BUFFER, convey);
        hclib::finish([=] {
            RUNTIME->start();
            RUNTIME->all_reduce(phase);
            RUNTIME->done(0);
        });
        delete RUNTIME;
        reset_local_buffer(LOCAL_BUFFER);
        if(phase < convey->_NUM_PORTERS - 1) {
            std::swap(LOCAL_BUFFER, GLOBAL_BUFFER);
        } 
    }
    shmem_barrier_all();
    T0_fprintf(stderr, "Time: %8.3f sec\n", wall_seconds() - t1);
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
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {
        MAIN *driver = new MAIN;
        driver->set_CLI(argc, argv);

        CONVEYORS *convey = new CONVEYORS;
        convey->BEGIN();
        std::vector<uint64_t> *LOCAL_BUFFER = new std::vector<uint64_t>(driver->_dataSize, 1);
        std::vector<uint64_t> *GLOBAL_BUFFER = new std::vector<uint64_t>(driver->_dataSize, 0);
        
        //ALL_REDUCE(GLOBAL_BUFFER, LOCAL_BUFFER, driver->_dataSize, convey);
        
        double t1 = wall_seconds();
        GLOBAL_BUFFER = lgp_reduce_add_l(LOCAL_BUFFER);
        T0_fprintf(stderr, "Time: %8.3f sec\n", wall_seconds() - t1);

        for(uint64_t tracker = 0; tracker < driver->_dataSize; tracker++) {
            assert((*GLOBAL_BUFFER)[tracker] == THREADS);
        }
        T0_fprintf(stderr, "OK Passed\n");
        shmem_barrier_all();
    });
    shmem_finalize();
    return EXIT_SUCCESS;
}