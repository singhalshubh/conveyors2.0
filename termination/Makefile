SRUN ?= oshrun

TARGETS= clean main_01

all: $(TARGETS)

%: %.cpp
	${CXX} -g -O3 -std=c++17 -DUSE_SHMEM=1 -ftrapv  -I$(BALE_INSTALL)/include -L$(BALE_INSTALL)/lib -o $@ $^ -lconvey -llibgetput -lspmat -lexstack -lm

clean:
	rm -f $(TARGETS)