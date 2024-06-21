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

static pthread_mutex_t generateRR_LOCK = PTHREAD_MUTEX_INITIALIZER;

class RRSelector: public hclib::Selector<1, IMMpckt> {
        GRAPH *graph;
        std::queue<IMMpckt>*currentFrontier;
        std::queue<IMMpckt>*nextFrontier;
        std::unordered_map<VERTEX, std::set<TAG>*> *_LOCALE_visited;
        int *phase;

        void insertIntoVisited_0(IMMpckt appPkt) {
            std::set<TAG>* s = new std::set<TAG>;
            s->insert(appPkt.tag);
            pthread_mutex_lock(&generateRR_LOCK);
            _LOCALE_visited->insert(std::make_pair(appPkt.dest_vertex, s));
            nextFrontier->push(appPkt);
            pthread_mutex_unlock(&generateRR_LOCK);
        }

        void insertIntoVisited_1(IMMpckt appPkt) {
            pthread_mutex_lock(&generateRR_LOCK);
            std::unordered_map<VERTEX, std::set<TAG>*>::iterator it = _LOCALE_visited->find(appPkt.dest_vertex);
            it->second->insert(appPkt.tag);
            nextFrontier->push(appPkt);
            pthread_mutex_unlock(&generateRR_LOCK);
        }

        void process(IMMpckt appPkt, int sender_rank) {            
            int case_ = -1;
            std::unordered_map<VERTEX, std::set<TAG>*>::iterator it = _LOCALE_visited->find(appPkt.dest_vertex);
            if(it == _LOCALE_visited->end()) {
                hclib::finish([=] {
                    hclib::async([=]{
                        insertIntoVisited_0(appPkt);
                    });
                });
            }
            else if(it->second->find(appPkt.tag) == it->second->end()) {
                hclib::finish([=] {
                    hclib::async([=]{ 
                        insertIntoVisited_1(appPkt);
                    });
                });
            }
        }

        public : void DO_ITR_LEVEL_ASYNC() {
            while(!currentFrontier->empty()) {
                IMMpckt appPkt = currentFrontier->front();
                currentFrontier->pop();
                if(*phase == ROOT_V) {
                    send(0, appPkt, appPkt.dest_vertex % THREADS);
                }
                else {
                    #ifdef DEBUG
                        assert(*phase == NO_ROOT_V);
                    #endif
                    EDGE *destAndWeight = graph->G->find(appPkt.dest_vertex)->second;
                    for(auto x: *destAndWeight) {
                        if(val(generator) < 0.01) {
                            appPkt.dest_vertex = x.first;
                            send(0, appPkt, appPkt.dest_vertex%THREADS);
                        }
                    }
                }
            }
        }

public:
    RRSelector(GRAPH *_graph, std::queue<IMMpckt>*_currentFrontier, 
        std::queue<IMMpckt>*_nextFrontier,std::unordered_map<VERTEX, std::set<TAG>*> *LOCALE_visited, int *_phase): 
            hclib::Selector<1, IMMpckt>(true), graph(_graph), currentFrontier(_currentFrontier), 
            nextFrontier(_nextFrontier), _LOCALE_visited(LOCALE_visited), phase(_phase) {
        mb[0].process = [this](IMMpckt appPkt, int sender_rank) { this->process(appPkt, sender_rank); };
    }
};

class GENERATE_RRR {
    private:
        std::queue<IMMpckt>*currentFrontier;
        std::queue<IMMpckt>*nextFrontier;
        std::unordered_map<VERTEX, std::set<TAG>*> *_LOCALE_visited;

    public:
        GENERATE_RRR() {
            currentFrontier = new std::queue<IMMpckt>;
            nextFrontier = new std::queue<IMMpckt>;
            _LOCALE_visited = new std::unordered_map<VERTEX, std::set<TAG>*>;
        }

        inline void FILL(GRAPH *G, uint64_t BATCH_SIZE) {
            /*  dest vertex is chosen at ranmdomly and therefore has to conform with application IMM standards, 
                which makes it global. We will continue supporting global dest_vertex which should not interfere
                with AGL in any sense.
            */
            trng::uniform_int_dist start(0, G->global_num_nodes);
            for(uint64_t tracker = 0; tracker < BATCH_SIZE; tracker++) {
                IMMpckt sendpckt;
                sendpckt.tag = MYTHREAD*BATCH_SIZE + tracker;
                sendpckt.dest_vertex = start(generator);
                currentFrontier->push(sendpckt);
            }
        }

        inline void FLUSH_AND_CLEAR(uint64_t offset, std::unordered_map<VERTEX, std::set<TAG>*> * visited) {
            for(auto vertex_tags: *_LOCALE_visited) {
                std::unordered_map<VERTEX, std::set<TAG>*>::iterator it = visited->find(vertex_tags.first);
                std::set<TAG>*s;
                if(it == visited->end()) {
                    s = new std::set<TAG>;
                    set_COPY(vertex_tags.second->begin(), vertex_tags.second->end(), s, offset);
                    visited->insert(std::make_pair(vertex_tags.first, s));
                }
                else {
                    set_COPY(vertex_tags.second->begin(), vertex_tags.second->end(), it->second, offset);
                }
                vertex_tags.second->clear();
            }
        }

        void PERFORM_GENERATERR(GRAPH *G, std::unordered_map<VERTEX, std::set<TAG>*> *visited, uint64_t BATCH_SIZE, uint64_t offset) {
            FILL(G, BATCH_SIZE);
            int phase = ROOT_V; // phase ->0 indicates that phase 0 is simple exchange phase.
            uint64_t OR_VAL = 1;
            while(OR_VAL == 1) {
                RRSelector rrselector(G, currentFrontier, nextFrontier, _LOCALE_visited, &phase);
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
            FLUSH_AND_CLEAR(offset, visited);
        }

        inline void CLEAR_GENERATERR() {
            #ifdef DEBUG
                assert(nextFrontier->empty() == true);
                assert(currentFrontier->empty() == true);
            #endif
        }

        void DELETE_GENERATERR() {
            delete nextFrontier;
            delete currentFrontier;
            delete _LOCALE_visited; 
        }
};