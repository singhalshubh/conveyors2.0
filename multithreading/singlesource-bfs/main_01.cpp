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
#define DEBUG
#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#include <machine/endian.h>
#endif

#include "utility.h"
#include "configuration.h"
#include "graph.h"
#include "generateRR.h"

int main (int argc, char* argv[]) {
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {
        set_locales();
        hclib::async_at([=] {
            /* MASTER: IMM configuration parameters */
            CONFIGURATION *cfg = new CONFIGURATION;
            cfg->GET_ARGS_FROM_CMD(argc, argv);
            
            /*########## Generate and Build Graph ##############*/
            /*#################################################*/
            std::vector<GRAPH*>*_g_list = new std::vector<GRAPH*>; 
            T0_fprintf(stderr, "App: #PEs: %ld, scale: %ld\n", THREADS, cfg->scale_);
            
            for(uint64_t tracker = 0; tracker < cfg->numberOfGraphs; tracker++) {
                GRAPH *g = new GRAPH;
                g->LOAD_GRAPH(cfg, tracker);
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
        }, nic);
    });
    lgp_finalize();
    return EXIT_SUCCESS;
}
