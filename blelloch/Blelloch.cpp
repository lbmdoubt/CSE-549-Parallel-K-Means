
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <cilk/cilk.h>
#include <cilktools/cilkview.h>
#include <cilk/reducer_min.h>
#include "DataGen.cpp"
#include "Point.cpp"

#define DEBUG 1

//default values
#define DEFAULT_NUM_POINTS 10
#define DEFAULT_POINT_RANGE 100
#define DEFAULT_CAST true

using namespace std;

struct pointDistance{
    int i, j; //index of the two points
    double distance;
};

struct nodeData{
    int randVal;
    int index;
};

void parallelMergeDistances(vector<pointDistance> *distances, int start1, int end1, int start2, int end2, pointDistance* results){
    if(end1 - start1 < end2 - start2){
        parallelMergeDistances(distances, start2, end2, start1, end1, results);
        return;
    }
    
    //base case
    if(end1 - start1 < 1){
        //return empty array
        return;
    } else if(end2 - start2 == 0){
        //the smaller array is empty, return the larger array
        cilk_for(int i = 0; i < end1 - start1; i++){
        //for(int i = 0; i < end1 - start1; i++){
            results[i] = distances->at(start1 + i);
        }
        return;
    } else if(end1 - start1 == 1){
        //there is 1 element in the larger array
        if(distances->at(start1).distance < distances->at(start2).distance){
            results[0] = distances->at(start1);
            results[1] = distances->at(start2);
        } else {
            results[0] = distances->at(start2);
            results[1] = distances->at(start1);
        }
        return;
    }
    
    int mid1 = (end1 + start1) / 2;
    pointDistance *part1, *part2;
    int part1Size, part2Size;
    
    if(distances->at(mid1).distance < distances->at(start2).distance){
        part1 = new pointDistance[mid1 - start1];
        part1Size = mid1 - start1;
        cilk_for(int i = 0; i < mid1 - start1; i++){
            part1[i] = distances->at(start1 + i);
        }
        part2Size = end1 - mid1 + end2 - start2;
        part2 = new pointDistance[part2Size];
        parallelMergeDistances(distances, mid1, end1, start2, end2, part2);
    } else if(distances->at(mid1).distance >= distances->at(end2 - 1).distance){
        part2Size = end1 - mid1;
        part2 = new pointDistance[end1 - mid1];
        cilk_for(int i = 0; i < end1 - mid1; i++){
            part2[i] = distances->at(mid1 + i);
        }
        part1Size = mid1 - start1 + end2 - start2;
        part1 = new pointDistance[part1Size];
        parallelMergeDistances(distances, start1, mid1, start2, end2, part1);
    } else {
        int mid2 = (end2 + start2) / 2;
        int rangeMin = start2;
        int rangeMax = end2;
        bool mid2Found = false;
        while(!mid2Found){
            if(distances->at(mid2 - 1).distance <= distances->at(mid1).distance && distances->at(mid2).distance > distances->at(mid1).distance){
                mid2Found = true;
            } else if(distances->at(mid2).distance <= distances->at(mid1).distance) {
                rangeMin = mid2;
                mid2 = (rangeMax + rangeMin) / 2;
            } else {
                rangeMax = mid2;
                mid2 = (rangeMax + rangeMin) / 2;
            }
        }
        //todo cilk_spawn
        part2Size = end1 - mid1 + end2 - mid2;
        part1Size = mid1 - start1 + mid2 - start2;
        part1 = new pointDistance[part1Size];
        part2 = new pointDistance[part2Size];
        cilk_spawn parallelMergeDistances(distances, start1, mid1, start2, mid2, part1);
        parallelMergeDistances(distances, mid1, end1, mid2, end2, part2);
        cilk_sync;
    }
    
    //concatenate the two parts
    if(part1Size + part2Size != end2 - start2 + end1 - start1 && DEBUG){
        cout << "Error, the merged sizes don't match" << endl;
    }
    //cilk_for(int i = 0; i < part1.size(); i++){
    for(int i = 0; i < part1Size; i++){
        results[i] = part1[i];
    }
    
    //cilk_for(int i = 0; i < part2.size(); i++){
    for(int i = 0; i < part2Size; i++){
        results[part1Size + i] = part2[i];
    }
}

//in parallel, calculate the distances between all points and sort them
void calcAndSortDistances(vector<Point> &input, int start, int end, vector<pointDistance>* results){
    //base cases
    if(end - start < 1){
        return;
    } else if(end - start == 1){
        int i = start / input.size();
        int j = start % input.size();
        results->at(start).i = i;
        results->at(start).j = j;
        results->at(start).distance = input[i].distance(input[j]);
        return;
    }
    
    int mid = (start + end) / 2;
    cilk_spawn calcAndSortDistances(input, start, mid, results);
    //calcAndSortDistances(input, start, mid, results);
    calcAndSortDistances(input, mid, end, results);
    cilk_sync;
    //merge
    pointDistance* mergeResults = new pointDistance[end - start];
    parallelMergeDistances(results, start, mid, mid, end, mergeResults);
    //cilk_for(int i = 0; i < end - start; i++){
    for(int i = 0; i < end - start; i++){
        results->at(start + i) = mergeResults[i];
    }
}

