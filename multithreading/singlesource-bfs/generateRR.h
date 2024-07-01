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

#define LEVEL 4

class RRSelector: public hclib::Selector<1, VERTEX> {
        std::vector<GRAPH*>*_g_list;
        std::queue<VERTEX>*currentFrontier;

        void process(VERTEX appPkt, int sender_rank) {            
            // there is a tag which is will be used first to locate the BFS record of that TAG.
            // if vertex is found, don't insert else insert.
            // This requires numOfSources BFS mulit-sources, which can be done all in parallel with guaranteed mutual exclusion. 
            int _checkpoint[4] = {-1};
            hclib::finish([=, &_checkpoint] {
                hclib::async([=, &_checkpoint] {  _checkpoint[0] = (*_g_list)[0]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[1] = (*_g_list)[1]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[2] = (*_g_list)[2]->insertIntoVisited(appPkt); });
                hclib::async([=, &_checkpoint] {  _checkpoint[3] = (*_g_list)[3]->insertIntoVisited(appPkt); });
            });
            while(_checkpoint[0] == -1 || _checkpoint[1] == -1 ||
                _checkpoint[2] == -1 || _checkpoint[3] == -1);
        }

        public : void DO_BFS_ASYNC() {
            for(int level = 0; level < LEVEL; level++) {
                uint64_t _size = 0;
                for(auto graph: *_g_list) {
                    for(auto v: *(graph->G)) {
                        if(val(generator) <= 0.01*(level+1)) {
                            currentFrontier->push(v.first);
                            _size++;
                        }
                    }
                }
                T0_fprintf(stderr, "Number of root vertices: %ld, level = %ld\n", _size, level);
                while(!currentFrontier->empty()) {
                    VERTEX u = currentFrontier->front();
                    currentFrontier->pop();
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
    RRSelector(std::vector<GRAPH*>*g_list, std::queue<VERTEX>*_currentFrontier): 
            hclib::Selector<1, VERTEX>(true), _g_list(g_list), currentFrontier(_currentFrontier) {
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
        }

        void PERFORM_GENERATERR(std::vector<GRAPH*>*_g_list) {
            RRSelector rrselector(_g_list, currentFrontier);
            hclib::finish([&rrselector] {
                rrselector.DO_BFS_ASYNC();
                rrselector.done(0);
            });   
        }

        void DELETE_GENERATERR() {
            delete currentFrontier; 
        }
};