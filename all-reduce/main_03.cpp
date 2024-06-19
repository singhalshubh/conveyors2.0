#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <shmem.h>
extern "C" {
#include "spmat.h"
}
#include "selector.h"

#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

namespace hclib {

    typedef struct {
        uint64_t i;
        float val;
    } Pkt;

    class RecursiveDoublingSelector : public hclib::Selector<1, Pkt> {
        uint64_t *data;

        void process(Pkt pkt, int sender_rank) {
            data[pkt.i] += pkt.val;
        }

    public:
        RecursiveDoublingSelector(uint64_t *_data) : data(_data) {
            mb[0].process = [this](Pkt pkt, int sender_rank) { this->process(pkt, sender_rank); };
        }
    };

    void AllReduceRecursiveDoubling(uint64_t *org_data, uint64_t size) {
        assert(THREADS && !(THREADS & (THREADS - 1)) == true);
        double t1 = wall_seconds();
        uint64_t *tmp_data = (uint64_t *) malloc(size * sizeof(uint64_t));
        for (int s = 1; s < THREADS; s *= 2) { //each stride is a "step"
            for(uint64_t tracker = 0; tracker < size; tracker++) {
                tmp_data[tracker] = 0; 
            }
            RecursiveDoublingSelector *rdSel = new RecursiveDoublingSelector(tmp_data);
            hclib::finish([=]{
                rdSel->start();
                for (int i = 0; i < size; i++) {
                    Pkt pkt;
                    pkt.i = i;
                    pkt.val = org_data[i];
                    int partner = (MYTHREAD + s) % THREADS;
                    if ((MYTHREAD / s) % 2 == 1) { partner = (MYTHREAD - s + THREADS) % THREADS; }
                    rdSel->send(0, pkt, partner);
                }
                rdSel->done(0);
            }); 
            for (int i = 0; i < size; i++) {
                org_data[i] += tmp_data[i];
            }            
            delete rdSel;
        }
        lgp_barrier();
        T0_fprintf(stderr, "Time: %lf\n", wall_seconds() - t1);
        for(uint64_t tracker = 0; tracker < size; tracker++) {
            assert((org_data)[tracker] == THREADS);
        }
    }
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
        
        uint64_t *org_data = (uint64_t *) malloc(driver->_dataSize * sizeof(uint64_t));
        for(uint64_t tracker = 0; tracker < driver->_dataSize; tracker++) {
            org_data[tracker] = 1; 
        }
        hclib::AllReduceRecursiveDoubling(org_data, driver->_dataSize);
    });
    shmem_finalize();
    return 0;
}