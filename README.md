# Actor Graph Library (AGL)

AGL is designed to ease writing graph actor-based applications. It provides functionality to load, generate, build, and distribute graphs. That way, users can focus on what they want their graph app to do, and not worry about building a graph, which is often a significant amount of coding effort if starting from scratch. It is built on top of [hclib](https://github.gatech.edu/Habanero/hclib-actor).

### Main Features
* _Load graphs from file_ in parallel across PEs. File should be a simple edge list with one edge per line, as `source  dest`.
* _Generate synthetic graphs_ both RMAT or uniform random
* _Builds distributed graph_ given edges from a file or the synthetic generator
* _Distributed CSR_ data structure provides convenient access to graph topology with iterators
* _Customized distrubtion methods_ to allocate which graph nodes are on which PEs. We recommend `Auto` for the mapper, which uses XOR for power of 2 number of PEs and cyclic otherwise.
* _Command-line interface parser_ eases making new kernels, so users can include the object, and then get CLI arguments to call the generator or load from a file.

### Features in Development
* Migrating main home of this repo to github.com (will continue to mirror to GT GitHub)
* Development: a stateful iterator that eases pausing and resuming 
* Research: exploring mapping operations via data structures instead of only hash functions (current)
* Research: exploring distributed determination of good data placements and minimizing shuffling to achieve it

### Contributors
* Scott Beamer
* Tanuj Gupta
* Nishant Khanorkar
* Amogh Lonkar
* Vincent Titterton