void findMinNode(nodeData* dataMat, int start, int end, nodeData* result){
    //base case
    if(end - start  < 1){
        result->randVal = -1;
        result->index = -1;
        return;
    }
    if(end - start == 1){
        result = &dataMat[start];
        return;
    }
    
    int mid = (start + end) / 2;
    nodeData min1, min2;
    
    cilk_spawn findMinNode(dataMat, start, mid, &min1);
    findMinNode(dataMat, mid, end, &min2);
    cilk_sync;
    
    if(min2.index == -1){
        result->randVal = min1.randVal;
        result->index = min1.index;
    } else if(min1.randVal <= min2.randVal && min1.index != -1){
        result->randVal = min1.randVal;
        result->index = min1.index;
    } else {
        result->randVal = min2.randVal;
        result->index = min2.index;
    }
}

void scan(int* data, int num, int* result){
    //TODO: check for off by one errors at the end of each loop

    int* halfData = new int[num / 2];
    int* halfScan = new int[num / 2 + 1];
    cilk_for(int i = 0; i < num / 2; i++){
        halfData[i] = data[i * 2] + data[i * 2 + 1];
    }
    scan(halfData, num / 2, halfScan);
    cilk_for(int i = 0; i < num / 2 - 1; i++){
        result[2 * i] = halfScan[i];
        result[2 * i + 1] = halfScan[i] + data[2 * i + 1];
    }
    
    //this is definitely wrong:
    //result[num] = halfData[num / 2];
    
    delete halfData;
}

int selectAndRemoveNodes(int* adjMat, int numPoints){
    int numRemoved = 0;

    nodeData* nodeValues = new nodeData[numPoints];
    int maxRand = 2 * pow(numPoints, 4);
    int matSize = numPoints * numPoints;
    cilk_for(int i = 0; i < numPoints; i++){
        nodeValues[i].randVal = rand() % maxRand + 1;
        nodeValues[i].index = 1;
    }
    
    nodeData* valuesMatrix = new nodeData[matSize];
    //proliferate the lower values twice
    cilk_for(int i = 0; i < numPoints; i++){
        cilk_for(int j = 0; j < numPoints; j++){
            int index = i * numPoints + j;
            if(adjMat[index] > 0){
                if(nodeValues[i].randVal <= nodeValues[j].randVal){
                    valuesMatrix[index] = nodeValues[i];
                } else {
                    valuesMatrix[index] = nodeValues[j];
                }
            } else {
                valuesMatrix[index].randVal = -1;
                valuesMatrix[index].index = -1;
            }
        }
    }
    
    cilk_for(int i = 0; i < numPoints; i++){
        findMinNode(valuesMatrix, i * numPoints, (i + 1) * numPoints, &nodeValues[i]);
    }
    
    cilk_for(int i = 0; i < numPoints; i++){
        cilk_for(int j = 0; j < numPoints; j++){
            int index = i * numPoints + j;
            if(adjMat[index] > 0){
                if(nodeValues[i].randVal <= nodeValues[j].randVal){
                    valuesMatrix[index] = nodeValues[i];
                } else {
                    valuesMatrix[index] = nodeValues[j];
                }
            }
        }
    }
    
    cilk_for(int i = 0; i < numPoints; i++){
        findMinNode(valuesMatrix, i * numPoints, (i + 1) * numPoints, &nodeValues[i]);
    }
    
    //select nodes to remove
    int* nodesToKeep = new int[matSize];
    cilk_for(int i = 0; i < numPoints; i++){
        if(nodeValues[i].index == i){
            numRemoved++;
        }
        cilk_for(int j = 0; j < numPoints; j++){
            if(nodeValues[i].index == i && nodeValues[j].index == j){
                nodesToKeep[i * numPoints + j] = 1;
            } else {
                nodesToKeep[i * numPoints + j] = 0;
            }
            
        }
    }
    
    
    
    //TODO: scan over nodes to keep
    int *scanResults = new int[matSize + 1];
    scan(nodesToKeep, matSize, scanResults);
    
    //TODO: readjust adjMat to reflect removed nodes
    
    delete nodeValues;
    delete nodesToKeep;
    return numRemoved;
}

