#define DST uint64_t
#define SRC uint64_t
#define WEIGHT double
#define VERTEX uint64_t

#define EDGE std::unordered_map<DST, WEIGHT>
#define TAG uint64_t

trng::lcg64 g_generator;
trng::uniform01_dist<float> g_val;

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
    SRC src;
    DST dst;
    WEIGHT weight;
    bool type;
};

class IMMpckt {
    public: 
        TAG tag;
        VERTEX dest_vertex;
};

class FileSelector: public hclib::Selector<1, fileAppPacket> {
    std::unordered_map<SRC, EDGE*> *adjacencyMap;
    CONFIGURATION *cfg;

    void process(fileAppPacket appPkt, int sender_rank) {
        if(appPkt.type == STORE) {
            if(adjacencyMap->find(appPkt.src) != adjacencyMap->end()) {
                EDGE *e = adjacencyMap->find(appPkt.src)->second;
                if(e->find(appPkt.dst) != e->end()) {
                    e->find(appPkt.dst)->second += appPkt.weight;
                }
                else {
                    e->insert(std::make_pair(appPkt.dst, appPkt.weight));
                }
            }
            else {
                EDGE* e = new EDGE;
                e->insert(std::make_pair(appPkt.dst, appPkt.weight));
                adjacencyMap->insert(std::make_pair(appPkt.src, e));
            }
        }
        else {
            if(cfg->undirected == false) {
                if(adjacencyMap->find(appPkt.dst) == adjacencyMap->end()) {
                    EDGE* e = new EDGE;
                    adjacencyMap->insert(std::make_pair(appPkt.dst, e));
                }
            }
            else {
                if(adjacencyMap->find(appPkt.dst) != adjacencyMap->end()) {
                    EDGE *e = adjacencyMap->find(appPkt.dst)->second;
                    if(e->find(appPkt.src) != e->end()) {
                        e->find(appPkt.src)->second += appPkt.weight;
                    }
                    else {
                        e->insert(std::make_pair(appPkt.src, appPkt.weight));
                    }
                }
                else {
                    EDGE* e = new EDGE;
                    e->insert(std::make_pair(appPkt.src, appPkt.weight));
                    adjacencyMap->insert(std::make_pair(appPkt.dst, e));
                }
            }
        }
    }

public:
    FileSelector(std::unordered_map<uint64_t, EDGE*> *_adjacencyMap, CONFIGURATION *_cfg): 
            hclib::Selector<1, fileAppPacket>(true), adjacencyMap(_adjacencyMap), cfg(_cfg) {
        mb[0].process = [this](fileAppPacket appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

class GRAPH {
    public:
        std::unordered_map<SRC, EDGE*> *G;
        uint64_t global_num_nodes;
        uint64_t global_num_edges;
        CONFIGURATION *cfg;
        uint64_t global_num_blocks_;
        uint64_t block_num_edges_;

        void ALLOCATE_GRAPH();
        void DEALLOCATE_GRAPH();
        void GENERATE_GRAPH();
        void LOAD_GRAPH();
        void READ_GRAPH();
        void STATS_OF_FILE();
        void generateWeightsLC();
        void CHECK_FORMAT();
};

void GRAPH::ALLOCATE_GRAPH() {
    this->G = new std::unordered_map<SRC, EDGE*>; 
    this->global_num_nodes = 1L << cfg->scale_;
    this->global_num_edges = this->global_num_nodes * cfg->degree_;
    int block_scale = cfg->scale_/2;
    this->block_num_edges_ = (1L << block_scale) * cfg->degree_;
    this->global_num_blocks_ = (1L << (cfg->scale_ - block_scale));
    //T0_fprintf(stderr, "%ld, %ld, %ld\n",block_scale, this->block_num_edges_, this->global_num_blocks_);
}

void GRAPH::DEALLOCATE_GRAPH() {
    delete G;
}

void GRAPH::GENERATE_GRAPH() {
    FileSelector* genSelector = new FileSelector(this->G, this->cfg);
    hclib::finish([=]() {
        if(cfg->TYPE == RMAT) {
            const float A = 0.57f, B = 0.19f, C = 0.19f;
            std::mt19937 rng;
            std::uniform_real_distribution<float> udist(0, 1.0f);
            for (size_t block = MYTHREAD; block < global_num_blocks_; block += THREADS) {
                rng.seed(kRandSeed + block);
                for (size_t m = 0; m < block_num_edges_; m++) {
                    SRC src = 0;
                    DST dst = 0;
                    for (uint64_t depth=0; depth < cfg->scale_; depth++) {
                        float rand_point = udist(rng);
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
                    pckt.weight = g_val(g_generator);
                    pckt.dst = src;
                    pckt.src = dst;
                    pckt.type = STORE;
                    genSelector->send(0, pckt, pckt.src % THREADS);
                    pckt.type = NO_STORE;
                    genSelector->send(0, pckt, pckt.dst % THREADS);
                }
            }
        }
        else {
            std::mt19937 rng;
            for (size_t block = MYTHREAD; block < global_num_blocks_; block += THREADS) {
                rng.seed(kRandSeed + block);
                std::uniform_int_distribution<int> udist(0, global_num_nodes-1);
                for (size_t m = 0; m < block_num_edges_; m++) {
                    SRC u = udist(rng);
                    DST v = udist(rng);
                    fileAppPacket pckt;
                    pckt.weight = g_val(g_generator);
                    pckt.dst = u;
                    pckt.src = v;
                    pckt.type = STORE;
                    genSelector->send(0, pckt, pckt.src % THREADS);
                    pckt.type = NO_STORE;
                    pckt.weight = g_val(g_generator);
                    genSelector->send(0, pckt, pckt.dst % THREADS);
                }
            }
        }
        genSelector->done(0);
    });
    delete genSelector;
}

void GRAPH::READ_GRAPH() {
    FileSelector* fileSelector = new FileSelector(this->G, this->cfg);
    hclib::finish([=]() {
        struct stat stats;
        std::ifstream file(this->cfg->fileName);
        if (!file.is_open()) { 
            report("ERROR_OPEN_FILE"); 
        }
        std::string line;
        stat(this->cfg->fileName, & stats);

        uint64_t bytes = stats.st_size / THREADS;       
        uint64_t rem_bytes = stats.st_size % THREADS;
        uint64_t start, end;
        if(MYTHREAD < rem_bytes) {
            start = MYTHREAD*(bytes + 1);
            end = start + bytes + 1;
        }
        else {
            start = MYTHREAD*bytes + rem_bytes;
            end = start + bytes;
        }

        file.seekg(start);
        if (MYTHREAD != 0) {                                     
            file.seekg(start - 1);
            getline(file, line); 
            if (line[0] != '\n') start += line.size();         
        } 

        while (start < end && start < stats.st_size) {
            getline(file, line);
            start += line.size() + 1;
            if (line[0] == '#') continue;
            fileAppPacket pckt;
            std::stringstream ss(line);
            ss >> pckt.dst >> pckt.src;
            pckt.weight = g_val(g_generator);
            pckt.dst--;
            pckt.src--;
            pckt.type = STORE;
            fileSelector->send(0, pckt, pckt.src % THREADS);
            pckt.type = NO_STORE;
            pckt.weight = g_val(g_generator);
            fileSelector->send(0, pckt, pckt.dst % THREADS);
        }
        file.close();
        fileSelector->done(0);
    });
}

void GRAPH::LOAD_GRAPH() {
    CONFIGURATION *cfg = this->cfg;
    this->ALLOCATE_GRAPH();
    if(cfg->scale_ == 0) {
        this->READ_GRAPH();
    }
    else {
        this->GENERATE_GRAPH();
    }
    this->generateWeightsLC();
    this->STATS_OF_FILE();
}

void GRAPH::STATS_OF_FILE() {
    uint64_t local_nodes = G->size();
    uint64_t num_nodes = lgp_reduce_add_l(local_nodes);
    global_num_nodes = num_nodes;
    T0_fprintf(stderr, "Total Number of Nodes in G: %llu\n", num_nodes);
    
    if(cfg->k >= num_nodes) {
        report("[REPORT_INFLUENCER_ERROR_CODE]:Please provide reasonable number of influencers, default = V(G)/2");
    }
    
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
        for(auto edge: *(x.second)) {
            ASSERT_WITH_MESSAGE(edge.first < global_num_nodes, std::to_string(edge.first));
        }
    }
}

void GRAPH::generateWeightsLC() {
    for(auto u: *G) {
        double total_weight = g_val(g_generator);
        for(auto edge: *(u.second)) {
            total_weight += edge.second; 
        }
        for(auto edge = u.second->begin(); edge != u.second->end(); edge++) {
            edge->second /= total_weight; 
        }
    }
}