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

        void process(VERTEX appPkt, int sender_rank) {            
            // there is a tag which is will be used first to locate the BFS record of that TAG.
            // if vertex is found, don't insert else insert.
            // This requires numOfSources BFS mulit-sources, which can be done all in parallel with guaranteed mutual exclusion. 
            int _checkpoint[10] = {-1};
            hclib::finish([=, &_checkpoint] {
                hclib::async([=, &_checkpoint] {  _checkpoint[0] = (*_g_list)[0]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[1] = (*_g_list)[1]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[2] = (*_g_list)[2]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[3] = (*_g_list)[3]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[4] = (*_g_list)[4]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[5] = (*_g_list)[5]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[6] = (*_g_list)[6]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[7] = (*_g_list)[7]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[8] = (*_g_list)[8]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[9] = (*_g_list)[9]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[10] = (*_g_list)[10]->insertIntoVisited(appPkt); });
            });
            bool res = false;
            for(int tracker = 0; tracker < _g_list->size(); tracker++) {
                res |= _checkpoint[tracker];
            }
            if(res) {nextFrontier->push(appPkt);}
        }

        public : void DO_ITR_LEVEL_ASYNC() {
            while(!currentFrontier->empty()) {
                VERTEX u = currentFrontier->front();
                currentFrontier->pop();
                if(*phase == ROOT_V) {
                    if(u % THREADS == MYTHREAD) {
                        send(0, u, MYTHREAD);
                    }
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
        std::queue<VERTEX>*_nextFrontier, int *_phase): 
            hclib::Selector<1, VERTEX>(true), _g_list(g_list), currentFrontier(_currentFrontier), 
            nextFrontier(_nextFrontier), phase(_phase) {
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

        void PERFORM_GENERATERR(std::vector<GRAPH*>*_g_list) {
            for(auto graph: *_g_list) {
                for(auto v: *(graph->G)) {
                    currentFrontier->push(v.first);
                }
            }
            int phase = ROOT_V; // phase ->0 indicates that phase 0 is simple exchange phase.
            uint64_t OR_VAL = 1;
            while(OR_VAL == 1) {
                RRSelector rrselector(_g_list, currentFrontier, nextFrontier, &phase);
                hclib::finish([&rrselector] {
                    rrselector.DO_ITR_LEVEL_ASYNC();
                    rrselector.done(0);
                });
                #ifdef DEBUG
                    uint64_t tot_size = lgp_reduce_add_l(nextFrontier->size());
                    T0_fprintf(stderr, "Size of next Frontier (total for all pes): %ld\n", tot_size);
                #endif
                uint64_t val = nextFrontier->size() > 0 ? 1:0;
                OR_VAL = lgp_reduce_max_l(val);
                std::swap(currentFrontier, nextFrontier);
                phase = NO_ROOT_V;
            }
        }

        void DELETE_GENERATERR() {
            delete nextFrontier;
            delete currentFrontier; 
        }
};