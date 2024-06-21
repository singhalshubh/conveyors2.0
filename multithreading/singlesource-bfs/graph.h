#define VERTEX uint64_t
#define EDGE std::set<VERTEX>

#define STORE 1
#define NO_STORE 0

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
    bool type;
};

class IMMpckt {
    public: 
        VERTEX dest_vertex;
};

class FileSelector: public hclib::Selector<1, fileAppPacket> {
    std::unordered_map<VERTEX, EDGE*> *adjacencyMap;
    CONFIGURATION *cfg;

    void process(fileAppPacket appPkt, int sender_rank) {
        if(appPkt.type == STORE) {
            if(adjacencyMap->find(appPkt.src) != adjacencyMap->end()) {
                EDGE *e = adjacencyMap->find(appPkt.src)->second;
                if(e->find(appPkt.dst) == e->end()) {
                    e->insert(appPkt.dst);
                }
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
    FileSelector(std::unordered_map<VERTEX, EDGE*> *_adjacencyMap, CONFIGURATION *_cfg): 
            hclib::Selector<1, fileAppPacket>(true), adjacencyMap(_adjacencyMap), cfg(_cfg) {
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
        bool insertIntoVisited(VERTEX);
};

bool GRAPH::insertIntoVisited(VERTEX appPkt) {
    if(_LOCALE_visited->find(appPkt) == _LOCALE_visited->end()) {
        _LOCALE_visited->insert(appPkt);
        return true;
    }
    return false;
}

void GRAPH::ALLOCATE_GRAPH(CONFIGURATION *cfg) {
    this->G = new std::unordered_map<VERTEX, EDGE*>; 
    this->global_num_nodes = 1L << cfg->scale_;
    this->global_num_edges = this->global_num_nodes * cfg->degree_;
    int block_scale = cfg->scale_/2;
    this->block_num_edges_ = (1L << block_scale) * cfg->degree_;
    this->global_num_blocks_ = (1L << (cfg->scale_ - block_scale));
    _LOCALE_visited = new std::set<VERTEX>;
}

void GRAPH::DEALLOCATE_GRAPH() {
    delete G;
    delete _LOCALE_visited;
}

void GRAPH::READ_GRAPH(CONFIGURATION *cfg, trng::mt19937 *rng) {
    FileSelector* genSelector = new FileSelector(this->G, cfg);
    hclib::finish([=]() {
        for (size_t block = MYTHREAD; block < global_num_blocks_; block += THREADS) {
            trng::uniform_int_dist udist(0, global_num_nodes-1);
            for (size_t m = 0; m < block_num_edges_; m++) {
                VERTEX u = udist(*rng);
                VERTEX v = udist(*rng);
                fileAppPacket pckt;
                pckt.dst = u;
                pckt.src = v;
                pckt.type = STORE;
                genSelector->send(0, pckt, pckt.src % THREADS);
                pckt.type = NO_STORE;
                genSelector->send(0, pckt, pckt.dst % THREADS);
            }
        }
        genSelector->done(0);
    });
}

void GRAPH::LOAD_GRAPH(CONFIGURATION *cfg, trng::mt19937 *rng) {
    this->ALLOCATE_GRAPH(cfg);
    this->READ_GRAPH(cfg, rng);
    this->STATS_OF_FILE();
}

void GRAPH::STATS_OF_FILE() {
    uint64_t local_nodes = G->size();
    uint64_t num_nodes = lgp_reduce_add_l(local_nodes);
    global_num_nodes = num_nodes;
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