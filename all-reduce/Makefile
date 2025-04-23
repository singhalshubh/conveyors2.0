PAPI_ROOT=/usr/local/pace-apps/manual/packages/papi/7.1.0/usr/local
SRUN ?= oshrun

TARGETS= clean pe_data data_pe

all: $(TARGETS)

%: %.cpp
	${CXX} -g -O3 -std=c++11 -DUSE_SHMEM=1 -ftrapv  -I$(BALE_INSTALL)/include -L$(BALE_INSTALL)/lib -o $@ $^ -I${PAPI_ROOT}/include -L${PAPI_ROOT}/lib -lpapi -lconvey -llibgetput -lspmat -lexstack -lpfm -lm

clean:
	rm -f $(TARGETS)