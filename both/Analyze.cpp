#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <cilk/cilk.h>
#include <cilktools/cilkview.h>
#include <cilk/reducer_min.h>
#include "cilktime.h"
#include "DataGen.cpp"
#include "Point.cpp"
#include "Blelloch.cpp"
#include "cilk_mapreduce.cpp"

using namespace std;

double calcError(vector<Point> points, vector<Point> centers){
    double error = 0.0;
    for(int i = 0; i < points.size(); i++){
        double minError = points[i].distance(centers[0]);
        for(int j = 1; j < centers.size(); j++){
            minError = min(minError, points[i].distance(centers[j]));
        }
        error += minError;
    }
    return error;
}

void printHelp(){
    cout << "Usage: ./analyze uniform [flags]" << endl;
    cout << "            tests on random uniformly generated data" << endl;
    cout << "       ./analyze clusters [numClusters] [stddev] [flags]" << endl;
    cout << "            tests on random data with clusters" << endl;
    cout << "            [numClusters]    The number of clusters to generate." << endl;
    cout << "            [stddev]         The standard deviation for the clusters." << endl;
    cout << "Usage: ./analyze file [fileName] [-f outputFile] [-k centers]" << endl;
    cout << "            tests on data read from a file." << endl;
    cout << "The following flags are accepted:" << endl;
    cout << "       [-f outputFile]     The file to output data to. If not set, output will be printed to screen." << endl;
    cout << "       [-k centers]        The number of centers to look for (aka, the k in k-means)" << endl;
    cout << "       [-s seed]           The seed that's used to generate the data. (well, technically the seed that's used to generate the seeds used to generate the data.)" << endl;
    cout << "       [-n numPoints]      The number of points to generate" << endl;
    cout << "       [-r range]          The range for generating the points." << endl;
    cout << "       [-i castToInts]     If 1, all generated points are cast to integers" << endl;
    cout << "       [-l loops]          The number of trials to run" << endl;
    cout << "       [-o format]         If 1, output will be organized by run, otherwise data will be outputted by type" << endl;
	cout << "       [-a initialChoice]  If 1, chooses initial points randomly. Otherwise initial points will be chosen by MaxIndSet" << endl;
	cout << "       [-b update]         If 1, centers will be updated via averaging. Otherwise points will be updated through swapping" << endl;
}

