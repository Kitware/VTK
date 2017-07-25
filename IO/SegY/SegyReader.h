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

#ifndef SEGYVISUALIZER_SEGYREADER_H
#define SEGYVISUALIZER_SEGYREADER_H

#include "BinaryHeaderBytesPositions.h"
#include "TraceHeaderBytesPositions.h"
#include "SegyTraceReader.h"
#include "IOUtil.h"
#include "Trace.h"

#include <vtkImageData.h>
#include <vtkActor.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

using namespace std;
class vtkStructuredGrid;

class SegyReader
{
public:
    SegyReader(){}
    virtual ~SegyReader();


public:
    bool GetImageData(vtkImageData *); // export the data in segy file as 3D volume
    bool ExportData3D(vtkImageData *); // export the data in segy file as 3D volume
    bool LoadFromFile(string path);
    void ExportData2D(vtkStructuredGrid *);
    void AddScalars(vtkStructuredGrid *);
private:
    bool readHeader(ifstream& in);
    vector<Trace*> traces;
    int formatCode;
    BinaryHeaderBytesPositions binaryHeaderBytesPos;
    SegyTraceReader traceReader;
    int sampleCountPerTrace;
};


#endif //SEGYVISUALIZER_SEGYREADER_H
