CXX=g++
INCLUDES=-I. 
CXXFLAGS=-O2 -fcilkplus $(INCLUDES)

LIBS = -lcilkrts

PROGS=genData genClusters blelloch

all:: $(PROGS)

DataMain.o: DataGen.cpp DataMain.cpp Point.cpp
Blelloch.o: Blelloch.cpp DataGen.cpp Point.cpp
ClustersMain.o: ClustersMain.cpp DataGen.cpp Point.cpp
genData: DataMain.o 
	$(CXX) $(BITS) -o $@ DataMain.o $(LIBS)
genClusters: ClustersMain.o 
	$(CXX) $(BITS) -o $@ ClustersMain.o $(LIBS)
blelloch: Blelloch.o 
	$(CXX) $(BITS) -o $@ Blelloch.o $(LIBS)

clean::
	-rm -f $(PROGS) *.o
