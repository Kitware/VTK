#include "SegyTraceReader.h"
#include <iostream>

#include "IOUtil.h"

void SegyTraceReader::printTraceHeader(ifstream &in, int startPos) {

    cout << "Position:" << startPos << endl;

    int traceSequenceNumberInLine = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.TraceNumber, in);
    cout << "Trace sequence number in line : " << traceSequenceNumberInLine << endl;

// Get number_of_samples from trace header position 115-116
    int numSamples = IOUtil::Instance()->readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);
    cout << "number of samples: " << numSamples << endl;

// Get inline number from trace header position 189-192
    int inlineNum = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.InlineNumber, in);
    cout << "in-line number : " << inlineNum << endl;

    int crosslineNum = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);
    cout << "cross-line number : " << crosslineNum << endl;

    int xCoordinate = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.XCoordinate, in);
    cout << "X coordinate for ensemble position of the trace : " << xCoordinate << endl;

    int yCoordinate = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.YCoordinate, in);
    cout << "Y coordinate for ensemble position of the trace : " << yCoordinate << endl;
}

bool SegyTraceReader::readTrace(int &startPos, ifstream &in, int formatCode, Trace* trace) {
    int fileSize = IOUtil::Instance()->getFileSize(in);

    if (startPos + 240 >= fileSize)
        return false;

    printTraceHeader(in, startPos);
    trace->crosslineNumber = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);
    trace->inlineNumber = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.InlineNumber, in);
    int numSamples = IOUtil::Instance()->readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);
    trace->xCoordinate = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.XCoordinate, in);
    trace->yCoordinate = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.YCoordinate, in);



    in.seekg(startPos + 240, in.beg);
    for (int i = 0; i < numSamples; i++) {
        float value;
        if(formatCode == 8)
            value = IOUtil::Instance()->readChar(in);
        else
            value = IOUtil::Instance()->readFloat(in); // TODO: readChar or readFloat according to format code
        trace->data.push_back(value);
    }

    startPos += 240 + getTraceSize(numSamples, formatCode);
    return true;
}

int SegyTraceReader::getTraceSize(int numSamples, int formatCode) {
    if (formatCode == 1 || formatCode == 2 || formatCode == 4 || formatCode == 5) {
        return 4 * numSamples;
    }
    if (formatCode == 3) {
        return 2 * numSamples;
    }
    if (formatCode == 8) {
        return numSamples;
    }
    cout << "Unsupported data format code : " << formatCode << endl;
}