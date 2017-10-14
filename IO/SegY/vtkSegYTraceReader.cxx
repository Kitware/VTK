/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYTraceReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSegYTraceReader.h"
#include "vtkSegYIOUtils.h"

#include <iostream>

//-----------------------------------------------------------------------------
vtkSegYTraceReader::vtkSegYTraceReader()
{
  this->XCoordinate = 72;
  this->YCoordinate = 76;
}

//-----------------------------------------------------------------------------
void vtkSegYTraceReader::SetXYCoordBytePositions(int x, int y)
{
  this->XCoordinate = x;
  this->YCoordinate = y;
}

//-----------------------------------------------------------------------------
void vtkSegYTraceReader::PrintTraceHeader(std::ifstream& in, int startPos)
{
  int traceSequenceNumberInLine = vtkSegYIOUtils::Instance()->readLongInteger(
    startPos + traceHeaderBytesPos.TraceNumber, in);
  std::cout << "Trace sequence number in line : " << traceSequenceNumberInLine
            << std::endl;

  int traceSequenceNumberInFile =
    vtkSegYIOUtils::Instance()->readLongInteger(in);
  std::cout << "Trace sequence number in file : " << traceSequenceNumberInFile
            << std::endl;

  // Get number_of_samples from trace header position 115-116
  int numSamples = vtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.NumberSamples, in);
  std::cout << "number of samples: " << numSamples << std::endl;

  short sampleInterval = vtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.SampleInterval, in);
  std::cout << "sample interval: " << sampleInterval << std::endl;

  // Get inline number from trace header position 189-192
  int inlineNum = vtkSegYIOUtils::Instance()->readLongInteger(
    startPos + traceHeaderBytesPos.InlineNumber, in);
  std::cout << "Field record number (inline number) : " << inlineNum
            << std::endl;

  int crosslineNum = vtkSegYIOUtils::Instance()->readLongInteger(
    startPos + traceHeaderBytesPos.CrosslineNumber, in);
  std::cout << "cross-line number (ensemble number) : " << crosslineNum
            << std::endl;

  int traceNumberWithinEnsemble = vtkSegYIOUtils::Instance()->readLongInteger(
    startPos + traceHeaderBytesPos.TraceNumberWithinEnsemble, in);
  std::cout << "trace number within ensemble : " << traceNumberWithinEnsemble
            << std::endl;

  short coordinateMultiplier = vtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.CoordinateMultiplier, in);
  std::cout << "coordinate multiplier : " << coordinateMultiplier << std::endl;

  int xCoordinate = vtkSegYIOUtils::Instance()->readLongInteger(
    startPos + this->XCoordinate, in);
  std::cout << "X coordinate for ensemble position of the trace : "
            << xCoordinate << std::endl;

  int yCoordinate = vtkSegYIOUtils::Instance()->readLongInteger(
    startPos + this->YCoordinate, in);
  std::cout << "Y coordinate for ensemble position of the trace : "
            << yCoordinate << std::endl;

  short coordinateUnits = vtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.CoordinateUnits, in);
  std::cout << "coordinateUnits: " << coordinateUnits << std::endl;
}

//-----------------------------------------------------------------------------
bool vtkSegYTraceReader::ReadTrace(int& startPos,
  std::ifstream& in,
  int formatCode,
  vtkSegYTrace* trace)
{
  int fileSize = vtkSegYIOUtils::Instance()->getFileSize(in);

  if (startPos + 240 >= fileSize)
  {
    return false;
  }

  // PrintTraceHeader(in, startPos);
  trace->crosslineNumber = vtkSegYIOUtils::Instance()->readLongInteger(
    startPos + traceHeaderBytesPos.CrosslineNumber, in);
  trace->inlineNumber = vtkSegYIOUtils::Instance()->readLongInteger(
    startPos + traceHeaderBytesPos.InlineNumber, in);
  int numSamples = vtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.NumberSamples, in);
  trace->xCoordinate = vtkSegYIOUtils::Instance()->readLongInteger(
    startPos + this->XCoordinate, in);
  trace->yCoordinate = vtkSegYIOUtils::Instance()->readLongInteger(
    startPos + this->YCoordinate, in);
  trace->CoordinateMultiplier = vtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.CoordinateMultiplier, in);
  trace->SampleInterval = vtkSegYIOUtils::Instance()->readShortInteger(
    startPos + traceHeaderBytesPos.SampleInterval, in);

  in.seekg(startPos + 240, in.beg);
  for (int i = 0; i < numSamples; i++)
  {
    float value;
    switch (formatCode)
    {
      case 1:
        value = vtkSegYIOUtils::Instance()->readIBMFloat(in);
        break;
      case 5:
        value = vtkSegYIOUtils::Instance()->readFloat(in);
        break;
      default:
        std::cerr << "Data sample format code " << formatCode
                  << " not supported." << std::endl;
        value = 0;
    }
    trace->data.push_back(value);
  }

  startPos += 240 + GetTraceSize(numSamples, formatCode);
  return true;
}

//-----------------------------------------------------------------------------
int vtkSegYTraceReader::GetTraceSize(int numSamples, int formatCode)
{
  if (formatCode == 1 || formatCode == 2 || formatCode == 4 || formatCode == 5)
  {
    return 4 * numSamples;
  }
  if (formatCode == 3)
  {
    return 2 * numSamples;
  }
  if (formatCode == 8)
  {
    return numSamples;
  }
  std::cerr << "Unsupported data format code : " << formatCode << std::endl;
  return -1;
}
