COMMON_OBJS = timer.o dummy.o
DBG      ?=
CXX      ?= g++
CXXFLAGS  = -O3 -I. -std=c++11 -I$(COMMON) $(DBG)

NVCC	= nvcc
NVFLAGS	= -O3 -g -I. -std=c++11 -I$(COMMON) -arch sm_70 -g --resource-usage -O3

ACC	    = nvc++
ACCFLAGS= -O3 -g -I. -std=c++11 -I$(COMMON) -acc -gpu=cc70 -g --restrict
ACCFLAGS+= -Minfo=acc

LD		= $(ACC)
#LDFLAGS = -lcudart $(ACCFLAGS)
LDFLAGS = $(ACCFLAGS)
#LIBS	= -lcublas #-lcblas -lopenblas
LIBS	= -cudalib=cublas -lstdc++

ifeq ($(CXX),icpc)
  CXXFLAGS += -xHost #-no-vec
  CXXFLAGS += -qopt-report=5
  CXXFLAGS += -Wunknown-pragmas # Disable warning about OpenMP pragma no defined.
endif

ifeq ($(CXX),g++)
  CXXFLAGS += -mtune=native -march=native
endif

EXEC = main_gol

all: $(EXEC)

# Load common make options
include Makefile.common
LDFLAGS	  = $(COMMON_LIBS)

-include *.d

main_gol.o: main_gol.cpp
	$(CXX) $(CXXFLAGS) -c main_gol.cpp

main_gol: main_gol.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o main.gol main_gol.o $(COMMON_OBJS) $(LDFLAGS)

%_acc.o: %_acc.cpp
	$(ACC) $(ACCFLAGS) -c $< -o $@
	$(ACC) -MM $(ACCFLAGS) $< > $*.d

clean: clean_common
	/bin/rm -fv $(EXEC) *.o *.optrpt *.d
