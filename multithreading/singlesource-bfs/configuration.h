#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

class CONFIGURATION {
    public:
        char filename[200];
        uint64_t numberOfGraphs;
        uint64_t scale_;
        uint64_t degree_;
        uint64_t corrupted_;
        void GET_ARGS_FROM_CMD(int argc, char* argv[]) {
            int opt;
            while( (opt = getopt(argc, argv, "s:d:g:c:o:")) != -1 ) {
                switch(opt) {
                    case 'o': sscanf(optarg, "%s", filename); break;
                    case 'g': sscanf(optarg, "%ld", &numberOfGraphs); break;
                    case 'c': sscanf(optarg, "%ld", &corrupted_); break;
                    case 's': sscanf(optarg, "%ld", &scale_); break;
                    case 'd': sscanf(optarg, "%ld", &degree_); break;
                    default:  break;
                }
            }
        }
};