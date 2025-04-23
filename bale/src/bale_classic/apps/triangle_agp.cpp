#include <math.h>
#include <shmem.h>
extern "C" {
#include <spmat.h>
}

#define THREADS shmem_n_pes()
#define MYTHREAD shmem_my_pe()


typedef struct pkg_tri_t {
    int64_t w;    
    int64_t vj;
} pkg_tri_t;

int64_t binary_search(int64_t l_, int64_t r_, int64_t val_, sparsemat_t* mat_) { // custom binary search function
    while (l_ <= r_) {
        int m_ = l_ + (r_ - l_) / 2;
        if (mat_->lnonzero[m_] == val_) { return 1; }
        if (mat_->lnonzero[m_] < val_) { l_ = m_ + 1; } else { r_ = m_ - 1; }
    }
    return 0;
}

static int64_t copied_tri_convey_push_process(int64_t* c, convey_t* conv, sparsemat_t* mat, int64_t done) {
    int64_t k, cnt = 0;
    struct pkg_tri_t pkg;
    while(1) {
        int ret = convey_pull(conv, &pkg, NULL);
        if(ret == convey_OK) {
            cnt += binary_search(mat->loffset[pkg.vj], mat->loffset[pkg.vj + 1] - 1, pkg.w, mat); 
        }
    }
    *c += cnt;
    return convey_advance(conv, done);
}

double triangle_agp(int64_t *count, int64_t *sr, sparsemat_t * L, sparsemat_t * U, int64_t alg) {
    int64_t cnt=0;
    int64_t numpulled=0;
    int64_t l_i, ii, k, kk, w, L_i, L_j;
    double t1 = wall_seconds();
    if(!L){ T0_printf("ERROR: triangle_agp: NULL L!\n"); return(-1); }

    for(l_i = 0; l_i < L->lnumrows; l_i++){ 
        for(k = L->loffset[l_i] + 1; k < L->loffset[l_i + 1]; k++) {
            L_i = l_i*THREADS + MYTHREAD;
            L_j = L->lnonzero[k];
            assert( L_j < L_i );

            // NOW: get L[L_j,:] and count intersections with L[L_i,:]
            int64_t start = L->loffset[l_i];
            int64_t kbegin = lgp_get_int64(L->offset,L_j); 
            int64_t kend   = lgp_get_int64(L->offset, L_j + THREADS);
            numpulled+=2;
            for( kk = kbegin; kk < kend; kk++){
                w = lgp_get_int64(L->nonzero, L_j%THREADS + kk*THREADS);
                numpulled++;
                assert( w < L_j );
                for(ii = start; ii < L->loffset[l_i + 1]; ii++) {
                    if( w ==  L->lnonzero[ii] ){ 
                        cnt++;
                        start = ii + 1;
                        break;
                    }
                    if( w < L->lnonzero[ii] ){ // the rest are all bigger because L is tidy
                        start = ii;
                        break;
                    }
                }
            }
        }
    }

    lgp_barrier();
    minavgmaxD_t stat[1];
    t1 = wall_seconds() - t1;
    lgp_min_avg_max_d( stat, t1, THREADS );

    *sr = numpulled; 
    *count = cnt;
    return(stat->avg);
}

