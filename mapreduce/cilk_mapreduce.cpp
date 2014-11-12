#include <vector>
#include <Point.cpp>
#include <DataGen.cpp>
#include <cilk/cilk.h>
#include <cilktools/cilkview.h>
#include <limits>
#include <set>
#include <cmath>

#define DEFAULT_NUM_OF_CLUSTERS 2;


using namespace std;


struct interNode{
    int index; //closest to which center
    double x, y; //sample information
};

void printInterNodeVector(vector<interNode> nodes){
    int num = nodes.size();
    if (num == 0){
        cout << "empty vector\n";
        return;
    }

    for (int i = 0; i < num; i++){
        cout << "nodes[" << i << "]: index=" << nodes[i].index << " x=" << nodes[i].x << " y=" << nodes[i].y << endl; 
    }
    cout << endl;
}
void printPointsVector(vector<Point> points){
    int num = points.size();
    for (int i = 0; i < num; i++){
        cout << "points[" << i << "]: " << points[i].getX() << " " << points[i].getY() << endl;
    }
    cout << endl;
}

void minReduceForIndex(double* array, int start, int end, int &index){
    //index stores the index of the minimum element among the array
    if ((end - start) == 1){
        index = start;
        return;
    }

    //first one on right
    int mid = (end + start) / 2;
    int leftIndex, rightIndex;
    cilk_spawn minReduceForIndex(array, start, mid, leftIndex);
    minReduceForIndex(array, mid, end, rightIndex);
    cilk_sync;

    if (array[leftIndex] < array[rightIndex]){
        index = leftIndex;
    }else{
        index = rightIndex;
    }
}

void sumReduceForXY(vector<interNode> list, double &resX, double &resY){
    //base case
    if (list.size() == 1){
        resX = list[0].x;
        resY = list[0].y;
        return;
    }

    //first one on right
    int mid = list.size() / 2;
    double resLX, resLY, resRX, resRY;
    vector<interNode> listL (list.begin(), list.begin() + mid);
    vector<interNode> listR (list.begin() + mid, list.end());

    cilk_spawn sumReduceForXY(listL, resLX, resLY);
    sumReduceForXY(listR, resRX, resRY);
    cilk_sync;

    resX = resLX + resRX;
    resY = resLY + resRY;
}




interNode mapParallel(Point point, vector<Point> centers){
    int num = centers.size();
    interNode result; 
    result.x = point.getX();
    result.y = point.getY();
    result.index = 0;

    double* dist_array = new double[num];
    //parallel to decrease span
    cilk_for(int i = 0; i < num; i++){
        dist_array[i] = point.distance(centers[i]);
    }

    minReduceForIndex(dist_array, 0, num, result.index);

    delete dist_array;
    return result;

}

interNode mapSerial(Point point, vector<Point> centers){
    int num = centers.size();
    interNode result; 
    result.x = point.getX();
    result.y = point.getY();

    int candidate = 0;
    double dist = std::numeric_limits<double>::max();

    for (int i = 0; i < centers.size(); i++){
        if (point.distance(centers[i]) < dist){
            dist = point.distance(centers[i]);
            candidate = i;
        }
    }
    result.index = candidate;
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
        centers[i].copy(points[index]);
    }

    //remove centers from points
    set<int>::reverse_iterator rit;
    for (rit = mySet.rbegin(); rit != mySet.rend(); rit++){
        points.erase(points.begin() + *rit);
    }
    
    return centers;
}

interNode combine(vector< vector<interNode> > lists, int index){
    interNode res;
    res.index = index;

    vector<interNode> focusList = lists[index];
    int num = focusList.size();
    if (num == 0){
        res.x = 0;
        res.y = 0;
        return res;
    }

    sumReduceForXY(focusList, res.x, res.y);
    return res;
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

    int numOfIteration = 0;

    //points & centers
    vector<Point> points = readData(filename);
    //cout << "before: " << points.size() << endl;
    //printPointsVector(points);
    vector<Point> centers = randomSelectKCenters(points, numOfClusters);
    vector<Point> centersOld (centers);
    cout << "original centers\n";
    printPointsVector(centers);
    //cout << "after: " << points.size() << endl;
    //printPointsVector(points);
    int numOfPoints = points.size();

    //centerLists
    vector< vector<interNode> > centerLists;
    for (int i = 0; i < numOfClusters; i++){
        vector<interNode> list;
        centerLists.push_back(list);
    }

    vector<interNode> interNodes(points.size());
    vector<interNode> combineNodes(numOfClusters);

    while(true){
        numOfIteration++;
        cout << "start of " << numOfIteration << " iteration\n";

        //mapping
        cilk_for (int i = 0; i < numOfPoints; i++){
            interNodes[i] = mapParallel(points[i], centers);
            int index = interNodes[i].index;
            centerLists[index].push_back(interNodes[i]);
        }
        /*
           printInterNodeVector(interNodes);
           cout << "centerLists\n";
           for (int i = 0; i < numOfClusters; i++){
           printInterNodeVector(centerLists[i]);
           }
           */

        cout << "map done\n";

        /*

        //combine
        cilk_for (int i = 0; i < numOfClusters; i++){
            combineNodes[i] = combine(centerLists, i);
        }
        //printInterNodeVector(combineNodes);


        cout << "combine done\n";

        //reduce
        cilk_for (int i = 0; i < numOfClusters; i++){
            int num = centerLists[i].size();
            //update centers
            if (num == 0){
                //do nothing
            }else{
                centers[i].setX(combineNodes[i].x / num);
                centers[i].setY(combineNodes[i].y / num);
            }
        }

        cout << "reduce done\n";

        //clear
        for (int i = 0; i < centerLists.size(); i++){
            centerLists[i].clear();
        }

        cout << "clear done\n";


        cout << "centers update\n";
        printPointsVector(centers);

        //check if okay
        //TODO
        double diff = 0;
        for (int i = 0; i < centers.size(); i++){
           diff += abs(centers[i].getX() - centersOld[i].getX()); 
           diff += abs(centers[i].getY() - centersOld[i].getY()); 
        }
        cout << "diff = " << diff << endl;
        cout << endl;

        if (diff < 1){
            break;
        }else{
            centersOld = centers;
        }

        */

        for (int i = 0; i < centerLists.size(); i++){
            centerLists[i].clear();
        }

        cout << "clear done\n";
        if (numOfIteration == 100)
            break;

    }

    return 0;
}

