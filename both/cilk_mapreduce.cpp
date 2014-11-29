#include <vector>
#include <Point.cpp>
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

void mapreduce_scan(int* data, int num, int* result){
    //base case
    if(num == 1){
        result[0] = 0;
        result[1] = data[0];
        return;
    } else if(num < 1){
        data[0] = 0;
        return;
    }

    int halfSize = num / 2;
    int* halfData = new int[halfSize];
    int* halfScan = new int[halfSize + 1];
    cilk_for(int i = 0; i < halfSize; i++){
        halfData[i] = data[i * 2] + data[i * 2 + 1];
    }
    mapreduce_scan(halfData, halfSize, halfScan);
    cilk_for(int i = 0; i < num; i++){
        if(i % 2 == 0){
            result[i] = halfScan[i / 2];
        } else {
            result[i] = halfScan[i / 2] + data[i - 1];
        }
    }

    result[num] = halfScan[halfSize];
    if(num % 2 == 1){
        result[num] += data[num - 1];
    }

    delete halfData, halfScan;
}

void printIntArray(int* array, int num){
    for (int i = 0; i < num; i++){
        cout << array[i] << " ";
    }
    cout << endl;
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

void mapreduce(vector<Point> &points, int numOfClusters, vector<Point> &centers){
    vector<Point> centersOld (centers);
    int numOfPoints = points.size();

    //centerLists
    vector< vector<interNode> > centerLists;
    for (int i = 0; i < numOfClusters; i++){
        vector<interNode> list;
        centerLists.push_back(list);
    }

    vector<interNode> interNodes(numOfPoints);
    vector<interNode> combineNodes(numOfClusters);

    int** indicatorArrays = new int*[numOfClusters];
    int** indicatorResults = new int*[numOfClusters];
    for (int i = 0; i < numOfClusters; i++){
        indicatorArrays[i] = new int[numOfPoints];
        indicatorResults[i] = new int[numOfPoints+1];
    }

    int iteration = 0;

    //work begin
    while(true){
        iteration ++;

        //mapping
        //initialzie indecator arrays
        cilk_for (int i = 0; i < numOfClusters; i++){
            cilk_for (int j = 0; j < numOfPoints; j++){
                indicatorArrays[i][j] = 0;
            }
        }
        //map each point, and check which center is closer
        cilk_for (int i = 0; i < numOfPoints; i++){
            interNodes[i] = mapParallel(points[i], centers);
            int index = interNodes[i].index;
            indicatorArrays[index][i] = 1;
        }
        //how many points that are closer to each center
        cilk_for (int i = 0; i < numOfClusters; i++){
            mapreduce_scan(indicatorArrays[i], numOfPoints, indicatorResults[i]);
            //printIntArray(indicatorResults[i], numOfPoints+1);
            int tmp = indicatorResults[i][numOfPoints];
            centerLists[i].resize(tmp);
        }
        //assign each node to centerLists
        cilk_for (int i = 0; i < numOfPoints; i++){
           //check which center it's closer to
           for (int j = 0; j < numOfClusters; j++){
               if (indicatorResults[j][i+1] != indicatorResults[j][i]){
                   //incremented, found
                   int index = indicatorResults[j][i];
                   centerLists[j][index] = interNodes[i];
                   break;
                }
            }
        }
        /*
        printInterNodeVector(interNodes);
        cout << "centerLists\n";
        for (int i = 0; i < numOfClusters; i++){
            printInterNodeVector(centerLists[i]);
        }
        */




        //combine
        cilk_for (int i = 0; i < numOfClusters; i++){
            combineNodes[i] = combine(centerLists, i);
        }
        //printInterNodeVector(combineNodes);

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

        //clear
        for (int i = 0; i < centerLists.size(); i++){
            centerLists[i].clear();
        }


        //cout << "centers update\n";
        //printPointsVector(centers);

        //check if okay
        //TODO
        double diff = 0;
        for (int i = 0; i < centers.size(); i++){
           diff += abs(centers[i].getX() - centersOld[i].getX()); 
           diff += abs(centers[i].getY() - centersOld[i].getY()); 
        }
        //cout << "diff = " << diff << endl;
        //cout << endl;
        //cout << "end of iteration " << iteration << endl;

        if (diff < 0.001){
            break;
        }else{
            centersOld = centers;
        }
    }


}

void mapreduce(vector<Point> &points, int numOfClusters){
	vector<Point> centers = randomSelectKCenters(points, numOfClusters);
    cout << "original centers\n";
    printPointsVector(centers);
	mapreduce(points, numOfClusters, centers);
}
