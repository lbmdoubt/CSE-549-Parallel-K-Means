#pragma once

#include "Point.h"
#include <cmath>

using namespace std;

Point::Point(){
    x = 0;
    y = 0;
}

Point::Point(double x1, double y1){
    x = x1;
    y = y1;
}

double Point::getX(){
    return x;
}

double Point::getY(){
    return y;
}

void Point::setX(double val){
    x = val;
}

void Point::setY(double val){
    y = val;
}

double Point::distance(Point p){
    return distance(p.x, p.y);
}

double Point::distance(double x1, double y1){
    return sqrt(pow(x1 - x, 2) + pow(y1 - y, 2));
}

void Point::copy(Point p){
    x = p.x;
    y = p.y;
}

Point operator*(const Point &base, const double mult){
    return Point(base.x * mult, base.y * mult);
}

Point operator+(const Point &left, const Point &right){
    return Point(left.x + right.x, left.y + right.y);
}

bool operator==(const Point &left, const Point &right){
    return left.x == right.x && left.y == right.y;
}