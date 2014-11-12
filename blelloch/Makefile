CXX=g++
INCLUDES=-I. 
CXXFLAGS=-O2 -fcilkplus $(INCLUDES)

LIBS = -lcilkrts

PROGS=genData genClusters blelloch analyze

all:: $(PROGS)

DataMain.o: DataGen.cpp DataMain.cpp Point.cpp
BlellochMain.o: BlellochMain.cpp Blelloch.cpp DataGen.cpp Point.cpp
ClustersMain.o: ClustersMain.cpp DataGen.cpp Point.cpp
Analyze.o: Analyze.cpp Blelloch.cpp DataGen.cpp Point.cpp
genData: DataMain.o 
	$(CXX) $(BITS) -o $@ DataMain.o $(LIBS)
genClusters: ClustersMain.o 
	$(CXX) $(BITS) -o $@ ClustersMain.o $(LIBS)
blelloch: BlellochMain.o 
	$(CXX) $(BITS) -o $@ BlellochMain.o $(LIBS)
analyze: Analyze.o
	$(CXX) $(BITS) -o $@ Analyze.o $(LIBS)

clean::
	-rm -f $(PROGS) *.o
