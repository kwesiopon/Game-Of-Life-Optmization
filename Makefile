COMMON_OBJS = timer.o dummy.o
DBG      ?=
CXX      ?= g++
CXXFLAGS  = -O3 -I. -std=c++11 -I$(COMMON) $(DBG)

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
#include $(COMMON)/Makefile.common
#LDFLAGS	  = $(COMMON_LIBS)

main_gol.o: main_gol.cpp
	$(CXX) $(CXXFLAGS) -c main_gol.cpp

main_gol: main_gol.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o main.gol main_gol.o $(COMMON_OBJS) $(LDFLAGS)

clean: clean_common
	/bin/rm -fv $(EXEC) *.o *.optrpt *.d
