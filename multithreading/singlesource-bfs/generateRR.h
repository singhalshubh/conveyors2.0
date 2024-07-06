#include <cassert>
#include <pthread.h>
#define ASSERT_WITH_MESSAGE(condition, message)\
   (!(condition)) ?\
      (std::cerr << "Assertion failed: (" << #condition << "), "\
      << "function " << __FUNCTION__\
      << ", file " << __FILE__\
      << ", line " << __LINE__ << "."\
      << std::endl << message << std::endl, abort(), 0) : 1

trng::lcg64 generator;
trng::uniform01_dist<float> val;

#define ROOT_V 0
#define NO_ROOT_V 1

class RRSelector: public hclib::Selector<1, VERTEX> {
        std::vector<GRAPH*>*_g_list;
        std::queue<VERTEX>*currentFrontier;
        std::queue<VERTEX>*nextFrontier;
        int *phase;
        uint64_t *NUMBER_OF_PULLS;
        std::vector<bool> *_checkpoint;

        void process(VERTEX appPkt, int sender_rank) {
            //(*NUMBER_OF_PULLS)++;
            hclib::finish([=] {
                uint64_t nChunks = _g_list->size()/hclib_get_num_workers() + 1;
                for(uint64_t block = 0; block < nChunks; block++) {
                    // Inside each block we have 
                    hclib::async([=] {  
                        uint64_t start = nChunks*block;
                        uint64_t end = std::min(start + nChunks, _g_list->size());
                        for (uint64_t tracker = start; tracker < end; tracker++) {
                            (*_checkpoint)[hclib_get_current_worker()] = (*_checkpoint)[hclib_get_current_worker()] | (*_g_list)[tracker]->insertIntoVisited(appPkt); 
                        }
                    });
                 }
            });
            bool res = false;
            for(int tracker = 0; tracker < _checkpoint->size(); tracker++) {
                res |= (*_checkpoint)[tracker];
                (*_checkpoint)[tracker] = false;
            }
            if(res) {nextFrontier->push(appPkt);}
        }

        public : void DO_ITR_LEVEL_ASYNC() {
            while(!currentFrontier->empty()) {
                VERTEX u = currentFrontier->front();
                currentFrontier->pop();
                if(*phase == ROOT_V) {
                    send(0, u, u%THREADS);
                }
                else {
                    #ifdef DEBUG
                        assert(*phase == NO_ROOT_V);
                    #endif
                    for(auto graph: *_g_list) {
                        if(graph->G->find(u) != graph->G->end()) {
                            EDGE *vertex_set = graph->G->find(u)->second;
                            for(auto vertex: *vertex_set) {
                                send(0, vertex, vertex%THREADS);
                            }
                        }
                    }
                }
            }
        }

public:
    RRSelector(std::vector<GRAPH*>*g_list, std::queue<VERTEX>*_currentFrontier, 
        std::queue<VERTEX>*_nextFrontier, int *_phase, uint64_t *_NUMBER_OF_PULLS, std::vector<bool> *checkpoint): 
            hclib::Selector<1, VERTEX>(true), _g_list(g_list), currentFrontier(_currentFrontier), 
            nextFrontier(_nextFrontier), phase(_phase), NUMBER_OF_PULLS(_NUMBER_OF_PULLS), _checkpoint(checkpoint) {
        mb[0].process = [this](VERTEX appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

class GENERATE_RRR {
    private:
        std::queue<VERTEX>*currentFrontier;
        std::queue<VERTEX>*nextFrontier;

    public:
        GENERATE_RRR() {
            currentFrontier = new std::queue<VERTEX>;
            nextFrontier = new std::queue<VERTEX>;
        }

        void PERFORM_GENERATERR(std::vector<GRAPH*>*_g_list, CONFIGURATION *cfg) {
            if(MYTHREAD == 0) {
                for(auto graph: *_g_list) {
                    uint64_t max_size = 0;
                    uint64_t vertex;
                    for(auto v: *(graph->G)) {
                        if(max_size < v.second->size()) {
                            max_size = v.second->size();
                            vertex = v.first;
                        }
                    }
                    currentFrontier->push(vertex);
                    break;
                }
            }
            lgp_barrier();
            double t1 = wall_seconds();
            int phase = ROOT_V; // phase ->0 indicates that phase 0 is simple exchange phase.
            uint64_t OR_VAL = 1;
            uint64_t TOTAL_PULLS = 0;
            std::vector<bool> *_checkpoint = new std::vector<bool>;
            for(int i = 0; i < hclib_get_num_workers(); i++) {
                _checkpoint->push_back(false);
            } 
            while(OR_VAL == 1) {
                uint64_t *NUMBER_OF_PULLS = new uint64_t;
                *NUMBER_OF_PULLS = 0;
                RRSelector rrselector(_g_list, currentFrontier, nextFrontier, &phase, NUMBER_OF_PULLS, _checkpoint);
                hclib::finish([&rrselector] {
                    rrselector.DO_ITR_LEVEL_ASYNC();
                    rrselector.done(0);
                });
                TOTAL_PULLS += lgp_reduce_add_l(*NUMBER_OF_PULLS);
                #ifdef DEBUG
                    uint64_t tot_size = lgp_reduce_add_l(nextFrontier->size());
                    T0_fprintf(stderr, "Size of next Frontier (total for all pes): %ld\n", tot_size);
                #endif
                uint64_t val = nextFrontier->size() > 0 ? 1:0;
                OR_VAL = lgp_reduce_max_l(val);
                std::swap(currentFrontier, nextFrontier);
                phase = NO_ROOT_V;
            }
            lgp_barrier();
            if(MYTHREAD == 0) {
                FILE *fp = fopen(cfg->filename, "a");
                fprintf(fp, "%f\n", wall_seconds() - t1);
                //fprintf(fp, "%ld\n", TOTAL_PULLS/THREADS);
                fclose(fp);
            }
            lgp_barrier();
        }

        void DELETE_GENERATERR() {
            delete nextFrontier;
            delete currentFrontier; 
        }
};