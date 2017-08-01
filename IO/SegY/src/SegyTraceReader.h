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

#ifndef SEGYVISUALIZER2D_SEGYTRACEREADER_H
#define SEGYVISUALIZER2D_SEGYTRACEREADER_H

#include <fstream>
#include "Trace.h"
#include "TraceHeaderBytesPositions.h"
using namespace std;

/// Single Trace Reader
class SegyTraceReader {
private:
    TraceHeaderBytesPositions traceHeaderBytesPos;
public:
    void printTraceHeader(ifstream& in, int startPos);
    bool readTrace(int &startPos, ifstream &in, int formatCode, Trace* trace);
    int getTraceSize(int numSamples, int formatCode);
};


#endif //SEGYVISUALIZER2D_SEGYTRACEREADER_H
