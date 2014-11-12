
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
#define DEFAULT_K 3

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
        result->randVal = dataMat[start].randVal;
        result->index = dataMat[start].index;
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

void findMinIndex(double* input, int start, int end, int* result){
    //base case
    if(end - start  < 1){
        result[0] = -1;
        return;
    }
    if(end - start == 1){
        result[0] = start;
        return;
    }
    
    int mid = (start + end) / 2;
    int min1, min2;
    
    cilk_spawn findMinIndex(input, start, mid, &min1);
    findMinIndex(input, mid, end, &min2);
    cilk_sync;
    
    if(min2 == -1){
        result[0] = min1;
    } else if(input[min1] <= input[min2] && min1 != -1){
        result[0] = min1;
    } else {
        result[0] = min2;
    }
}

void findMinIndex(int* input, int start, int end, int* result){
    //base case
    if(end - start  < 1){
        result[0] = -1;
        return;
    }
    if(end - start == 1){
        result[0] = start;
        return;
    }
    
    int mid = (start + end) / 2;
    int min1, min2;
    
    cilk_spawn findMinIndex(input, start, mid, &min1);
    findMinIndex(input, mid, end, &min2);
    cilk_sync;
    
    if(min2 == -1){
        result[0] = min1;
    } else if(input[min1] <= input[min2] && min1 != -1){
        result[0] = min1;
    } else {
        result[0] = min2;
    }
}

