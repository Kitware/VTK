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

#include <vtkActor.h>
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
    vector<float> xCoordinates;
    vector<float> yCoordinates;
    bool isBigEndian;
    int crossLineNumberStep;
    int traceCount;
    int formatCode;

    int minCrossLineNumber;
    int maxCrossLineNumber;

    int crosslineNumberCount;
    int sampleCount;

    long fileSize;
public:
    void render2d(vtkActor*  );

    void normalizeCoordinates(vector<float>& coordinates)
    {
        if(coordinates.size() == 0)
            return;

        float min = coordinates[0], max = coordinates[0];
        for(int i=0; i<coordinates.size(); i++)
        {
            min = min < coordinates[i] ? min : coordinates[i];
            max = max > coordinates[i] ? max : coordinates[i];
        }

        float range = max - min;
        if(range < 0.000001)
            return;

        for(int i=0; i<coordinates.size(); i++)
        {
            coordinates[i] = (coordinates[i] - min) / range;
        }
    }
};


#endif //SEGYVISUALIZER_SEGYREADER_H
