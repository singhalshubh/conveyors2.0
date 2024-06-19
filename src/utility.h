/*
Habanero Labs, US
Shubhendra Pal Singhal 2024
*/

#define ASSERT_WITH_MESSAGE(condition, message)\
   (!(condition)) ?\
      (std::cerr << "Assertion failed: (" << #condition << "), "\
      << "function " << __FUNCTION__\
      << ", file " << __FILE__\
      << ", line " << __LINE__ << "."\
      << std::endl << message << std::endl, abort(), 0) : 1

static inline uint64_t _rdtscp(void) {
    unsigned a, d;
    asm volatile("rdtscp" : "=a" (a), "=d" (d) : : "%rbx", "%rcx");
    return ((uint64_t) a) | (((uint64_t) d) << 32);
}

size_t PROCS_PER_NODE() {
    int *buffer = (int *) shmem_calloc(1, sizeof(int));
    shmem_barrier_all();
    size_t _LOCAL_NUM_PROCS = 0;
    for(int i = 0; i < THREADS; i++) {
        if(shmem_ptr(buffer, i) != NULL) {
            _LOCAL_NUM_PROCS += 1;
        }
    }
    shmem_free(buffer);
    #ifdef SYS_DEBUG
        T0_fprintf(stderr, "Setting local number of PEs on one node as: %ld\n", _LOCAL_NUM_PROCS);
    #endif
    return _LOCAL_NUM_PROCS;
}