int calcMaxIndSet(vector<pointDistance> distances, int cutoff, int numPoints){
    double cutoffVal = distances[cutoff].distance;
    int numPairs = numPoints * numPoints;
    int *adjMat = new int[numPairs];
    //create adjacncy matrix
    cilk_for(int i = 0; i < numPairs; i++){
        if(distances[i].distance <= cutoffVal){
            adjMat[i] = 1;
        } else {
            adjMat[i] = 0;
        }
    }
    
    int totalNodesRemoved = 0;
    int currentNodesRemoved = 1;
    int currentPoints = numPoints;
    
    while(currentNodesRemoved > 0){
        currentNodesRemoved = selectAndRemoveNodes(adjMat, currentPoints);
        currentPoints -= currentNodesRemoved;
        totalNodesRemoved += currentNodesRemoved;
    }
    
    delete adjMat;
    return totalNodesRemoved;
}

vector<Point> blelloch(vector<Point> input, int k){
    int num = input.size();
    
    //in parallel, calculate the distances between all points and sort them
    vector<pointDistance> distances;
    distances.resize(num * num); //this will produce duplicates, but makes things a lot easier
    calcAndSortDistances(input, 0, num * num, &distances);
    
    if(DEBUG){
        for(int i = 0; i < num; i++){
            for(int j = 0; j < num; j++){
                cout << distances[i * num + j].distance << " ";
            }
            cout << endl;
        }
    }
    
    int max = num * num;
    int min = 0;
    int current = (min + max) / 2;
    int* maxIndSets = new int[num * num];
    
    cilk_for(int i = 0; i < num * num; i++){
        maxIndSets[i] = -1;
    }
    
    
    /**
    while (maxIndSets[current] < 0 || !(maxIndSets[current] <= k && (current == 0 || maxIndSets[current - 1] > k))) {
        if(maxIndSets[current] < 0){
            //calc k
        }
        if(current > 0 && maxIndSets[current - 1] < 0){
        }
        if current == k and current - 1 == k
            go down
        if current < k
            go down
        if current > k
            go up
            
    }
    */
    
    delete maxIndSets;
    return input; //Temporary holder for the results
}

void printHelpShort(){
    cout << "Usage: ./blelloch [-f filename] [-n numPoints] [-s seed] [-r range] [-i castToInts] [-o outFile] [-c cilk]" << endl;
    cout << "type \"./blelloch -help\" for a detailed explanation" << endl << endl;
}

void printHelp(){
    cout << "Usage: ./blelloch [-f filename] [-n numPoints] [-s seed] [-r range] [-i castToInts] [-o outFile] [-c cilk]" << endl;
    cout << "    [-f filename]   If set, loads the points data from a file" << endl;
    cout << "                    Otherwise, the points are randomly generated" << endl;
    cout << "    [-n numPoints]  Sets the number of points to generate if -f isn't set." << endl;
    cout << "                    Defaults to 10" << endl;
    cout << "    [-s seed]       Sets the seed for the random number generator." << endl;
    cout << "                    If not set, a random seed based on the time will be chosen" << endl;
    cout << "                    note: this will also seed the random numbers for the algorithm" << endl;
    cout << "    [-r range]      Sets the upper limit of the range for x and y values." << endl;
    cout << "                    Defaults to 100." << endl;
    cout << "    [-i castToInts] If 0, the points generated will be doubles." << endl;
    cout << "                    Otherwise the x and y values will be generated as integers." << endl;
    cout << "                    Defaults to 1." << endl << endl;
    cout << "    [-o outFile]    Writes the results to this file if set." << endl;
    cout << "    [-o cilk]       If 1, cilk's workspan analysis will be enabled. Default off" << endl;
}

int main(int argc, char* argv[]){
    int seed = time(NULL);
    int num = DEFAULT_NUM_POINTS;
    int range = DEFAULT_POINT_RANGE;
    bool castToInt = DEFAULT_CAST;
    char* file = NULL;
    if(argc > 1){
        if (string(argv[1]) == "-help") {
            printHelp();
            return 0;
        }
        
        for (int i = 1; i < argc; i += 2) {
            string arg = string(argv[i]);
            if (arg == "-f") {
                file = argv[i + 1];
            } else if (arg == "-n"){
                num = atoi(argv[i + 1]);
            } else if (arg == "-s"){
                seed = atoi(argv[i + 1]);
            } else if (arg == "-r"){
                range = atoi(argv[i + 1]);
            } else if (arg == "-i"){
                if(string(argv[i + 1]) == "0"){
                    castToInt = false;
                }
            } else {
                cout << arg << " is not recognized" << endl;
                printHelpShort();
                return 0;
            }
        }
    } else {
        printHelpShort();
    }  
    
    vector<Point> list;
    srand(seed);
    
    if(file != NULL){
        list = readData(file);
    } else {
        list = genData(num, seed, range, castToInt);
    }
    
    for(int i = 0; i < list.size(); i++){
        cout << list[i].getX() << " " << list[i].getY() << endl;
    }
    
    blelloch(list, 3);
    return 0;
}