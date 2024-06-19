#define BUFFER_BYTES 10000

class PORTER {
    public: 
        STATUS SYSTEM_STATE;
        uint64_t _LOCAL_NUM_PROCS;
        uint64_t *_IS_COMPLETE;
        uint64_t _MAX_LIMIT;
        /*DONE signals*/
        int64_t _SIZE;
        atomic_uint64_t** SIGNAL_PTRS;
        atomic_uint64_t* SIGNAL_BUFFERS;
        /*Data buffers*/
        uint64_t* DATA_BUFFERS_SEND;
        uint64_t* DATA_BUFFERS_RECV;
        uint64_t** DATA_PTRS;
        int64_t *NEIGH;

        /*We keep definition generic by keeping done signals as 64-bits, as conveyors use done signal from received signal
        which indicates the number of buffers sent through their channel*/

        void _ALLOCATE_NEIGH_INTRA();
        void _ALLOCATE_NEIGH_INTER(int _NUM_PORTERS);

        inline void _ALLOCATE_BUFFERS();
        
        void _ALLOCATE_PTRS();

        void BEGIN_INTRA(uint64_t LOCAL_NUM_PROCS, int64_t SIZE);
        void BEGIN_INTER(uint64_t LOCAL_NUM_PROCS, int64_t SIZE, int _NUM_PORTERS);

        void PUSH_INTRA();
        void PUSH_INTER(uint64_t identifier);
        
        bool PULL();
        
        bool DONE_INTRA(bool ENDGAME);
        bool DONE_INTER(bool ENDGAME, uint64_t identifier);
        
        void DELETE();
};


 void PORTER::_ALLOCATE_NEIGH_INTRA() {
    NEIGH = (int64_t *) calloc(_SIZE, sizeof(int64_t));
    #ifdef SYS_DEBUG
        assert(NEIGH != NULL);
    #endif
    for(uint64_t tracker = 0; tracker < _SIZE; tracker++) {
        NEIGH[tracker] = -1;
    }
    uint64_t row_LOCATOR = MYTHREAD / _LOCAL_NUM_PROCS;
    for(uint64_t tracker = 0; tracker < _SIZE; tracker++) {
        uint64_t dest_pe = tracker + _LOCAL_NUM_PROCS * row_LOCATOR;
        if(dest_pe >= THREADS) {
            _MAX_LIMIT = tracker;
            break;
        }
        NEIGH[tracker] = dest_pe;
    }
    _MAX_LIMIT = _SIZE;
}

 void PORTER::_ALLOCATE_NEIGH_INTER(int _NUM_PORTERS) {
    NEIGH = (int64_t *) calloc(_SIZE, sizeof(int64_t));
    #ifdef SYS_DEBUG
        assert(NEIGH != NULL);
    #endif
    for(uint64_t tracker = 0; tracker < _SIZE; tracker++) {
        NEIGH[tracker] = -1;
    }
    uint64_t col = MYTHREAD % _LOCAL_NUM_PROCS;
    if(_NUM_PORTERS == 2) { //matrix porters
        for(uint64_t tracker = 0; tracker < _SIZE; tracker++) {
            uint64_t dest_pe = tracker * _LOCAL_NUM_PROCS + col;
            if(dest_pe >= THREADS) {
                _MAX_LIMIT = tracker;
                break;
            }
            NEIGH[tracker] = dest_pe;
        }
        _MAX_LIMIT = _SIZE;
    }
    else if(_NUM_PORTERS == 3) {
        uint64_t row = MYTHREAD / _LOCAL_NUM_PROCS;
        uint64_t tag = (_LOCAL_NUM_PROCS * col) + (row % _LOCAL_NUM_PROCS);
        for (uint64_t tracker = 0; tracker < _SIZE; tracker++) {
            uint64_t dest_pe = (tracker * _LOCAL_NUM_PROCS*_LOCAL_NUM_PROCS) + tag;
            if(dest_pe >= THREADS) {
                _MAX_LIMIT = tracker;
                break;
            } 
            NEIGH[tracker] = dest_pe; 
        }
        _MAX_LIMIT = _SIZE;
    }
}

inline void PORTER::_ALLOCATE_BUFFERS() {
    SIGNAL_BUFFERS = (atomic_uint64_t *) shmem_calloc(_SIZE, sizeof(atomic_uint64_t));
    #ifdef SYS_DEBUG
        assert(SIGNAL_BUFFERS != NULL);
    #endif
    for(uint64_t tracker = 0; tracker < _SIZE; tracker++) {
        SIGNAL_BUFFERS[tracker] = 0;
    }

    DATA_BUFFERS_SEND = (uint64_t *) shmem_calloc(_SIZE*BUFFER_BYTES, sizeof(uint64_t));
    DATA_BUFFERS_RECV = (uint64_t *) shmem_calloc(_SIZE*BUFFER_BYTES, sizeof(uint64_t));
    #ifdef SYS_DEBUG
        assert(DATA_BUFFERS_SEND != NULL);
        assert(DATA_BUFFERS_RECV != NULL);
    #endif
}

