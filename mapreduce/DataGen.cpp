
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include "Point.cpp"

using namespace std;

#define PI 3.14159265
#define NUM_SAMPLES 12

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

Point genRandCirc(){
    double length = fRand(0, 1) - 0.5;
    double angle = fRand(0, PI);
    double x = cos(angle) * length;
    double y = sin(angle) * length;
    return Point(x, y);
}

vector<Point> genClusters(int k, double stdDev, int num, int seed, int range, bool castToInt, vector<Point> &centers){
    vector<Point> output;
    output.resize(num);
    srand(seed);
    for(int i = 0; i < k; i++){
        double x, y;
        if(castToInt){
            x = (double)(rand() % range);
            y = (double)(rand() % range);
        } else {
            x = fRand(0, range);
            y = fRand(0, range);
        }
        centers.push_back(Point(x, y));
    }
    for(int i = 0; i < num; i++){
        Point center = centers[rand() % k];
        Point p;
        for(int j = 0; j < NUM_SAMPLES; j++){
            p = p + genRandCirc();
        }
        p = p * stdDev;
        p = p + center;
        if(castToInt){
            p.setX(floor(p.getY()));
            p.setY(floor(p.getY()));
        }
        output[i] = p;
    }
    return output;
}

vector<Point> readData(char* filename, vector<Point> &centers){
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
    
    if(!inFile.eof()){
        inFile >> num;
        centers.resize(num);
        for(int i = 0; i < num; i++){
            double x, y;
            inFile >> x >> y;
            centers[i].setX(x);
            centers[i].setY(y);
        }
    }
    
    inFile.close();
    return output;
}

vector<Point> readData(char* filename){
    vector<Point> tempCenters;
    return readData(filename, tempCenters);
}

void writeData(char* filename, vector<Point> &points, vector<Point> &centers){
    ofstream outFile(filename);
    outFile << points.size() << endl;
    for(int i = 0; i < points.size(); i++){
        outFile << points[i].getX() << " " << points[i].getY() << endl;
    }
    if(centers.size() > 0){
        outFile << centers.size() << endl;
        for(int i = 0; i < centers.size(); i++){
            outFile << centers[i].getX() << " " << centers[i].getY() << endl;
        }
    }
    outFile.close();
}


void writeData(char* filename, vector<Point> &points){
    vector<Point> tempCenters;
    writeData(filename, points, tempCenters);
}