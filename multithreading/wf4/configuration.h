#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()

#define RMAT 0
#define UNIFORM 1

class CONFIGURATION {
    public:
        char fileName[200];
        char outputfileName[200];
        double epsilon;
        uint64_t k;
        bool type;
        bool undirected = false;
        bool TYPE = RMAT;
        int scale_ = 0;
        int degree_ = -1;
        void GET_ARGS_FROM_CMD(int argc, char* argv[]) {
            int opt;
            while( (opt = getopt(argc, argv, "huge:k:o:s:d:f:")) != -1 ) {
                switch(opt) {
                    case 'h': fprintf(stderr, "[HELP]: Multi-threading in graphs for conveyors2.0 -e <epsilon> -k <#ofinfluencers> -o outputfilename -u<1 for unweighted>, -c,r,x for AGL Mappers"); exit(1);
                    case 'e': sscanf(optarg,"%lf" , &(epsilon)); break;
                    case 'k': sscanf(optarg,"%ld", &(k)); break;
                    case 'o': sscanf(optarg,"%s" , outputfileName); break;
                    case 'u': undirected = true; break;
                    case 'g': TYPE = UNIFORM; break;
                    case 's': sscanf(optarg,"%d" , &(scale_)); break;
                    case 'd': sscanf(optarg,"%d" , &(degree_)); break;
                    case 'f': sscanf(optarg,"%s" , fileName); break;
                    default:  break;
                }
            }
            T0_fprintf(stderr, "Application: Multi-threading in graphs for conveyors2.0, Number of influencers: %ld, epsilon = %f, output file: %s, Is un-directed: %d, scale: %d, degree: %d, file: %s\n\n", k, epsilon, outputfileName, undirected, scale_, degree_, fileName);
        }
};