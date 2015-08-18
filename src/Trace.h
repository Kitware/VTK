//
// Created by jia chen on 8/3/15.
//

#ifndef SEGYVISUALIZER2D_TRACE_H
#define SEGYVISUALIZER2D_TRACE_H

#include <vector>
using namespace std;

class Trace {
public:
    float xCoordinate;
    float yCoordinate;
    vector<float> data;
    int inlineNumber;
    int crosslineNumber;
};


#endif //SEGYVISUALIZER2D_TRACE_H
