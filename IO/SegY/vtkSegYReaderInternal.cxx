/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYReaderInternal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSegYReaderInternal.h"

#include "vtkArrayData.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSegYBinaryHeaderBytesPositions.h"
#include "vtkSegYIOUtils.h"
#include "vtkSegYTraceReader.h"
#include "vtkStructuredGrid.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <map>
#include <set>

namespace {
  const int FIRST_TRACE_START_POS = 3600;  // this->Traces start after 3200 + 400 file header
  double decodeMultiplier(short multiplier)
  {
    return
      (multiplier < 0) ?
      (-1.0 / multiplier)
      : (multiplier > 0 ? multiplier : 1.0);
  }

};

//-----------------------------------------------------------------------------
vtkSegYReaderInternal::vtkSegYReaderInternal() :
  SampleInterval(0), FormatCode(0), SampleCountPerTrace(0)
{
  this->BinaryHeaderBytesPos = new vtkSegYBinaryHeaderBytesPositions();
  this->VerticalCRS = 0;
  this->TraceReader = new vtkSegYTraceReader();
}

//-----------------------------------------------------------------------------
vtkSegYReaderInternal::~vtkSegYReaderInternal()
{
  delete this->BinaryHeaderBytesPos;
  delete this->TraceReader;
  for (auto trace : this->Traces)
    delete trace;
}

//-----------------------------------------------------------------------------
void vtkSegYReaderInternal::SetXYCoordBytePositions(int x, int y)
{
  this->TraceReader->SetXYCoordBytePositions(x, y);
}

//-----------------------------------------------------------------------------
void vtkSegYReaderInternal::SetVerticalCRS(int v)
{
  this->VerticalCRS = v > 0 ? 1 : 0;
}

//-----------------------------------------------------------------------------
void vtkSegYReaderInternal::LoadTraces()
{
  int traceStartPos = FIRST_TRACE_START_POS;
  std::streamoff fileSize = vtkSegYIOUtils::Instance()->getFileSize(this->In);
  while (traceStartPos + 240 < fileSize)
  {
    vtkSegYTrace* pTrace = new vtkSegYTrace();
    this->TraceReader->ReadTrace(traceStartPos, this->In, this->FormatCode, pTrace);
    this->Traces.push_back(pTrace);
  }
}

//-----------------------------------------------------------------------------
bool vtkSegYReaderInternal::ReadHeader()
{
  this->SampleInterval = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->SampleInterval, this->In);
  this->FormatCode = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->FormatCode, this->In);
  this->SampleCountPerTrace = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->NumSamplesPerTrace, this->In);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegYReaderInternal::Is3DComputeParameters(
  int* extent, double* origin, double* spacing, int* spacingSign)
{
  this->ReadHeader();
  int traceStartPos = FIRST_TRACE_START_POS;
  std::streamoff fileSize = vtkSegYIOUtils::Instance()->getFileSize(this->In);
  int crosslineFirst, crosslineSecond, inlineFirst = 0,
    inlineNumber = 0, crosslineNumber;
  double coordFirst[3] = {0},
    coordSecondX[3] = {0},
    coordSecondY[3] = {0},
    d[3];
  int xCoord = 0, yCoord = 0;
  short coordMultiplier = 0;
  int prevTraceStartPos = traceStartPos;
  int crosslineCount = 0;
  if (traceStartPos + 240 < fileSize)
  {
    this->TraceReader->ReadInlineCrossline
      (traceStartPos, this->In, this->FormatCode,
       &inlineFirst, &crosslineFirst,
       &xCoord, &yCoord, &coordMultiplier);
    double coordinateMultiplier = decodeMultiplier(coordMultiplier);
    coordFirst[0] = coordinateMultiplier * xCoord;
    coordFirst[1] = coordinateMultiplier * yCoord;
    coordFirst[2] = 0;
    ++crosslineCount;
  }
  int traceSize = traceStartPos - prevTraceStartPos;
  if (traceStartPos + 240 < fileSize)
  {
    this->TraceReader->ReadInlineCrossline
      (traceStartPos, this->In, this->FormatCode,
       &inlineNumber, &crosslineSecond,
       &xCoord, &yCoord, &coordMultiplier);
    double coordinateMultiplier = decodeMultiplier(coordMultiplier);
    coordSecondX[0] = coordinateMultiplier * xCoord;
    coordSecondX[1] = coordinateMultiplier * yCoord;
    coordSecondX[2] = 0;
    ++crosslineCount;
  }
  vtkMath::Subtract(coordSecondX, coordFirst, d);
  spacingSign[0] = d[0] >= 0 ? 1 : -1;
  float xStep = vtkMath::Norm(d);
  while(inlineFirst == inlineNumber && traceStartPos + 240 < fileSize)
  {
    this->TraceReader->ReadInlineCrossline
      (traceStartPos, this->In, this->FormatCode,
       &inlineNumber, &crosslineNumber,
       &xCoord, &yCoord, &coordMultiplier);
    ++crosslineCount;
  }
  if (traceStartPos + 240 < fileSize)
  {
    // we read a crossline from the next inline
    --crosslineCount;
  }
  int inlineCount = (fileSize - FIRST_TRACE_START_POS) / traceSize / crosslineCount;
  auto e = {
    0, crosslineCount - 1,
    0, inlineCount - 1,
    0, this->SampleCountPerTrace - 1,
  };
  std::copy(e.begin(), e.end(), extent);
  if (traceStartPos + 240 >= fileSize)
  {
    // this is a 2D dataset
    return false;
  }
  double coordinateMultiplier = decodeMultiplier(coordMultiplier);
  coordSecondY[0] = coordinateMultiplier * xCoord;
  coordSecondY[1] = coordinateMultiplier * yCoord;
  coordSecondY[2] = 0;
  vtkMath::Subtract(coordSecondY, coordFirst, d);
  spacingSign[1] = d[1] >= 0 ? 1 : -1;
  spacingSign[2] = (this->VerticalCRS == 0 ? -1 : 1); // goes
  float yStep = vtkMath::Norm(d);

  // The samples are uniformly placed at sample interval depths
  // Dividing by 1000.0 to convert from microseconds to milliseconds.
  float zStep = this->SampleInterval / 1000.0;
  std::array<double, 3> o = {{coordFirst[0],
                              coordFirst[1],
                              - zStep * (this->SampleCountPerTrace - 1)}};
  std::copy(o.begin(), o.end(), origin);
  auto s = {xStep, yStep, zStep};
  std::copy(s.begin(), s.end(), spacing);
  return true;
}




