//
// Created by jia chen on 8/18/15.
//

#ifndef SEGYVISUALIZER2D_RDVREADER_H
#define SEGYVISUALIZER2D_RDVREADER_H


#include <vtkPolyData.h>
using namespace std;
class RdvReader {
public:
    bool Read(string path, vtkPolyData* polyData);
};


#endif //SEGYVISUALIZER2D_RDVREADER_H
