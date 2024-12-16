#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdint>

#define NUM_ELEMENTS 200000000 // 200M elements

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv); // Initialize the MPI environment

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank); // Get the rank of the process
    MPI_Comm_size(MPI_COMM_WORLD, &world_size); // Get the total number of processes

    // Allocate and initialize local data
    std::vector<uint64_t> local_data(NUM_ELEMENTS, 1); // Example initialization

    // Allocate space for the result
    std::vector<uint64_t> global_result(NUM_ELEMENTS);

    // Perform MPI_Allreduce with MPI_UINT64_T
    double start_time = MPI_Wtime(); // Start timing
    MPI_Allreduce(local_data.data(), global_result.data(), NUM_ELEMENTS, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD);
    double end_time = MPI_Wtime(); // End timing
    if(world_rank == 0) {
        fprintf(stderr, "Time: %f\n", end_time - start_time);
    }



    MPI_Finalize(); // Finalize the MPI environment
    return 0;
}
