
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include "DataGen.cpp"
#include "Point.cpp"

#define DEFAULT_NUM_POINTS 10
#define DEFAULT_POINT_RANGE 100
#define DEFAULT_CAST true

using namespace std;

void printHelpShort(){
    cout << "Usage: ./genData [-f filename] [-n numPoints] [-s seed] [-r range] [-i castToInts]" << endl;
    cout << "type \"./genData -help\" for an explanation" << endl << endl;
}

void printHelp(){
    cout << "Usage: ./genData [-f filename] [-n numPoints] [-s seed] [-r range] [-i castToInts]" << endl;
    cout << "    [-f filename]   If set, outputs the points to a file specified by filename." << endl;
    cout << "                    Otherwise, prints the points onto the screen" << endl;
    cout << "    [-n numPoints]  Sets the number of points to generate. Defaults to 10" << endl;
    cout << "    [-s seed]       Sets the seed for the random number generator." << endl;
    cout << "                    If not set, a random seed based on the time will be chosen" << endl;
    cout << "    [-r range]      Sets the upper limit of the range for x and y values." << endl;
    cout << "                    Defaults to 100." << endl;
    cout << "    [-i castToInts] If 0, the points generated will be doubles." << endl;
    cout << "                    Otherwise the x and y values will be generated as integers." << endl;
    cout << "                    Defaults to 1." << endl << endl;
}

int main(int argc, char* argv[]){
    int seed = time(NULL);
    int num = DEFAULT_NUM_POINTS;
    int range = DEFAULT_POINT_RANGE;
    bool castToInt = DEFAULT_CAST;
    bool writeFile = false;
    char* file;
    if(argc > 1){
        if (string(argv[1]) == "-help") {
            printHelp();
            return 0;
        }
        
        for (int i = 1; i < argc; i += 2) {
            string arg = string(argv[i]);
            if (arg == "-f") {
                writeFile = true;
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
    
    vector<Point> list = genData(num, seed, range, castToInt);    
    
    if(writeFile){
        writeData(file, list); 
    } else {
        for(int i = 0; i < num; i++){
            cout << list[i].getX() << " " << list[i].getY() << endl;
        }
    }
    return 0;
}