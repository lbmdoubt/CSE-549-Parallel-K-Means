CXX=g++
INCLUDES=-I.
CXXFLAGS=-O2 -fcilkplus $(INCLUDES)

LIBS = -lcilkrts

PROGS=cilk_mapreduce

all:: $(PROGS)

cilk_mapreduce.o: DataGen.cpp Point.cpp cilk_mapreduce.cpp 
cilk_mapreduce: cilk_mapreduce.o
	$(CXX) -o $@ cilk_mapreduce.o $(LIBS)

clean::
	-rm -f $(PROGS) *.o