void PORTER::_ALLOCATE_PTRS() {
    SIGNAL_PTRS = (atomic_uint64_t **) calloc(_SIZE, sizeof(atomic_uint64_t *));
    #ifdef SYS_DEBUG
        assert(SIGNAL_PTRS != NULL);
    #endif
    for(uint64_t tracker = 0; tracker < _SIZE; tracker++) {
        if(NEIGH[tracker] != -1) {
            SIGNAL_PTRS[tracker] = (atomic_uint64_t*) shmem_ptr(SIGNAL_BUFFERS, NEIGH[tracker]);
            #ifdef SYS_DEBUG
                std::string trace_msg = std::to_string(NEIGH[tracker]) + std::to_string(MYTHREAD);
                ASSERT_WITH_MESSAGE(SIGNAL_PTRS[tracker] != NULL, trace_msg);
            #endif
        }
    }

    DATA_PTRS = (uint64_t **) calloc(_SIZE, sizeof(uint64_t *));
    #ifdef SYS_DEBUG
        assert(DATA_PTRS != NULL);
    #endif
    for(uint64_t tracker = 0; tracker < _SIZE; tracker++) {
        if(NEIGH[tracker] != -1) {
            DATA_PTRS[tracker] = (uint64_t*) shmem_ptr(DATA_BUFFERS_RECV, NEIGH[tracker]);
            #ifdef SYS_DEBUG
                std::string trace_msg = std::to_string(NEIGH[tracker]) + std::to_string(MYTHREAD);
                ASSERT_WITH_MESSAGE(DATA_PTRS[tracker] != NULL, trace_msg);
            #endif
        }
    }
}

 void PORTER::BEGIN_INTRA(uint64_t LOCAL_NUM_PROCS, int64_t SIZE) {
    #ifdef SYS_DEBUG
        ASSERT_WITH_MESSAGE(SIZE > 0, std::to_string(MYTHREAD) + "SIZE = 0 was requested for a PE\n");
        ASSERT_WITH_MESSAGE(LOCAL_NUM_PROCS > 0, std::to_string(MYTHREAD) + "LOCAL_PROCS = 0 detected!\n");
    #endif
    _LOCAL_NUM_PROCS = LOCAL_NUM_PROCS;
    _SIZE = SIZE;
    _IS_COMPLETE = new uint64_t;
    *_IS_COMPLETE = 0;
    SYSTEM_STATE = STATUS::INIT;
    _ALLOCATE_BUFFERS();
    _ALLOCATE_NEIGH_INTRA();
    _ALLOCATE_PTRS();
    #ifdef SYS_DEBUG
        T0_fprintf(stderr, "Allocated Buffers 1D\n");
    #endif
}

 void PORTER::BEGIN_INTER(uint64_t LOCAL_NUM_PROCS, int64_t SIZE, int _NUM_PORTERS) {
    #ifdef SYS_DEBUG
        ASSERT_WITH_MESSAGE(SIZE > 0, std::to_string(MYTHREAD) + "SIZE = 0 was requested for a PE\n");
        ASSERT_WITH_MESSAGE(LOCAL_NUM_PROCS > 0, std::to_string(MYTHREAD) + "LOCAL_PROCS = 0 detected!\n");
    #endif
    _LOCAL_NUM_PROCS = LOCAL_NUM_PROCS;
    _SIZE = SIZE;
    _IS_COMPLETE = new uint64_t;
    *_IS_COMPLETE = 0;
    SYSTEM_STATE = STATUS::INIT;
    _ALLOCATE_BUFFERS();
    _ALLOCATE_NEIGH_INTER(_NUM_PORTERS);
    #ifdef SYS_DEBUG
        T0_fprintf(stderr, "Allocated Buffers 2D\n");
    #endif
}

 void PORTER::PUSH_INTRA() {
    uint64_t rank = MYTHREAD%_LOCAL_NUM_PROCS;
    for(uint64_t tracker = 0; tracker < _SIZE; tracker++) {
        if(NEIGH[tracker] != -1) {
            uint64_t signal = 1; // CAN PLUG in higher values which will imply signalling done!
            SIGNAL_PTRS[tracker][rank] = signal;
        }
    }
}

 void PORTER::PUSH_INTER(uint64_t identifier) {
    for(uint64_t tracker = 0; tracker < _SIZE; tracker++) {
        if(NEIGH[tracker] != -1) {
            uint64_t signal = 1; // CAN PLUG in higher values which will imply signalling done!
            shmem_put64((uint64_t *) (&SIGNAL_BUFFERS[identifier]), &signal, 1, NEIGH[tracker]);
        }
    }
}

 bool PORTER::PULL() {
    for(uint64_t tracker = 0; tracker < _SIZE; tracker++) {
        if(NEIGH[tracker] != -1) {
            uint64_t recv{};
            recv = SIGNAL_BUFFERS[tracker].load();
            if(recv == 1) {
                *_IS_COMPLETE = *_IS_COMPLETE + 1;
                NEIGH[tracker] = -1;
            }
        }
    }
    if(*_IS_COMPLETE == _MAX_LIMIT) {
        return true;
    }
    return false;
}

 bool PORTER::DONE_INTRA(bool ENDGAME) {
    if(ENDGAME) {
        if(SYSTEM_STATE == STATUS::INIT) {
            PUSH_INTRA();
            SYSTEM_STATE = STATUS::PUSH_DONE;
        }
        if(PULL() == false) {
            return false;
        }
        else {
            return true;
        }
    }
    return false;
}

 bool PORTER::DONE_INTER(bool ENDGAME, uint64_t identifier) {
    if(ENDGAME) {
        if(SYSTEM_STATE == STATUS::INIT) {
            PUSH_INTER(identifier);
            SYSTEM_STATE = STATUS::PUSH_DONE;
        }
        if(PULL() == false) {
            return false;
        }
        else {
            return true;
        }
    }
    return false;
}

 void PORTER::DELETE() {
    delete _IS_COMPLETE;
    shmem_free(SIGNAL_BUFFERS);
    if(SIGNAL_PTRS != NULL) {
        delete SIGNAL_PTRS;
    }
}