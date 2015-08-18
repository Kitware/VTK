#ifndef SEGYVISUALIZER2D_SEGYTRACEREADER_H
#define SEGYVISUALIZER2D_SEGYTRACEREADER_H

#include <fstream>
#include "Trace.h"
#include "TraceHeaderBytesPositions.h"
using namespace std;

/// Read Single Trace
class SegyTraceReader {
private:
    TraceHeaderBytesPositions traceHeaderBytesPos;
public:
    void printTraceHeader(ifstream& in, int startPos);
    bool readTrace(int &startPos, ifstream &in, int formatCode, Trace* trace);
    int getTraceSize(int numSamples, int formatCode);
};


#endif //SEGYVISUALIZER2D_SEGYTRACEREADER_H
