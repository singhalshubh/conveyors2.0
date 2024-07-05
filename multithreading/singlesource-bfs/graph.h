#define VERTEX uint64_t
#define EDGE std::set<VERTEX>

#define ASSERT_WITH_MESSAGE(condition, message)\
   (!(condition)) ?\
      (std::cerr << "Assertion failed: (" << #condition << "), "\
      << "function " << __FUNCTION__\
      << ", file " << __FILE__\
      << ", line " << __LINE__ << "."\
      << std::endl << message << std::endl, abort(), 0) : 1

struct fileAppPacket {
    VERTEX src;
    VERTEX dst;
    int type;
};

#define STORE 1
#define NO_STORE 0

std::uniform_real_distribution<float> udist(0, 1.0f);

class IMMpckt {
    public: 
        VERTEX dest_vertex;
        uint64_t tag;
};

class FileSelector: public hclib::Selector<1, fileAppPacket> {
    std::unordered_map<VERTEX, EDGE*> *adjacencyMap;

    void process(fileAppPacket appPkt, int sender_rank) {
        if(appPkt.type == STORE) {
            if(adjacencyMap->find(appPkt.src) != adjacencyMap->end()) {
                adjacencyMap->find(appPkt.src)->second->insert(appPkt.dst);
            }
            else {
                EDGE* e = new EDGE;
                e->insert(appPkt.dst);
                adjacencyMap->insert(std::make_pair(appPkt.src, e));
            }
        }
        else {
            if(adjacencyMap->find(appPkt.dst) == adjacencyMap->end()) {
                EDGE* e = new EDGE;
                adjacencyMap->insert(std::make_pair(appPkt.dst, e));
            }
        }
    }

public:
    FileSelector(std::unordered_map<uint64_t, EDGE*> *_adjacencyMap): 
            hclib::Selector<1, fileAppPacket>(true), adjacencyMap(_adjacencyMap) {
        mb[0].process = [this](fileAppPacket appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

class GRAPH {
    public:
        std::unordered_map<VERTEX, EDGE*> *G;
        uint64_t global_num_nodes;
        uint64_t global_num_edges;
        uint64_t block_num_edges_;
        uint64_t global_num_blocks_;
        std::set<VERTEX> *_LOCALE_visited;

        void ALLOCATE_GRAPH(CONFIGURATION*);
        void DEALLOCATE_GRAPH();
        void LOAD_GRAPH(CONFIGURATION*, trng::mt19937 *);
        void READ_GRAPH(CONFIGURATION*, trng::mt19937 *);
        void STATS_OF_FILE();
        void CHECK_FORMAT();
        int insertIntoVisited(VERTEX);
};

int GRAPH::insertIntoVisited(VERTEX appPkt) {
    if(G->find(appPkt) == G->end()) {
        return 0;
    }
    if(_LOCALE_visited->find(appPkt) == _LOCALE_visited->end()) {
        _LOCALE_visited->insert(appPkt);
        return 1;
    }
    return 0;
}

void GRAPH::ALLOCATE_GRAPH(CONFIGURATION *cfg) {
    this->G = new std::unordered_map<VERTEX, EDGE*>; 
    this->global_num_nodes = 1L << cfg->scale_;
    this->global_num_edges = this->global_num_nodes * cfg->degree_;
    int block_scale = cfg->scale_/2;
    this->block_num_edges_ = (1L << block_scale) * cfg->degree_;
    this->global_num_blocks_ = (1L << (cfg->scale_ - block_scale));
    this->_LOCALE_visited = new std::set<VERTEX>;
    // for(int pe = 0; pe < THREADS; pe++) {
    //     if(pe%2 == 0) {
    //         for(int tracker = 0; tracker < ((global_num_nodes/THREADS) * cfg->corrupted_)/100; tracker++) {
    //             _LOCALE_visited->insert(global_num_nodes+tracker);
    //         }
    //     }
    // }
}

void GRAPH::DEALLOCATE_GRAPH() {
    delete G;
    delete _LOCALE_visited;
}


void GRAPH::READ_GRAPH(CONFIGURATION *cfg, trng::mt19937 *rng) {
    FileSelector* genSelector = new FileSelector(this->G);
    hclib::finish([=]() {
        const float A = 0.57f, B = 0.19f, C = 0.19f;
        for (size_t block = MYTHREAD; block < global_num_blocks_; block += THREADS) {
            for (size_t m = 0; m < block_num_edges_; m++) {
                VERTEX src = 0;
                VERTEX dst = 0;
                for (uint64_t depth=0; depth < cfg->scale_; depth++) {
                    float rand_point = udist(*rng);
                    src = src << 1;
                    dst = dst << 1;
                    if (rand_point < A+B) {
                        if (rand_point > A) {
                            dst++;
                        }
                    } 
                    else {
                        src++;
                        if (rand_point > A+B+C) {
                            dst++;
                        }
                    }
                }
                fileAppPacket pckt;
                pckt.dst = src;
                pckt.src = dst;
                pckt.type = STORE;
                genSelector->send(0, pckt, pckt.src % THREADS);
                pckt.type = NO_STORE;
                genSelector->send(0, pckt, pckt.dst % THREADS);
            }
        }
        genSelector->done(0);
    });
    delete genSelector;
}

void GRAPH::LOAD_GRAPH(CONFIGURATION *cfg, trng::mt19937 *rng) {
    this->ALLOCATE_GRAPH(cfg);
    this->READ_GRAPH(cfg, rng);
    this->STATS_OF_FILE();
    this->CHECK_FORMAT();
}

void GRAPH::STATS_OF_FILE() {
    uint64_t local_nodes = G->size();
    uint64_t num_nodes = lgp_reduce_add_l(local_nodes);
    T0_fprintf(stderr, "Total Number of Nodes in G: %llu\n", num_nodes);
    
    uint64_t local_edges = 0;
    for(auto x: *G) {
        local_edges += (x.second)->size();
    }
    uint64_t num_edges = lgp_reduce_add_l(local_edges);
    global_num_edges = num_edges;
    T0_fprintf(stderr, "Total Number of Edges in G: %llu\n", num_edges);
}
    
void GRAPH::CHECK_FORMAT() {
    // Every vertice should be from 0..n-1 from the user
    for(auto x: *G) {
        assert(x.first < global_num_nodes);
        for(auto u: *(x.second)) {
            ASSERT_WITH_MESSAGE(u < global_num_nodes, std::to_string(u));
        }
    }
}