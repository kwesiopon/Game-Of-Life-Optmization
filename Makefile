COMMON_OBJS = timer.o dummy.o
DBG      ?= -g
CXX      ?= g++
CXXFLAGS  = -O3 -I. -std=c++11 -I$(COMMON) $(DBG)

NVCC	    = nvcc
NVFLAGS	= -O3 -g -I. -std=c++11 -I$(COMMON) -arch sm_70 -g --resource-usage -O3

ACC	    = nvc++
ACCFLAGS= -O3 -g -I. -std=c++11 -I$(COMMON) -acc -gpu=cc70 -g --restrict
ACCFLAGS+= -Minfo=acc


LD		= $(ACC)
#LDFLAGS = -lcudart $(ACCFLAGS)
LDFLAGS = $(ACCFLAGS)
LIBS	= -lcublas #-lcblas -lopenblas
#LIBS	= -cudalib=cublas -lstdc++

ifeq ($(CXX),icpc)
  CXXFLAGS += -xHost #-no-vec
  CXXFLAGS += -qopt-report=5
  CXXFLAGS += -Wunknown-pragmas # Disable warning about OpenMP pragma no defined.
endif

ifeq ($(CXX),g++)
  CXXFLAGS += -mtune=native -march=native
endif

EXEC = main_gol gol_acc

all: $(EXEC)

OBJS = $(EXEC:=.o)
DEPS = $(OBJS:.o=.d)

-include $(DEPS)

# Load common make options
include Makefile.common
LDFLAGS	  = $(COMMON_LIBS)

-include *.d

main_gol.o: main_gol.cpp
	$(CXX) $(CXXFLAGS) -c main_gol.cpp

main_gol: main_gol.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o main_gol $^ $(CXXFLAGS)

#gol_acc: gol_acc.cpp $(COMMON_OBJS)
#	$(ACC) $(ACCFLAGS) -o gol_acc $^ $(LDSFlAGS)

clean: clean_common
	/bin/rm -fv $(EXEC) *.o *.optrpt *.d
