#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <cilk/cilk.h>
#include <cilktools/cilkview.h>
#include <cilk/reducer_min.h>
#include "Blelloch.cpp"
#include "DataGen.cpp"
#include "Point.cpp"

//default values
#define DEFAULT_NUM_POINTS 10
#define DEFAULT_POINT_RANGE 100
#define DEFAULT_CAST true
#define DEFAULT_K 3

using namespace std;

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
    char* outfile = NULL;
    if(argc > 1){
        if (string(argv[1]) == "-help") {
            printHelp();
            return 0;
        }
        
        for (int i = 1; i < argc; i += 2) {
            string arg = string(argv[i]);
            if (arg == "-f") {
                file = argv[i + 1];
            } else if (arg == "-0") {
                outfile = argv[i + 1];
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
    
    if(DEBUG){
        cout << "the generated points are:" << endl;
        for(int i = 0; i < list.size(); i++){
            cout << list[i].getX() << " " << list[i].getY() << endl;
        }
    }
    
    if(k == num){
        cout << "looking for same number of centers as points, just copy the input file." << endl;
        return 0;
    } else if(k > num){
        cout << "looking for more centers than points, just take the input and duplicate a few points. I'm too lazy to do it for you" << endl;
        return 0;
    }
    
    vector<Point> results = blelloch(list, k);
    
    if(outfile != NULL){
        writeData(outfile, results);
    } else {
        for(int i = 0; i < results.size(); i++){
            cout << results[i].getX() << " " << results[i].getY() << endl;
        }
    }
    
    return 0;
}