//
// Created by jia chen on 6/30/15.
//

#ifndef SEGYVISUALIZER_SEGYREADER_H
#define SEGYVISUALIZER_SEGYREADER_H
#include <iostream>
#include <fstream>
#include <vector>
#include <vtkImageData.h>
#include "BinaryHeaderBytesPositions.h"
#include "TraceHeaderBytesPositions.h"
using namespace std;

class SegyReader {
public:
    SegyReader();
    void ExportData(vtkImageData* ); // export the data in segy file as 3D volume
    bool LoadFromFile(string path);
private:

    void printBinaryHeader(ifstream& in);
    void printTraceHeader(ifstream& in, int startPos);

    bool readTextualHeader(ifstream& in);
    bool readBinaryHeader(ifstream& in);

    bool readTrace(int &startPos, ifstream &in, int formatCode);

    int readShortInteger(int pos, ifstream &in);
    int readLongInteger(int pos, ifstream &in);
    float readFloat(ifstream &in);
    char readChar(ifstream &in);

    int getTraceSize(int numSamples, int formatCode);
    int getFileSize(ifstream& in);
    int getFormatCode(ifstream& in);

    bool checkIfBigEndian();
    void swap(char* a, char* b);
    void scanFile(ifstream& in);
private:
    BinaryHeaderBytesPositions binaryHeaderBytesPos;
    TraceHeaderBytesPositions traceHeaderBytesPos;

    vector<float> data;
    bool isBigEndian;
    int crossLineNumberStep;
    int traceNumberStep;
    int traceCount;
    int formatCode;

    int minTraceNumber;
    int maxTraceNumber;
    int minCrossLineNumber;
    int maxCrossLineNumber;

    int traceNumberCount;
    int crosslineNumberCount;

    int sampleCount;
};


#endif //SEGYVISUALIZER_SEGYREADER_H