void scan(int* data, int num, int* result){
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
    scan(halfData, halfSize, halfScan);
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

void scan(double* data, int num, double* result){
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
    double* halfData = new double[halfSize];
    double* halfScan = new double[halfSize + 1];
    cilk_for(int i = 0; i < halfSize; i++){
        halfData[i] = data[i * 2] + data[i * 2 + 1];
    }
    scan(halfData, halfSize, halfScan);
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

int selectAndRemoveNodes(int* adjMat, int numPoints, int* indNodes, int* indicesToRemove){
    int numRemoved = 0;

    nodeData* nodeValues = new nodeData[numPoints];
    int *neighborsToRemove = new int[numPoints + 1];
    int maxRand = 2 * pow(numPoints, 4);
    int matSize = numPoints * numPoints;
    cilk_for(int i = 0; i < numPoints; i++){
        nodeValues[i].randVal = rand() % maxRand + 1;
        nodeValues[i].index = i;
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
            indicesToRemove[i] = 1;
            indNodes[i] = 1;
        } else{
            indicesToRemove[i] = 0;
            indNodes[i] = 0;
        }
    }
    
    //mark neighbors to remove
    cilk_for(int i = 0; i < numPoints; i++){
        neighborsToRemove[i] = 0;
        cilk_for(int j = 0; j < numPoints; j++){
            if(indicesToRemove[j] == 1 && adjMat[i * numPoints + j] == 1){
                neighborsToRemove[i] = 1;
            }
        }
    }
    //and proliferate the neighbors a second time
    cilk_for(int i = 0; i < numPoints; i++){
        indicesToRemove[i] = 0;
        cilk_for(int j = 0; j < numPoints; j++){
            if(neighborsToRemove[j] == 1 && adjMat[i * numPoints + j] == 1){
                indicesToRemove[i] = 1;
            }
        }
    }
    
    scan(indicesToRemove, numPoints, neighborsToRemove);
    numRemoved = neighborsToRemove[numPoints];
    
    cilk_for(int i = 0; i < numPoints; i++){
        cilk_for(int j = 0; j < numPoints; j++){
            if(indicesToRemove[i] == 1 || indicesToRemove[j] == 1){
                nodesToKeep[i * numPoints + j] = 0;
            } else {
                nodesToKeep[i * numPoints + j] = 1;
            }
            
        }
    }
    
    //scan over nodes to keep
    int *scanResults = new int[matSize + 1];
    scan(nodesToKeep, matSize, scanResults);
    //readjust adjMat to reflect removed nodes
    int *tempAdjMat = new int[numPoints * numPoints];
    cilk_for(int i = 0; i < numPoints * numPoints; i++){
        if(nodesToKeep[i] == 1){
            tempAdjMat[scanResults[i]] = adjMat[i];
        }
    }
    
    cilk_for(int i = 0; i < numPoints * numPoints; i++){
       adjMat[i] = tempAdjMat[i];
    }
    
    delete nodeValues;
    delete nodesToKeep;
    delete scanResults;
    delete tempAdjMat;
    delete neighborsToRemove;
    
    return numRemoved;
}

int calcMaxIndSet(vector<pointDistance> distances, int cutoff, int numPoints, int* indSet){
    double cutoffVal = distances[cutoff].distance;
    int numPairs = numPoints * numPoints;
    int maxIndSetSize = 0;
    int *adjMat = new int[numPairs];
    int *remainingPointsScan = new int[numPoints + 1];
    
    //initialize independent set data 
    cilk_for(int i = 0; i < numPoints; i++){
        indSet[i] = 0;
    }
    
    //create an array to keep track of points removed from the graph
    int *remainingPoints = new int[numPoints];
    cilk_for(int i = 0; i < numPoints; i++){
        remainingPoints[i] = 1;
    }
    
    
    //create adjacncy matrix
    cilk_for(int i = 0; i < numPairs; i++){
        adjMat[i] = 0;
    }
    
    cilk_for(int i = 0; i < cutoff; i++){
        adjMat[distances[i].i * numPoints + distances[i].j] = 1;
    }
    
    int totalNodesRemoved = 0;
    int currentNodesRemoved = 1;
    int currentPoints = numPoints;
    
    while(currentNodesRemoved > 0 && totalNodesRemoved < numPoints){
        int* currentIndNodes = new int[numPoints - totalNodesRemoved];
        int* currentRemovedList = new int[numPoints - totalNodesRemoved];
        currentNodesRemoved = selectAndRemoveNodes(adjMat, currentPoints, currentIndNodes, currentRemovedList);
        
        currentPoints -= currentNodesRemoved;
        totalNodesRemoved += currentNodesRemoved;
        
        scan(remainingPoints, numPoints, remainingPointsScan);
        cilk_for(int i = 0; i < numPoints; i++){
            if(remainingPoints[i] == 1){
                indSet[i] = currentIndNodes[remainingPointsScan[i]];
                remainingPoints[i] = 1 - currentRemovedList[remainingPointsScan[i]];
            }
        }
        
        delete currentRemovedList;
        delete currentIndNodes;
    }
    
    //get the number of nodes
    scan(indSet, numPoints, remainingPointsScan);
    maxIndSetSize = remainingPointsScan[numPoints];
    
    delete adjMat;
    delete remainingPointsScan;
    delete remainingPoints;
    return maxIndSetSize;
}

vector<Point> blelloch(vector<Point> input, int k){
    int num = input.size();
    vector<Point> results;
    
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
    int* maxIndNums = new int[num * num];
    int* maxIndSet = new int[num];
    
    cilk_for(int i = 0; i < num * num; i++){
        maxIndNums[i] = -1;
    }
    
    int loops = 0; //sanity check for debugging
    while ((maxIndNums[current] < 0 || !(maxIndNums[current] <= k && (current == 0 || maxIndNums[current - 1] > k))) && loops < 100) {
        if(maxIndNums[current] < 0){
            maxIndNums[current] = calcMaxIndSet(distances, current, num, maxIndSet);
        }
        
        if(current > 0 && maxIndNums[current - 1] < 0){
            maxIndNums[current - 1] = calcMaxIndSet(distances, current - 1, num, maxIndSet);
        }
        if(maxIndNums[current] == k and maxIndNums[current - 1] == k){
            max = current;
        }
        if(maxIndNums[current] < k){
            max = current;
        }
        if(maxIndNums[current] > k){
            min = current;
        }
        current = (min + max) / 2;
            
        loops++;
    }
    
    //calculate it again to find the independent set
    int maxSize = calcMaxIndSet(distances, current, num, maxIndSet);
    
    //due to the randomness of the algorithm, we'll do one final sanity check
    loops = 0;
    while(maxSize != k && loops < 10){
        if(maxSize > k){
            current++;
        }
        if(maxSize < k){
            current--;
        }
        maxSize = calcMaxIndSet(distances, current, num, maxIndSet);
        loops++;
    }
    if(maxSize != k){
        //something went wrong...
        cout << "Error: max independent set is inconsistent" << endl;
        cout << "set size is " << maxSize << " at cutoff " << current << endl;
        return results;
    }
    
    if(DEBUG){
        cout << "max independent set found at index " << current << " with cutoff " << distances[current].distance << endl;
        for(int i = 0; i < num; i++){
            cout << maxIndSet[i] << " ";
        }
        cout << endl;
    }
    
    //create an array for centers
    int* centers = new int[k];
    int* indSetScan = new int[num + 1];
    scan(maxIndSet, num, indSetScan);
    cilk_for(int i = 0; i < num; i++){
        if(maxIndSet[i] == 1){
            centers[indSetScan[i]] = i;
        }
    }
    
    //keep track of the center each point is assigned to
    int* assignments = new int[num];
    cilk_for(int i = 0; i < num; i++){
        double* distanceToCenter = new double[k];
        int minIndex;
        cilk_for(int j = 0; j < k; j++){
            distanceToCenter[j] = input[i].distance(input[centers[j]]);
        }
        findMinIndex(distanceToCenter, 0, k, &minIndex);
        assignments[i] = minIndex;
    }
	
	if(DEBUG){
		cout << "initial centers" << endl;
		for(int i = 0; i < k; i++){
			cout << centers[i] << " ";
		}
		cout << endl;
		cout << "center assignments" << endl;
		for(int i = 0; i < num; i++){
			cout << assignments[i] << " ";
		}
		cout << endl;
	}
    
    //now to begin the swapping....
    int* swaps = new int[num];
    bool stopSwapping = false;
    cilk_for(int i = 0; i < num; i++){
        swaps[i] = -1;
    }
	
	loops = 0;
    
    while(!stopSwapping && (!DEBUG || (loops < 2))){
        stopSwapping = true;
        //cilk_for(int i = 0; i < k; i++){//for each center
        for(int i = 0; i < k; i++){//DEBUG
            double *swapDeltas = new double[num];
            cilk_for(int j = 0; j < num; j++){ //test each possible swap
				//check all points
				if(j != centers[i]){
                    double *delta = new double[num];
                    double *deltaScan = new double[num + 1];
					cilk_for(int l = 0; l < num; l++){
						double curDistance = input[l].distance(input[assignments[l]]);
						if(assignments[l] != i){
							//if its not assigned to center i, it might swap to new center at point j
							double potentialDistance = input[l].distance(input[j]);
							if(curDistance > potentialDistance){
								delta[l] = potentialDistance - curDistance;
							} else {
								//or it might remain assigned to its old point
								delta[l] = 0;
							}
						} else {
							//if it is assigned to center i, it might swap to new center j or any other center
							double* distanceToCenter = new double[k];
							int minIndex;
							cilk_for(int m = 0; m < k; m++){
								if(m == i){
									distanceToCenter[m] = input[l].distance(input[j]);
								} else {
									distanceToCenter[m] = input[l].distance(input[centers[m]]);
								}
							}
							findMinIndex(distanceToCenter, 0, k, &minIndex);
							if(minIndex != i && distanceToCenter[minIndex] < curDistance){
								//cout << "Error: swapping to an existing center should not reduce the cost" << endl;
								//cout << minIndex << ":" << distanceToCenter[minIndex] << " " << i << ":" << curDistance << endl;
							}
							delta[l] = distanceToCenter[minIndex] - curDistance;
							delete distanceToCenter;
						}
					}
				
                    scan(delta, num, deltaScan);
				
                    swapDeltas[j] = deltaScan[num];
				
                    if(j == 4){
                        for(int q = 0; q < num; q++){
                            cout << delta[q] << " ";
                        }
                        cout << endl;
                    }
				
                    delete delta;
                    delete deltaScan;
				} else {
					swapDeltas[j] = 0;
				}
				
            }
            
            //find min in swap deltas
			int bestSwapIndex;
            findMinIndex(swapDeltas, 0, num, &bestSwapIndex);
			if(swapDeltas[bestSwapIndex] < 0){
				//propose the swap
				swaps[bestSwapIndex] = i;
			}
			
			cout << i << ":";
			for(int j = 0; j < num; j++){
				cout << swapDeltas[j] << " ";
			}
			cout << endl;
			
            delete swapDeltas;
        }
		
		if(DEBUG){
			cout << "swaps at loop " << loops << ":" << endl;
			for(int i = 0; i < num; i++){
				cout << swaps[i] << " ";
			}
			cout << endl;
		}
		
		//perform one swaps
		int indexToSwap = -1;
		cilk_for(int i = 0; i < num; i++){
			if(swaps[i] >= 0 && centers[swaps[i]] != i){
				indexToSwap = i;
				stopSwapping = false;
			} else if(swaps[i] >= 0){
				cout << "Error: attempting to swap to the same index" << endl;
			}
		}
		
		if(indexToSwap >= 0){
			centers[swaps[indexToSwap]] = indexToSwap;
		}
		
		
		
		if(DEBUG){
			cout << "old center assignments" << endl;
			for(int i = 0; i < num; i++){
				cout << assignments[i] << " ";
			}
			cout << endl;
		}
		
		//update assignments
		cilk_for(int i = 0; i < num; i++){
			double* distanceToCenter = new double[k];
			int minIndex;
			cilk_for(int j = 0; j < k; j++){
				distanceToCenter[j] = input[i].distance(input[centers[j]]);
			}
			findMinIndex(distanceToCenter, 0, k, &minIndex);
			assignments[i] = minIndex;
		}
		
		if(DEBUG){
			cout << "new center assignments" << endl;
			for(int i = 0; i < num; i++){
				cout << assignments[i] << " ";
			}
			cout << endl;
		}
		
		if(DEBUG){
			cout << "centers after loop " << loops << ":" << endl;
			for(int i = 0; i < k; i++){
				cout << centers[i] << " ";
			}
			cout << endl;
			cout << endl;
		}
		
		//reset swap data
		cilk_for(int i = 0; i < num; i++){
			swaps[i] = -1;
		}
		
		loops++;
    }
    
    delete maxIndNums;
    delete maxIndSet;
    delete swaps;
    delete centers;
    delete indSetScan;
    delete assignments;
    return results; 
}

void printHelpShort(){
    cout << "Usage: ./blelloch [-f filename] [-n numPoints] [-s seed] [-r range] [-i castToInts] [-o outFile] [-c cilk] [-k centers]" << endl;
    cout << "type \"./blelloch -help\" for a detailed explanation" << endl << endl;
}

void printHelp(){
    cout << "Usage: ./blelloch [-f filename] [-n numPoints] [-s seed] [-r range] [-i castToInts] [-o outFile] [-c cilk] [-k centers]" << endl;
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
    cout << "    [-c cilk]       If 1, cilk's workspan analysis will be enabled. Default off" << endl;
    cout << "    [-k centers]    the number of centers to look for, default 3" << endl;
}

int main(int argc, char* argv[]){
    int seed = time(NULL);
    int num = DEFAULT_NUM_POINTS;
    int range = DEFAULT_POINT_RANGE;
    bool castToInt = DEFAULT_CAST;
    int k = DEFAULT_K;
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
            } else if (arg == "-k"){
                k = atoi(argv[i + 1]);
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
    
    if(k == num){
        cout << "looking for same number of centers as points, just copy the input file." << endl;
        return 0;
    } else if(k > num){
        cout << "looking for more centers than points, just take the input and duplicate a few points. I'm too lazy to do it for you" << endl;
        return 0;
    }
    
    blelloch(list, k);
    return 0;
}