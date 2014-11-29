CXX=g++
INCLUDES=-I. 
CXXFLAGS=-O2 -fcilkplus $(INCLUDES)

LIBS = -lcilkrts

PROGS=genData genClusters analyze

all:: $(PROGS)

DataMain.o: DataGen.cpp DataMain.cpp Point.cpp
ClustersMain.o: ClustersMain.cpp DataGen.cpp Point.cpp
Analyze.o: Analyze.cpp Blelloch.cpp cilk_mapreduce.cpp DataGen.cpp Point.cpp Timer.cpp
genData: DataMain.o 
	$(CXX) $(BITS) -o $@ DataMain.o $(LIBS)
genClusters: ClustersMain.o 
	$(CXX) $(BITS) -o $@ ClustersMain.o $(LIBS)
analyze: Analyze.o
	$(CXX) $(BITS) -o $@ Analyze.o $(LIBS)

clean::
	-rm -f $(PROGS) *.o
