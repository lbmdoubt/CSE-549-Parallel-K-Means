
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include "Point.cpp"

using namespace std;

double fRand(double fMin, double fMax){
    double f = (double)rand() / RAND_MAX;
    return fMin + f * (fMax - fMin);
}

vector<Point> genData(int num, int seed, int range, bool castToInt){
    vector<Point> output;
    output.resize(num);
    srand(seed);
    for(int i = 0; i < num; i++){
        double x, y;
        if(castToInt){
            x = (double)(rand() % range);
            y = (double)(rand() % range);
        } else {
            x = fRand(0, range);
            y = fRand(0, range);
        }
        output[i] = Point(x, y);
    }
    return output;
}

vector<Point> readData(char* filename){
    vector<Point> output;
    int num;
    ifstream inFile(filename);
    inFile >> num;
    output.resize(num);
    for(int i = 0; i < num; i++){
        double x, y;
        inFile >> x >> y;
        output[i].setX(x);
        output[i].setY(y);
    }
    return output;
}