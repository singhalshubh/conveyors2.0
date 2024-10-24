/*
Habanero Labs, US
Shubhendra Pal Singhal 2024
*/

#include <math.h>
#include <shmem.h>
#include <std_options.h>
#include <string>
#include <sys/time.h>
#include <endian.h>
#include <ctime> 
#include <cstdlib> 
#include "selector.h"
#include <random>
#include <cmath>
#include <cstddef>
#include <limits>
#include <atomic>
#include <cassert>
#include <iostream>

typedef std::atomic<std::uint64_t> atomic_uint64_t;
#define DEBUG 1

#if DEBUG
#define SYS_DEBUG
#endif

enum STATUS {
    INIT = 0,
    PUSH_DONE = 1,
    ENDGAME = 2
};

#include "utility.h"
#include "porter.h"

/* This program is blocking barriers coded in such a way that:
    Inputs: done() invocation from the user.
    Process: This program does not ensure flow of data. It just 
            simulates the done function from Conveyors. 
            1D, 2D, 3D topology done supported.
    Output: done((Porter)_i) -> done((Porter)_i+1) -> done((Porter)_i+2) -> done((Porter)_i+3)....
*/

class CONVEYORS {
    public:
        uint64_t _LOCAL_NUM_PROCS;
        int _NUM_PORTERS;
        int *start_DONE;
        PORTER porter[3];

        void NEW();
        void BEGIN();
        bool DONE();
        void RESET();
        void DELETE();
};


void CONVEYORS::BEGIN() {
    _LOCAL_NUM_PROCS = PROCS_PER_NODE();
    if(THREADS <= _LOCAL_NUM_PROCS) {
        _NUM_PORTERS = 1;
        int64_t _SIZE = (int64_t) (THREADS < _LOCAL_NUM_PROCS ? THREADS : _LOCAL_NUM_PROCS);
        porter[0].BEGIN_INTRA(_LOCAL_NUM_PROCS, _SIZE);
    }
    else if(THREADS > _LOCAL_NUM_PROCS && THREADS*THREADS < std::pow(_LOCAL_NUM_PROCS, 5)) {
        _NUM_PORTERS = 2;
        int64_t _SIZE[2] = {(int64_t) (_LOCAL_NUM_PROCS), 
            (int64_t) (THREADS % _LOCAL_NUM_PROCS == 0 ? 
                THREADS / _LOCAL_NUM_PROCS : THREADS / _LOCAL_NUM_PROCS + 1)};
        porter[0].BEGIN_INTRA(_LOCAL_NUM_PROCS, _SIZE[0]);
        porter[1].BEGIN_INTER(_LOCAL_NUM_PROCS, _SIZE[1], _NUM_PORTERS);
    }
    else {
        _NUM_PORTERS = 3;
        int64_t _SIZE[2];
        _SIZE[0] = (int64_t)(_LOCAL_NUM_PROCS);
        _SIZE[1] = 1 + ((int64_t) (THREADS - 1) / (int64_t) (_LOCAL_NUM_PROCS * _LOCAL_NUM_PROCS));
        porter[0].BEGIN_INTRA(_LOCAL_NUM_PROCS, _SIZE[0]);
        porter[1].BEGIN_INTER(_LOCAL_NUM_PROCS, _SIZE[1], _NUM_PORTERS);
        porter[2].BEGIN_INTRA(_LOCAL_NUM_PROCS, _SIZE[0]);
    }
    start_DONE = new int;
    *start_DONE = 0;
}

bool CONVEYORS::DONE() {
    bool ENDGAME = true;
    for(int type = *start_DONE; type < _NUM_PORTERS; type++) {
        bool IS_DONE;
        if(type == 0 || type == 2) {
            IS_DONE = porter[type].DONE_INTRA(ENDGAME);
        }
        else {
            uint64_t identifier;
            if(_NUM_PORTERS == 2) {
                identifier = MYTHREAD/_LOCAL_NUM_PROCS;
            }
            else if(_NUM_PORTERS == 3) {
                identifier = MYTHREAD/(_LOCAL_NUM_PROCS*_LOCAL_NUM_PROCS);
            }
            IS_DONE = porter[type].DONE_INTER(ENDGAME, identifier);
        }
        if(IS_DONE == false) {
            ENDGAME = false;
        }
        else if(IS_DONE == true) {
            ENDGAME = true;
            *start_DONE = *start_DONE + 1;
            #ifdef SYS_DEBUG
                T0_fprintf(stderr, "One more dimension over\n");
            #endif
        }
    }
    if(*start_DONE == _NUM_PORTERS) {
        return true;
    }
    return false;
}

void CONVEYORS::DELETE() {
    for(int type = *start_DONE; type < _NUM_PORTERS; type++) {
        porter[type].DELETE();
    }
}