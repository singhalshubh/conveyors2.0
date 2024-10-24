#include <shmem.h>
#include <stdio.h>
#include <cassert>
#include <cstring>

static inline uint64_t _rdtsc(void) {
    unsigned a, d;
    asm volatile("rdtsc" : "=a" (a), "=d" (d) : : "%rbx", "%rcx");
    return ((uint64_t) a) | (((uint64_t) d) << 32);
}

int main(int argc, char *argv[]) {
    shmem_init();
    size_t SIZE = std::stoi(argv[1]); 
    int *shm = (int *) shmem_malloc(SIZE*sizeof(int));
    int *local = (int *) malloc(SIZE*sizeof(int));
    for(int i = 0; i < SIZE; i++) {
        local[i] = i;
    }
    uint64_t start_time, end_time;
    shmem_barrier_all();

    if (shmem_my_pe() == 0) {
        //int *ptr = (int *) shmem_ptr(shm, 1);
        start_time = _rdtsc();
        shmem_putmem(shm, local, SIZE*sizeof(int), 1);
        //std::memcpy(ptr, local, SIZE*sizeof(int));
        end_time = _rdtsc();
    }
    shmem_barrier_all();
    if(shmem_my_pe() == 0) {
        std::string ff = "debug-";
        ff += std::to_string(SIZE);
        ff += ".txt";
        FILE *fp = fopen(ff.c_str(), "a");
        fprintf(fp, "%ld\n", end_time - start_time);
        fclose(fp);
    }
    if(shmem_my_pe() == 1) {
        for(int i = 0; i < SIZE; i++) {
            assert(shm[i] == i && shm[i] == local[i]);
        }
    }
    shmem_barrier_all();
    shmem_free(shm);
    delete local;
    shmem_finalize();
}