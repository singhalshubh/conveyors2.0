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
#include <chrono>
#include <utility>
#include <memory>
#include <sstream>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#include <machine/endian.h>
#endif
#define DEBUG

#include "utility.h"
#include "configuration.h"
#include "graph.h"
#include "generateRR.h"

int main (int argc, char* argv[]) {
    static long lock = 0;
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {


        /*MULTITHREADING (derived from Akihiro, Habanero Labs)*/
        int nworkers = hclib_get_num_workers();
        int nlocales = hclib_get_num_locales();
        hclib_locale_t *locales = hclib_get_all_locales();
        for (int i = 0; i < nlocales; i++) {
            hclib_locale_t l = locales[i];
            //fprintf(stderr, "locale%d: %s\n", i, l.lbl);
            if (i == 1) {
                nic = &locales[i];
                //fprintf(stderr, "%s is assigned to NIC (Communication+General Worker)\n", l.lbl);
            }
        }

        /* MASTER: IMM configuration parameters */

        /*########## Generate and Build Graph ##############*/
        /*#################################################*/

        CONFIGURATION *cfg = new CONFIGURATION;
        cfg->GET_ARGS_FROM_CMD(argc, argv);

        GRAPH *g = new GRAPH;
        g->cfg = cfg;
        g->LOAD_GRAPH();
        #ifdef DEBUG
            g->CHECK_FORMAT();
        #endif

        /*#################################################*/
        /*############# IMM Math and time init ####################*/
        /*#################################################*/

        struct timeval tt,rr,tt1,rr1,generateRR_time = {0}, selectSeeds_time = {0};
        gettimeofday(&tt, NULL);
        #ifdef DEBUG
            T0_fprintf(stderr, "STEP 1: Sampling\n");
        #endif

        generator.seed(0UL);
        generator.split(2, 1);
        generator.split(THREADS, MYTHREAD);
        double l = 1.0;
        l = l * (1 + 1 / std::log2(g->global_num_nodes));
        double epsilonPrime =  1.4142135623730951 * cfg->epsilon;
        double LB = 0;
        size_t thetaPrimePrevious = 0;

        /*#################################################*/
        /*############# DATA Structures init ###############*/
        /*#################################################*/

        GENERATE_RRR *sample = new GENERATE_RRR();
        std::unordered_map<VERTEX, std::set<TAG>*> *visited = new std::unordered_map<VERTEX, std::set<TAG>*>;

        for(int tracker = 1; tracker < std::log2(g->global_num_nodes); ++tracker) {
            ssize_t thetaPrime = ThetaPrime(tracker, epsilonPrime, l, cfg->k, g->global_num_nodes)/THREADS + 1;
            size_t delta = thetaPrime - thetaPrimePrevious;
            #ifdef DEBUG
                T0_fprintf(stderr, "Delta/PE: %ld\n", delta);
                gettimeofday(&tt1, NULL);
            #endif
            sample->PERFORM_GENERATERR(g, visited, delta, thetaPrimePrevious);  
            sample->CLEAR_GENERATERR();

            thetaPrimePrevious += delta;
        }
        size_t thetaLocal = Theta(cfg->epsilon, l, cfg->k, LB, g->global_num_nodes)/THREADS + 1;
        #ifdef DEBUG
            T0_fprintf(stderr, "\nThetaFinal/PE: %ld\n", thetaLocal - thetaPrimePrevious);
            T0_fprintf(stderr, "final,STEP 2: Generate RR final\n");
            gettimeofday(&tt1, NULL);
        #endif
        if (thetaLocal > thetaPrimePrevious) {
            size_t final_delta = thetaLocal - thetaPrimePrevious;
            sample->PERFORM_GENERATERR(g, visited, final_delta, thetaPrimePrevious);
            sample->CLEAR_GENERATERR();
        }
        gettimeofday(&rr, NULL);
        timersub(&rr, &tt, &rr);
        T0_fprintf(stderr, "Total Time: %8.3lf seconds\n", rr.tv_sec + (double) rr.tv_usec/(double)1000000);
        
        lgp_barrier();
        delete visited;
        sample->DELETE_GENERATERR();
        delete sample;
        g->DEALLOCATE_GRAPH();
        delete g;
        delete cfg;
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