//-----------------------------------------------------------------------------
void vtkSegYReaderInternal::ExportData(
  vtkImageData* imageData,
  int* extent, double* origin, double* spacing, int* spacingSign)
{
  imageData->SetExtent(extent);
  imageData->SetOrigin(origin);
  imageData->SetSpacing(spacing);
  int* dims = imageData->GetDimensions();

  vtkNew<vtkFloatArray> scalars;
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(dims[0] * dims[1] * dims[2]);
  scalars->SetName("trace");
  imageData->GetPointData()->SetScalars(scalars);
  int id = 0;
  int destK, destJ, destI;
  for (int k = 0; k < dims[2]; ++k)
  {
    destK = (spacingSign[2] > 0 ? k : dims[2] - k - 1);
    for (int j = 0; j < dims[1]; ++j)
    {
      destJ = (spacingSign[1] > 0 ? j : dims[1] - j - 1);
      for (int i = 0; i < dims[0]; ++i)
      {
        destI = (spacingSign[0] > 0 ? i : dims[0] - i - 1);
        vtkSegYTrace* trace = this->Traces[destJ * dims[0] + destI];
        scalars->SetValue(id++, trace->Data[destK]);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSegYReaderInternal::ExportData(vtkStructuredGrid* grid, int* extent)
{
  if (!grid)
  {
    return;
  }
  grid->SetExtent(extent);
  int* dims = grid->GetDimensions();
  vtkNew<vtkPoints> points;

  vtkNew<vtkFloatArray> scalars;
  scalars->SetName("trace");
  scalars->SetNumberOfComponents(1);
  scalars->Allocate(dims[0] * dims[1] * dims[2]);

  int sign = this->VerticalCRS == 0 ? -1 : 1;
  int id = 0;
  for (int k = 0; k < dims[2]; ++k)
  {
    for (int j = 0; j < dims[1]; ++j)
    {
      for (int i = 0; i < dims[0]; ++i)
      {
        auto trace = this->Traces[j * dims[0] + i];
        double coordinateMultiplier = decodeMultiplier(trace->CoordinateMultiplier);
        double x = coordinateMultiplier * trace->XCoordinate;
        double y = coordinateMultiplier * trace->YCoordinate;

        // The samples are uniformly placed at sample interval depths
        // Dividing by 1000.0 to convert from microseconds to milliseconds.
        double z = sign * k * (trace->SampleInterval / 1000.0);
        points->InsertNextPoint(x, y, z);
        scalars->InsertValue(id++, trace->Data[k]);
      }
    }
  }

  grid->SetPoints(points);
  grid->GetPointData()->SetScalars(scalars);
}
