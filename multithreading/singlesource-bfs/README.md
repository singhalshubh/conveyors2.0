### APP - Single Source Multi-graph BFS
`main_01.cpp` is the APP which allows parallelism conversion from PE level to thread level.

`main_02.cpp` is the APP which keeps data/PE a constant.

`main_03.cpp` is the APP which can only be used by HCLIB_WORKER=1. This does not contain any hclib::async mechanism.

### Directory
.
├── configuration.h (CLI for APP)
├── generateRR_3.h (for no hclib::async() in message handlers)
├── generateRR.h (WITH hclib::async() in message handlers)
├── graph.h
└── utility.h
├── main_01.cpp
├── main_02.cpp
├── main_03.cpp
├── Makefile
├── run.sh (SBATCH script)
├── plot.sh (plots graph for main02 and main03)
├── test.py (plots graph for main02 and main03)