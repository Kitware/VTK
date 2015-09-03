#ifndef SEGYVISUALIZER_SEGYREADER_H
#define SEGYVISUALIZER_SEGYREADER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

#include <vtkImageData.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataAlgorithm.h>

#include "BinaryHeaderBytesPositions.h"
#include "TraceHeaderBytesPositions.h"
#include "SegyTraceReader.h"
#include "IOUtil.h"
#include "Trace.h"

using namespace std;

class SegyReader
{
public:
    SegyReader(){}
    virtual ~SegyReader();


public:
    bool ExportData3D(vtkImageData *); // export the data in segy file as 3D volume
    bool LoadFromFile(string path);
    bool ExportData2D(vtkPolyData *);
private:
    bool readHeader(ifstream& in);
    vector<Trace*> data;
    int formatCode;
    BinaryHeaderBytesPositions binaryHeaderBytesPos;
    SegyTraceReader traceReader;
    int sampleCountPerTrace;
};


#endif //SEGYVISUALIZER_SEGYREADER_H
