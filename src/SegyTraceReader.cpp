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

#include "SegyTraceReader.h"
#include <iostream>

#include "IOUtil.h"

void SegyTraceReader::printTraceHeader(ifstream &in, int startPos) {

//    cout << "Position:" << startPos << endl;

    int traceSequenceNumberInLine = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.TraceNumber, in);
    cout << "Trace sequence number in line : " << traceSequenceNumberInLine << endl;

    int traceSequenceNumberInFile = IOUtil::Instance()->readLongInteger(in);
    cout << "Trace sequence number in file : " << traceSequenceNumberInFile << endl;


  // Get number_of_samples from trace header position 115-116
    int numSamples = IOUtil::Instance()->readShortInteger(startPos + traceHeaderBytesPos.NumberSamples, in);
    cout << "number of samples: " << numSamples << endl;

    short sampleInterval = IOUtil::Instance()->readShortInteger(startPos + traceHeaderBytesPos.SampleInterval, in);
    cout << "sample interval: " << sampleInterval << endl;


// Get inline number from trace header position 189-192
    int inlineNum = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.InlineNumber, in);
    cout << "Field record number (inline number) : " << inlineNum << endl;

    int crosslineNum = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.CrosslineNumber, in);
    cout << "cross-line number (ensemble number) : " << crosslineNum << endl;

    int traceNumberWithinEnsemble = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.TraceNumberWithinEnsemble, in);
    cout << "trace number within ensemble : " << traceNumberWithinEnsemble << endl;

    short coordinateMultiplier = IOUtil::Instance()->readShortInteger(startPos + traceHeaderBytesPos.CoordinateMultiplier, in);
    cout << "coordinate multiplier : " << coordinateMultiplier << endl;

    int xCoordinate = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.XCoordinate, in);
    cout << "X coordinate for ensemble position of the trace : " << xCoordinate << endl;

    int yCoordinate = IOUtil::Instance()->readLongInteger(startPos + traceHeaderBytesPos.YCoordinate, in);
    cout << "Y coordinate for ensemble position of the trace : " << yCoordinate << endl;

    short coordinateUnits = IOUtil::Instance()->readShortInteger(startPos + traceHeaderBytesPos.CoordinateUnits, in);
    cout << "coordinateUnits: " << coordinateUnits << endl;
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
    trace->CoordinateMultiplier = IOUtil::Instance()->readShortInteger(startPos + traceHeaderBytesPos.CoordinateMultiplier, in);
    trace->SampleInterval = IOUtil::Instance()->readShortInteger(startPos + traceHeaderBytesPos.SampleInterval, in);

    in.seekg(startPos + 240, in.beg);
    //std::cout << "Values: ";
    for (int i = 0; i < numSamples; i++) {
        float value;
        switch (formatCode)
        {
        case 1:
          value = IOUtil::Instance()->readIBMFloat(in);
          break;
        case 5:
          value = IOUtil::Instance()->readFloat(in);
          break;
        default:
          std::cerr << "Data sample format code " << formatCode << " not supported." << std::endl;
          value = 0;
        }
        trace->data.push_back(value);
        //std::cout << value << " ";
    }
    cout << endl;

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
