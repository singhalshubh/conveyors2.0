#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

#define IC 0
#define LT 1

#define ESTIMATE_THETA 0
#define FINAL 1

class CONFIGURATION {
    public:
        uint64_t numberOfGraphs;
        uint64_t scale_;
        uint64_t degree_;
        void GET_ARGS_FROM_CMD(int argc, char* argv[]) {
            int opt;
            while( (opt = getopt(argc, argv, "g:s:d:")) != -1 ) {
                switch(opt) {
                    case 'g': sscanf(optarg, "%ld", &numberOfGraphs); break;
                    case 's': sscanf(optarg, "%ld", &scale_); break;
                    case 'd': sscanf(optarg, "%ld", &degree_); break;
                    default:  break;
                }
            }
        }
};