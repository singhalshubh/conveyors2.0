#include "convey.h"

int main() {  
    const char *deps[] = { "system", "bale_actor" };
    hclib::launch(deps, 2, [=] {  
        T0_fprintf(stderr, "[Application: CONVEYORS done] PEs: %ld\n", THREADS);
        CONVEYORS *convey = new CONVEYORS;
        convey->BEGIN();
        
        shmem_barrier_all();

        uint64_t start_cycle = _rdtscp();
            while(convey->DONE() == false);
        uint64_t end_cycle = _rdtscp();
        uint64_t exec = end_cycle - start_cycle;
        T0_fprintf(stderr, "Time taken by simulated done: %ld cycles\n", exec);
        
        shmem_barrier_all();
        convey->DELETE();
        delete convey;
    });
    shmem_finalize();
    return EXIT_SUCCESS;
}