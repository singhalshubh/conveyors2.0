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
#include <random>

#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

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

class OneToAllSelector: public hclib::Selector<1, PACKET> {
  public: 
  std::vector<uint64_t> *LOCAL_BUFFER;
  std::vector<uint64_t> *GLOBAL_BUFFER;

    void process0(PACKET pkt, int sender_rank) {
        (*GLOBAL_BUFFER)[pkt.index] = pkt.data_item;
    }

    void one_to_all(uint64_t *mype) {
        
        if(*mype == MYTHREAD) {
            for(uint64_t pe = 0; pe < THREADS; pe++) {
                for(uint64_t data_index = 0; data_index < LOCAL_BUFFER->size(); data_index++) {
                    PACKET packet(data_index, (*LOCAL_BUFFER)[data_index]);  
                    send(0, packet, pe);
                }
            }
        }
        
    }
public:
  OneToAllSelector(std::vector<uint64_t> *_GLOBAL_BUFFER, std::vector<uint64_t> *_LOCAL_BUFFER) :
         LOCAL_BUFFER(_LOCAL_BUFFER), GLOBAL_BUFFER(_GLOBAL_BUFFER)   {
    mb[0].process = [this] (PACKET pkt, int sender_rank) { this->process0(pkt, sender_rank); };
  }
};

void reset_local_buffer(std::vector<uint64_t> *LOCAL_BUFFER) {
    for(uint64_t data_index = 0; data_index < LOCAL_BUFFER->size(); data_index++) {
        (*LOCAL_BUFFER)[data_index] = 0;
    }
}

void ONE_TO_ALL(std::vector<uint64_t> *GLOBAL_BUFFER, std::vector<uint64_t> *LOCAL_BUFFER, uint64_t dataSize, uint64_t mype) {
    std::string filename = "main_03-";
    filename += std::to_string(THREADS);
    filename += "-";
    filename += std::to_string(dataSize);
    filename += ".txt";
    FILE *fp = fopen(filename.c_str(), "a+");

    double t1 = wall_seconds();
    OneToAllSelector *RUNTIME = new OneToAllSelector(GLOBAL_BUFFER, LOCAL_BUFFER);
    hclib::finish([=, &mype] {
        RUNTIME->start();
        RUNTIME->one_to_all(&mype);
        RUNTIME->done(0);
    });
    shmem_barrier_all();
    T0_fprintf(fp, "%8.3f\n", wall_seconds() - t1);
    fclose(fp);
    delete RUNTIME;
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
            T0_fprintf(stderr, "[Application]: Naive One-to-all for %ld \n", _dataSize);
        }
};

int main(int argc, char *argv[]) {
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {
        MAIN *driver = new MAIN;
        driver->set_CLI(argc, argv);
        if(THREADS <= 32) {
           for(int pe = 0; pe < THREADS; pe++) {
                std::vector<uint64_t> *LOCAL_BUFFER = new std::vector<uint64_t>(driver->_dataSize, 1);
                std::vector<uint64_t> *GLOBAL_BUFFER = new std::vector<uint64_t>(driver->_dataSize, 0);
                ONE_TO_ALL(GLOBAL_BUFFER, LOCAL_BUFFER, driver->_dataSize, pe);
                for(uint64_t tracker = 0; tracker < driver->_dataSize; tracker++) {
                    assert((*GLOBAL_BUFFER)[tracker] == 1);
                }
                delete LOCAL_BUFFER;
                delete GLOBAL_BUFFER;
            } 
        }
        else if(THREADS >= 64) {        
            std::mt19937 gen(0);
            std::uniform_int_distribution<> dist(0, THREADS); 
            for(int tracker = 0; tracker < 10; tracker++) {
                std::vector<uint64_t> *LOCAL_BUFFER = new std::vector<uint64_t>(driver->_dataSize, 1);
                std::vector<uint64_t> *GLOBAL_BUFFER = new std::vector<uint64_t>(driver->_dataSize, 0);
                ONE_TO_ALL(GLOBAL_BUFFER, LOCAL_BUFFER, driver->_dataSize, dist(gen));
                for(uint64_t tracker = 0; tracker < driver->_dataSize; tracker++) {
                    assert((*GLOBAL_BUFFER)[tracker] == 1);
                }
                delete LOCAL_BUFFER;
                delete GLOBAL_BUFFER;
            }
        }
          
        T0_fprintf(stderr, "OK Passed\n");
        shmem_barrier_all();
    });
    shmem_finalize();
    return EXIT_SUCCESS;
}