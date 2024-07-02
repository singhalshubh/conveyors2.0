#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

class CONFIGURATION {
    public:
        uint64_t numberOfGraphs = 10;
        uint64_t scale_;
        uint64_t degree_;
        void GET_ARGS_FROM_CMD(int argc, char* argv[]) {
            int opt;
            while( (opt = getopt(argc, argv, "s:d:")) != -1 ) {
                switch(opt) {
                    case 's': sscanf(optarg, "%ld", &scale_); break;
                    case 'd': sscanf(optarg, "%ld", &degree_); break;
                    default:  break;
                }
            }
        }
};