#include <vector>
#include <Point.cpp>
#include <DataGen.cpp>
#include <cilk/cilk.h>
#include <cilktools/cilkview.h>
#include <set>

#define DEFAULT_NUM_OF_CLUSTERS 2;


using namespace std;


struct interNode{
    int index; //closest to which center
    double x, y; //sample information
};

void printPointsVector(vector<Point> points){
    int num = points.size();
    for (int i = 0; i < num; i++){
        cout << "points[" << i << "]: " << points[i].getX() << " " << points[i].getY() << endl;
    }
    cout << endl;
}

int minReduceForIndex(double* array, int start, int end){
    if ((end - start) == 1){
        return start;
    }

    //first one on right
    int mid = (end + start) / 2;

    int leftIndex = minReduceForIndex(array, start, mid);
    int rightIndex = minReduceForIndex(array, mid, end);

    if (array[leftIndex] < array[rightIndex]){
        return leftIndex;
    }else{
        return rightIndex;
    }
}



interNode map(Point point, vector<Point> centers){
    int num = centers.size();
    interNode result; 
    result.x = point.getX();
    result.y = point.getY();

    double* dist_array = new double[num];
    //parallel to decrease span
    cilk_for(int i = 0; i < num; i++){
        dist_array[i] = point.distance(centers[i]);
    }

    result.index = minReduceForIndex(dist_array, 0, num);
    return result;

}

vector<Point> randomSelectKCenters(vector<Point> &points, int numOfClusters){
    vector<Point> centers(numOfClusters);
    set<int> mySet;
    int size = points.size();
    srand(time(NULL));

    for (int i = 0; i < numOfClusters; i++){
        int index = rand() % size;
        //avoid duplicate
        while (mySet.find(index) != mySet.end()){
            index = rand() % size;
        }
        
        mySet.insert(index);
        centers[i] = points[index];
    }

    //remove centers from points
    set<int>::reverse_iterator rit;
    for (rit = mySet.rbegin(); rit != mySet.rend(); rit++){
        points.erase(points.begin() + *rit);
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
    //cout << "before: " << points.size() << endl;
    //printPointsVector(points);
    vector<Point> centers = randomSelectKCenters(points, numOfClusters);
    //cout << "centers\n";
    //printPointsVector(centers);
    //cout << "after: " << points.size() << endl;
    //printPointsVector(points);


    vector<interNode> interNodes(points.size());
    cilk_for(int i = 0; i < points.size() ; i++){
        interNodes[i] = map(points[i], centers);
    }


    return 0;
}

