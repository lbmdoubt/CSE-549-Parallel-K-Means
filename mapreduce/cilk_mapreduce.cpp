#include <vector>
#include <Point.cpp>
#include <DataGen.cpp>
#include <cilk/cilk.h>
#include <cilktools/cilkview.h>
#include <algorithm>

#define DEFAULT_NUM_OF_CLUSTERS 2;


using namespace std;


struct interNode{
    int index; //closest to which center
    double x, y; //sample information
};


interNode map(Point point, vector<Point> centers){
    int num = centers.size();
    interNode result; 
    result.x = point.getX();
    result.y = point.getY();

    int* dist_array = new int[num];
    cilk_for(int i = 0; i < num; i++){
        dist_array[i] = point.distance(centers.at(i));
    }

    result.index = 0;
    

    return result;

}

vector<Point> randomSelectKCenters(vector<Point> points, int numOfClusters){
    vector<Point> centers(numOfClusters);
    int size = points.size();
    srand(time(NULL));

    for (int i = 0; i < numOfClusters; i++){
        int index = rand() % size;
        //TODO
        //avoid duplicate, can't use find function

        centers[i] = points[index];

        cout << "centers[" << i << "]: index = " << index << endl;
    }

    return centers;
}


void printHelp(){
    cout << "Usage: ./cilk_mapreduce [-f filename] [-n numOfCluster]" << endl;
}


int main(int argc, char* argv[]){
    char* filename = NULL;
    int numOfClusters = DEFAULT_NUM_OF_CLUSTERS; 

    if (argc > 1){
        if (argv[1] == "-h"){
            printHelp();
            return 0;
        }

        for (int i = 1; i < argc; i += 2){
            string arg = argv[i];
            if (arg == "-f"){
                filename = argv[i+1];
            }
            else if (arg == "-n"){
                numOfClusters = atoi(argv[i+1]);
            }
            else{
                printHelp();
                return 0;
            }
        }
    }
    
    if (filename == NULL){
        printHelp();
        return 0;
    }

    vector<Point> points = readData(filename);
    vector<Point> centers = randomSelectKCenters(points, numOfClusters);

    for (int i = 0; i < numOfClusters; i++){
        Point focus = centers[i];
        cout << "centers[" << i << "]: " << focus.getX() << ", " << focus.getY() << endl;
    }


    return 0;
}