int main(int argc, char* argv[]) {

    lgp_init(argc, argv);

    int64_t buf_cnt = 1024;
    int64_t models_mask = 8;  // default is running all models
    int64_t l_numrows = 10000;         // number of a rows per thread
    int64_t nz_per_row = 35;           // target number of nonzeros per row (only for Erdos-Renyi)
    int64_t read_graph = 0L;           // read graph from a file
    char filename[64];
    int64_t cores_per_node = 0;
    
    double t1;
    int64_t i, j;
    int64_t alg = 0;
    int64_t gen_kron_graph = 0L;
    int kron_graph_mode = 0;
    char * kron_graph_string;
    double erdos_renyi_prob = 0.0;

    int printhelp = 0;
    int opt; 
    while ((opt = getopt(argc, argv, "hb:c:M:n:f:a:e:K:t:")) != -1) {
        switch (opt) {
            case 'h': printhelp = 1; break;
            case 'b': sscanf(optarg,"%ld", &buf_cnt);  break;
            case 'c': sscanf(optarg,"%ld" ,&cores_per_node); break;
            case 'M': sscanf(optarg,"%ld", &models_mask);  break;
            case 'n': sscanf(optarg,"%ld", &l_numrows); break;
            case 'f': read_graph = 1; sscanf(optarg,"%s", filename); break;

            case 'a': sscanf(optarg,"%ld", &alg); break;
            case 'e': sscanf(optarg,"%lg", &erdos_renyi_prob); break;
            case 'K': gen_kron_graph = 1; kron_graph_string = optarg; break;
            default:  break;
        }
    }

    // if (printhelp) usage(); // Skipping print help
    int64_t numrows = l_numrows * THREADS;
    if (erdos_renyi_prob == 0.0) { // use nz_per_row to get erdos_renyi_prob
        erdos_renyi_prob = (2.0 * (nz_per_row - 1)) / numrows;
        if (erdos_renyi_prob > 1.0) erdos_renyi_prob = 1.0;
    } else {                     // use erdos_renyi_prob to get nz_per_row
        nz_per_row = erdos_renyi_prob * numrows;
    }

    T0_fprintf(stderr,"Running triangle on %d threads\n", THREADS);
    if (!read_graph && !gen_kron_graph) {
        T0_fprintf(stderr,"Number of rows per thread   (-N)   %ld\n", l_numrows);
        T0_fprintf(stderr,"Erdos Renyi prob (-e)   %g\n", erdos_renyi_prob);
    }

    T0_fprintf(stderr,"Model mask (M) = %ld (should be 1,2,4,8,16 for agi, exstack, exstack2, conveyors, alternates\n", models_mask);  
    T0_fprintf(stderr,"algorithm (a) = %ld (0 for L & L*U, 1 for L & U*L)\n", alg);

    double correct_answer = -1;

    sparsemat_t *A, *L, *U;

    L = erdos_renyi_random_graph(numrows, erdos_renyi_prob, UNDIRECTED, NOLOOPS, 12345);
    

    lgp_barrier();
    if (alg == 1)
        U = transpose_matrix(L);   

    lgp_barrier();

    T0_fprintf(stderr, "L has %ld rows/cols and %ld nonzeros.\n", L->numrows, L->nnz);
    int64_t max_degree = 0;
    for (int l_i = 0; l_i < L->lnumrows; l_i++) {
        int64_t degree = L->loffset[l_i + 1] - L->loffset[l_i] + 1;
        if (degree > max_degree) {
            max_degree = degree;
        }
    }
    fprintf(stderr, "[PE%d] L has local %ld nonzeros (max_degree=%ld).\n", MYTHREAD, L->lnnz, max_degree);

    if (!is_lower_triangular(L, 0)) {
        T0_fprintf(stderr,"ERROR: L is not lower triangular!\n");
        assert(false);
    }

    T0_fprintf(stderr, "Run triangle counting ...\n");
    int64_t tri_cnt;           // partial count of triangles on this thread
    int64_t total_tri_cnt;     // the total number of triangles on all threads
    int64_t sh_refs;         // number of shared reference or pushes
    int64_t total_sh_refs;

    int64_t* cc = (int64_t*)lgp_all_alloc(L->numrows, sizeof(int64_t));
    int64_t* l_cc = lgp_local_part(int64_t, cc);
    for (i = 0; i < L->lnumrows; i++)
        l_cc[i] = 0;
        
    lgp_barrier();

    /* calculate col sums */
    for (i = 0; i < L->lnnz; i++) {
        long lindex = L->lnonzero[i] / THREADS;
        long pe = L->lnonzero[i] % THREADS;
        lgp_fetch_and_inc(&cc[lindex], pe);
    }

    lgp_barrier();

    int64_t rtimesc_calc = 0;
    for (i = 0; i < L->lnumrows; i++) {
        int64_t deg = L->loffset[i + 1] - L->loffset[i];        
        rtimesc_calc += deg * l_cc[i];
    }

    /* calculate sum (r_i choose 2) */
    int64_t rchoose2_calc = 0;
    for (i = 0; i < L->lnumrows; i++) {
        int64_t deg = L->loffset[i + 1] - L->loffset[i];
        rchoose2_calc += deg * (deg - 1) / 2;
    }

    /* calculate sum (c_i choose 2) */
    int64_t cchoose2_calc = 0;
    for (i = 0; i < L->lnumrows; i++) {
        int64_t deg = l_cc[i];
        cchoose2_calc += deg * (deg - 1) / 2;
    }
    
    int64_t pulls_calc = 0;
    int64_t pushes_calc = 0;
    if (alg == 0) {
        pulls_calc = lgp_reduce_add_l(rtimesc_calc);
        pushes_calc = lgp_reduce_add_l(rchoose2_calc);
    } else {
        pushes_calc = lgp_reduce_add_l(rtimesc_calc);
        pulls_calc = lgp_reduce_add_l(cchoose2_calc);
    }

    lgp_all_free(cc);

    T0_fprintf(stderr,"Calculated: Pulls = %ld\n            Pushes = %ld\n\n", pulls_calc, pushes_calc);

    int64_t use_model;
    double laptime = 0.0;

    tri_cnt = 0;
    total_tri_cnt = 0;
    sh_refs = 0;
    total_sh_refs = 0;

    // only running selector model
    T0_fprintf(stderr, "Running AGP set-intersection method: \n");
    laptime = triangle_agp(&tri_cnt, &sh_refs, L, U, alg);
    lgp_barrier();

    total_tri_cnt = lgp_reduce_add_l(tri_cnt);
    total_sh_refs = lgp_reduce_add_l(sh_refs);
    T0_fprintf(stderr, "  %8.3lf seconds: %16ld triangles\n", laptime, total_tri_cnt);
    //  uint64_t tot_push_cycles = lgp_reduce_add_l(push_cycles)/THREADS;
    // uint64_t tot_pull_cycles = lgp_reduce_add_l(pull_cycles)/THREADS;
    // uint64_t tot_advance_cycles = lgp_reduce_add_l(advance_cycles)/THREADS;
    // uint64_t tot_cycles = lgp_reduce_add_l(total_cycles)/THREADS;
    // T0_fprintf(stderr, "PAPI push cycles: %lld\n", tot_push_cycles);
    // T0_fprintf(stderr, "PAPI pull cycles: %lld\n", tot_pull_cycles);
    // T0_fprintf(stderr, "PAPI advance cycles: %lld\n", tot_advance_cycles);
    // T0_fprintf(stderr, "PAPI histo cycles: %lld\n", tot_cycles);
    //T0_fprintf(stderr, "%16ld shared refs\n", total_sh_refs);
    if ((correct_answer >= 0) && (total_tri_cnt != (int64_t)correct_answer)) {
        T0_fprintf(stderr, "ERROR: Wrong answer!\n");
    }

    if(correct_answer == -1) {
        correct_answer = total_tri_cnt;
    }

    lgp_barrier();
    lgp_finalize();
    return 0;
}