int main(int argc, char* argv[]){
    bool invalidArgs = true;
    vector<vector<Point> > results;
    vector<vector<Point> > centers;
    vector<double> error;
    vector<double> populationCenterError;
    vector<int> seeds;
	vector<long> times;
    int seed = time(NULL);
    bool format = true;
    ofstream output;
    bool fromFile = false;
    if(argc > 1){
        vector<Point> points;
        int flagIndex = 0;
        int k = 3;
        if(string(argv[1]) == "file" && argc > 2){
            invalidArgs = false;
            char* input = argv[2];
            centers.resize(1);
            points = readData(input, centers[0]);
            
            flagIndex = 3;
            while(flagIndex < argc - 1){
                if(string(argv[flagIndex]) == "-f"){
                    output.open(argv[flagIndex + 1]);
                } else if (string(argv[flagIndex]) == "-k"){
                    k = atoi(argv[flagIndex + 1]);
                }
                
                flagIndex += 2;
            }
            
            fromFile = true;
            
            results.resize(1);
            results[0] = blelloch(points, k);
            if(centers[0].size() > 0){
                populationCenterError.resize(1);
                populationCenterError[0] = calcError(points, centers[0]);
            }
            error.resize(1);
            error[0] = calcError(points, results[0]);
        } else if(string(argv[1]) == "clusters" || string(argv[1]) == "uniform") {
            invalidArgs = false;
            int type = 0;
            int numClusters = 1;
            double stdDev = 10;
			bool initialChoice = false;
			bool update = false;
            if(string(argv[1]) == "clusters"){
                type = 0;
                numClusters = atoi(argv[2]);
                stdDev = atof(argv[3]);
                flagIndex = 4;
            } else if(string(argv[1]) == "uniform"){
                type = 1;
                flagIndex = 2;
            }
            
            int targetLoops = 1;
            int num = 10;
            int range = 100;
            bool castToInt = 1;
            
            
            while(flagIndex < argc - 1){
                if(string(argv[flagIndex]) == "-f"){
                    output.open(argv[flagIndex + 1]);
                } else if (string(argv[flagIndex]) == "-k"){
                    k = atoi(argv[flagIndex + 1]);
                } else if (string(argv[flagIndex]) == "-s"){
                    seed = atoi(argv[flagIndex + 1]);
                } else if (string(argv[flagIndex]) == "-n"){
                    num = atoi(argv[flagIndex + 1]);
                } else if (string(argv[flagIndex]) == "-r"){
                    range = atoi(argv[flagIndex + 1]);
                } else if (string(argv[flagIndex]) == "-i"){
                    castToInt = (string(argv[flagIndex + 1]) == "1");
                } else if (string(argv[flagIndex]) == "-l"){
                    targetLoops = atoi(argv[flagIndex + 1]);
                } else if (string(argv[flagIndex]) == "-o"){
                    format = (string(argv[flagIndex + 1]) == "1");
                } else if (string(argv[flagIndex]) == "-a" && atoi(argv[flagIndex + 1]) == 1){
                    initialChoice = true;
                } else if (string(argv[flagIndex]) == "-b" && atoi(argv[flagIndex + 1]) == 1){
                    update = true;
                }
                
                flagIndex += 2;
            }
            
            //because these algorithms are randomized we want to pregenerate the seeds so that all runs are consistent
            cout << "initial seed: " << seed << endl;
            srand(seed);
            seeds.resize(targetLoops);
            for(int i = 0; i < targetLoops; i++){
                seeds[i] = rand();
            }
            
            if(type == 0){
                centers.resize(targetLoops);
                populationCenterError.resize(targetLoops);
            }
            results.resize(targetLoops);
            error.resize(targetLoops);
			times.resize(targetLoops);
            
            for(int i = 0; i < targetLoops; i++){
                cout << i << "/" << targetLoops << endl;
                if(type == 1){
                    points = genData(num, seeds[i], range, castToInt);
                } else {
					points = genClusters(numClusters, stdDev, num, seeds[i], range, castToInt, centers[i]);
					populationCenterError[i] = calcError(points, centers[i]);
                }
				
				//__cilkview_workspan_reset();
				//__cilkview_workspan_start();
				
				long startTime = cilk_getticks();
				if(initialChoice){
					results[i] = randomSelectKCenters(points, k);
				} else {
					results[i] = findCentersBlelloch(points, k);
				}
				if(update){
					mapreduce(points, k, results[i]);
				} else {
					swap(points, k, results[i]);
				}
				long endTime = cilk_getticks();
				
				//__cilkview_workspan_stop();
				times[i] = endTime - startTime;
                
                error[i] = calcError(points, results[i]);
            }
        }
        
    }
    if(invalidArgs){
        printHelp();
    } else {
        if(!output.is_open()){
            if(format){
                for(int i = 0; i < results.size(); i++){
                    cout << "run #" << i << endl;
                    cout << "seed: " << seeds[i] << endl;
                    cout << "results: " << endl;
                    for(int j = 0; j < results[i].size(); j++){
                        cout << results[i][j].getX() << " " << results[i][j].getY() << endl;
                    }
                    cout << "error: " << error[i] << endl;
					cout << "time: " << times[i] << endl;
                    if(centers.size() > i && centers[i].size() > 0){
                        cout << "population centers:" << endl;
                        for(int j = 0; j < centers[i].size(); j++){
                            cout << centers[i][j].getX() << " " << centers[i][j].getY() << endl;
                        }
                        cout << "population center error: " << populationCenterError[i] << endl;
                    }
                }
            } else {
                cout << "total runs: " << results.size() << endl;
                cout << "seeds:" << endl;
                for(int i = 0; i < results.size(); i++){
                    cout << seeds[i] << endl;
                }
                cout << "results: " << endl;
                for(int i = 0; i < results.size(); i++){
                    for(int j = 0; j < results[i].size(); j++){
                        cout << " " << results[i][j].getX() << " " << results[i][j].getY() << "  ";
                    }
                    cout << endl;
                }
                cout << "errors: " << endl;
                for(int i = 0; i < results.size(); i++){
                    cout << error[i] << endl;
                }
				cout << "times: " << endl;
                for(int i = 0; i < times.size(); i++){
                    cout << times[i] << endl;
                }
                if(centers.size() > 0 && centers[0].size() > 0){
                    cout << "population centers:" << endl;
                    for(int i = 0; i < centers.size(); i++){
                        for(int j = 0; j < centers[i].size(); j++){
                            cout << " " << centers[i][j].getX() << " " << centers[i][j].getY() << "  ";
                        }
                        cout << endl;
                    }
                    cout << "population center error: " << endl;
                    for(int i = 0; i < centers.size(); i++){
                        cout << populationCenterError[i] << endl;
                    }
                }
            }
        } else {
            if(!fromFile){
                output << "initial seed: " << seed << endl;
            }
            if(format){
                for(int i = 0; i < results.size(); i++){
                    output << "run #" << i << endl;
                    output << "seed: " << seeds[i] << endl;
                    output << "results: " << endl;
                    for(int j = 0; j < results[i].size(); j++){
                        output << results[i][j].getX() << " " << results[i][j].getY() << endl;
                    }
                    output << "error: " << error[i] << endl;
					output << "time: " << times[i] << endl;
                    if(centers.size() > i && centers[i].size() > 0){
                        output << "population centers:" << endl;
                        for(int j = 0; j < centers[i].size(); j++){
                            output << centers[i][j].getX() << " " << centers[i][j].getY() << endl;
                        }
                        output << "population center error: " << populationCenterError[i] << endl;
                    }
                }
            } else {
                output << "total runs: " << results.size() << endl;
                output << "seeds:" << endl;
                for(int i = 0; i < results.size(); i++){
                    output << seeds[i] << endl;
                }
                output << "results: " << endl;
                for(int i = 0; i < results.size(); i++){
                    for(int j = 0; j < results[i].size(); j++){
                        output << " " << results[i][j].getX() << " " << results[i][j].getY() << "  ";
                    }
                    output << endl;
                }
                output << "errors: " << endl;
                for(int i = 0; i < results.size(); i++){
                    output << error[i] << endl;
                }
				output << "times: " << endl;
                for(int i = 0; i < times.size(); i++){
                    output << times[i] << endl;
                }
                if(centers.size() > 0 && centers[0].size() > 0){
                    output << "population centers:" << endl;
                    for(int i = 0; i < centers.size(); i++){
                        for(int j = 0; j < centers[i].size(); j++){
                            output << " " << centers[i][j].getX() << " " << centers[i][j].getY() << "  ";
                        }
                        output << endl;
                    }
                    output << "population center error: " << endl;
                    for(int i = 0; i < centers.size(); i++){
                        output << populationCenterError[i] << endl;
                    }
                }
            }
            output.close();
        }
    }
}