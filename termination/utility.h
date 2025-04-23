/*
Habanero Labs, US
Shubhendra Pal Singhal 2024
*/


inline UNIT_OF_TIME AVG(std::vector<UNIT_OF_TIME> *v) {
    return std::reduce(v->begin(), v->end()) / v->size();
}

inline UNIT_OF_TIME MAX(std::vector<UNIT_OF_TIME> *v) {
    return *std::max_element(v->begin(), v->end());
}

inline UNIT_OF_TIME MIN(std::vector<UNIT_OF_TIME> *v) {
    return *std::min_element(v->begin(), v->end());
}

UNIT_OF_TIME VAR(std::vector<UNIT_OF_TIME> *v) {
    UNIT_OF_TIME mean = AVG(v);
    uint64_t sz = v->size();
    auto variance_func = [&mean, &sz](UNIT_OF_TIME accumulator, const UNIT_OF_TIME& val) {
        return accumulator + ((val - mean)*(val - mean) / (sz - 1));
    };

    return std::accumulate(v->begin(), v->end(), 0.0, variance_func);
}

UNIT_OF_TIME MEDIAN(std::vector<UNIT_OF_TIME> *v) {
    std::sort(v->begin(), v->end());
    UNIT_OF_TIME val;
    if(v->size() % 2 != 0) { // odd
        uint64_t pos = v->size()/2;
        val = (*v)[pos];
    }
    else if(v->size() % 2 == 0 && v->size() >= 2){
        uint64_t pos = v->size()/2;
        val = ((*v)[pos-1] + (*v)[pos])/2;
    }
    else {
        val = (*v)[0];
    }
    return val;
}

#define ASSERT_WITH_MESSAGE(condition, message)\
   (!(condition)) ?\
      (std::cerr << "Assertion failed: (" << #condition << "), "\
      << "function " << __FUNCTION__\
      << ", file " << __FILE__\
      << ", line " << __LINE__ << "."\
      << std::endl << message << std::endl, abort(), 0) : 1

static inline uint64_t _rdtsc(void) {
    unsigned a, d;
    asm volatile("rdtsc" : "=a" (a), "=d" (d) : : "%rbx", "%rcx");
    return ((uint64_t) a) | (((uint64_t) d) << 32);
}

