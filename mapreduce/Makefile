CXX=g++
INCLUDES=-I.
CXXFLAGS=-O2 -fcilkplus $(INCLUDES)

LIBS = -lcilkrts

PROGS=genData genClusters cilk_mapreduce

all:: $(PROGS)

DataMain.o: DataGen.cpp DataMain.cpp Point.cpp
ClustersMain.o: ClustersMain.cpp DataGen.cpp Point.cpp
cilk_mapreduce.o: DataGen.cpp Point.cpp cilk_mapreduce.cpp

genData: DataMain.o
	$(CXX) -o $@ DataMain.o $(LIBS)
genClusters: ClustersMain.o
	$(CXX) -o $@ ClustersMain.o $(LIBS)
cilk_mapreduce: cilk_mapreduce.o
	$(CXX) -o $@ cilk_mapreduce.o $(LIBS)

clean::
	-rm -f $(PROGS) *.o
