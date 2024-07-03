#include <math.h>
#include <shmem.h>
extern "C" {
#include <spmat.h>
}
#include <std_options.h>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <queue>
#include <fstream>
#include <sys/stat.h>
#include <sys/time.h>
#include "selector.h"
#include <endian.h>
#include <ctime> 
#include <cstdlib> 
#include <random>
#include <sys/time.h>
#include <cmath>
#include <cstddef>
#include <limits>
#include "trng/lcg64.hpp"
#include "trng/uniform01_dist.hpp"
#include "trng/uniform_int_dist.hpp"
#include "trng/mt19937.hpp"
#include <chrono>
#include <utility>
#include <memory>
#include <sstream>

#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#include <machine/endian.h>
#endif

#include "utility.h"
#include "configuration.h"
#include "graph.h"
#include "generateRR_3.h"

int main (int argc, char* argv[]) {
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {
        /* MASTER: IMM configuration parameters */
        CONFIGURATION *cfg = new CONFIGURATION;
        cfg->GET_ARGS_FROM_CMD(argc, argv);
        /*########## Generate and Build Graph ##############*/
        /*#################################################*/
        std::vector<GRAPH*>*_g_list = new std::vector<GRAPH*>; 
        trng::mt19937 rng, rng1;
        rng.seed(0UL + MYTHREAD);
        rng1.seed(12UL);
        int max_scale = cfg->scale_;
        T0_fprintf(stderr, "App: #PEs: %ld, scale: %ld\n", THREADS, max_scale);
        int max_deg = cfg->degree_;
        for(uint64_t tracker = 0; tracker < cfg->numberOfGraphs; tracker++) {
            GRAPH *g = new GRAPH;
            std::uniform_int_distribution<int> udist(10, max_scale);
            cfg->scale_ = udist(rng1);
            std::uniform_int_distribution<int> udist1(10, max_deg);
            cfg->degree_ = udist1(rng1);
            g->LOAD_GRAPH(cfg, &rng);
            _g_list->push_back(g);
            #ifdef DEBUG
                g->CHECK_FORMAT();
            #endif
        }
        /*#################################################*/
        /*############# IMM Math and time init ####################*/
        /*#################################################*/
        lgp_barrier();
        GENERATE_RRR *sample = new GENERATE_RRR();
        sample->PERFORM_GENERATERR(_g_list, cfg);  
        sample->DELETE_GENERATERR();
        delete sample;
        for(auto g: *_g_list) {
            g->DEALLOCATE_GRAPH();
        }
        delete _g_list;
    });
    lgp_finalize();
    return EXIT_SUCCESS;
}

// class TestSelector: public hclib::Selector<1, int64_t> {
//     int64_t *sum;

//     int64_t fib(int n) {
//         if (n == 0) {
//             return 0;
//         } else if (n == 1) {
//             return 1;
//         } else {
//             return fib(n - 1) + fib(n - 2);
//         }
//     }

//     int64_t fib_async(int n) {
//         if (n == 0) {
//             return 0;
//         } else if (n == 1) {
//             return 1;
//         } else {
//             int64_t ret1, ret2;
//             hclib::finish([=, &ret1, &ret2] {
//                 hclib::async([=, &ret1]{ ret1 = fib(n - 1); });
//                 hclib::async([=, &ret2]{ ret2 = fib(n - 2); });
//             });
//             return ret1 + ret2;
//         }
//     }

//     void process(int64_t pkt, int sender_rank) {
//         *sum = *sum + fib_async(pkt);
//     }

//   public:

//     TestSelector(int64_t *_sum): sum(_sum) {
//         mb[0].process = [this](int64_t pkt, int sender_rank) { this->process(pkt, sender_rank); };
//     }
// };
