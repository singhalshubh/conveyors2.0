SRUN ?= oshrun

TARGETS=memcpy

all: clean $(TARGETS)

%: %.cpp
	$(CXX) -g -O3 -std=c++11 -o $@ $^ -lpthread

clean:
	rm -f $(TARGETS)
