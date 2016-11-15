/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
