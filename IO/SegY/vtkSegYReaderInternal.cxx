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

namespace
{
const int FIRST_TRACE_START_POS = 3600; // this->Traces start after 3200 + 400 file header
double decodeMultiplier(short multiplier)
{
  return (multiplier < 0) ? (-1.0 / multiplier) : (multiplier > 0 ? multiplier : 1.0);
}
};

//-----------------------------------------------------------------------------
vtkSegYReaderInternal::vtkSegYReaderInternal()
  : SampleInterval(0)
  , FormatCode(0)
  , SampleCountPerTrace(0)
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
  {
    delete trace;
  }
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
void vtkSegYReaderInternal::LoadTraces(int* extent)
{
  std::streamoff traceStartPos = FIRST_TRACE_START_POS;
  std::streamoff fileSize = vtkSegYIOUtils::Instance()->getFileSize(this->In);

  // allocate traces vector
  int dims[3] = { extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1 };

  bool is3d = (extent[3] - extent[2] > 1) ? true : false;
  this->Traces.resize(dims[0] * dims[1], nullptr);
  size_t traceCount = 0;
  while (traceStartPos + 240 < fileSize)
  {
    vtkSegYTrace* pTrace = new vtkSegYTrace();
    this->TraceReader->ReadTrace(traceStartPos, this->In, this->FormatCode, pTrace);
    size_t loc = traceCount;
    if (is3d)
    {
      loc = pTrace->CrosslineNumber - extent[0] + (pTrace->InlineNumber - extent[2]) * dims[0];
    }
    this->Traces[loc] = pTrace;
    traceCount++;
  }
}

//-----------------------------------------------------------------------------
bool vtkSegYReaderInternal::ReadHeader()
{
  this->SampleInterval = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->SampleInterval, this->In);
  this->FormatCode =
    vtkSegYIOUtils::Instance()->readShortInteger(this->BinaryHeaderBytesPos->FormatCode, this->In);
  this->SampleCountPerTrace = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->NumSamplesPerTrace, this->In);
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegYReaderInternal::Is3DComputeParameters(
  int* extent, double origin[3], double spacing[3][3], int* spacingSign, bool force2D)
{
  this->ReadHeader();
  std::streamoff traceStartPos = FIRST_TRACE_START_POS;
  std::streamoff fileSize = vtkSegYIOUtils::Instance()->getFileSize(this->In);
  int inlineNumber = 0, crosslineNumber;
  int xCoord = 0, yCoord = 0;
  short coordMultiplier = 0;

  size_t traceCount = 0;

  // for the forced 2D case we ignore lines/crosslines and just stitch together the
  // traces in order applying their x,y coordinates
  if (force2D)
  {
    while (traceStartPos + 240 < fileSize)
    {
      this->TraceReader->ReadInlineCrossline(traceStartPos, this->In, this->FormatCode,
        &inlineNumber, &crosslineNumber, &xCoord, &yCoord, &coordMultiplier);
      traceCount++;
    }
    extent[0] = 0;
    extent[1] = static_cast<int>(traceCount - 1);
    extent[2] = 0;
    extent[3] = 0;
    extent[4] = 0;
    extent[5] = this->SampleCountPerTrace - 1;
    return false;
  }

  // compute the dimensions of the dataset, to be safe we
  // look at all the traces and compute the set of inline
  // and crossline indicies
  std::set<int> crossLines;
  std::map<int, std::array<double, 3> > crossCoordinates;
  std::set<int> inLines;
  int basisPointCount = 0;
  double basisCoords[3][3];
  int basisIndex[3][2] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
  double iBasis[2][3];
  double basisLength[2];

  while (traceStartPos + 240 < fileSize)
  {
    this->TraceReader->ReadInlineCrossline(traceStartPos, this->In, this->FormatCode, &inlineNumber,
      &crosslineNumber, &xCoord, &yCoord, &coordMultiplier);
    traceCount++;
    double coordinateMultiplier = decodeMultiplier(coordMultiplier);

    // store a third point, must have different basis from
    // first two
    if (basisPointCount == 2)
    {
      iBasis[1][0] = crosslineNumber - basisIndex[0][0];
      iBasis[1][1] = inlineNumber - basisIndex[0][1];
      iBasis[1][2] = 0.0;
      basisLength[1] = vtkMath::Normalize(iBasis[1]);
      if (fabs(vtkMath::Dot(iBasis[0], iBasis[1])) < 0.99)
      {
        basisCoords[basisPointCount][0] = coordinateMultiplier * xCoord;
        basisCoords[basisPointCount][1] = coordinateMultiplier * yCoord;
        basisCoords[basisPointCount][2] = 0;
        basisIndex[basisPointCount][0] = crosslineNumber;
        basisIndex[basisPointCount][1] = inlineNumber;
        basisPointCount++;
      }
    }

    // store a second point, just any point other than first
    if (basisPointCount == 1)
    {
      basisCoords[basisPointCount][0] = coordinateMultiplier * xCoord;
      basisCoords[basisPointCount][1] = coordinateMultiplier * yCoord;
      basisCoords[basisPointCount][2] = 0;
      basisIndex[basisPointCount][0] = crosslineNumber;
      basisIndex[basisPointCount][1] = inlineNumber;
      iBasis[0][0] = basisIndex[1][0] - basisIndex[0][0];
      iBasis[0][1] = basisIndex[1][1] - basisIndex[0][1];
      iBasis[0][2] = 0.0;
      basisLength[0] = vtkMath::Normalize(iBasis[0]);
      basisPointCount++;
    }

    // store a first point
    if (basisPointCount == 0)
    {
      basisCoords[basisPointCount][0] = coordinateMultiplier * xCoord;
      basisCoords[basisPointCount][1] = coordinateMultiplier * yCoord;
      basisCoords[basisPointCount][2] = 0;
      basisIndex[basisPointCount][0] = crosslineNumber;
      basisIndex[basisPointCount][1] = inlineNumber;
      basisPointCount++;
    }

    inLines.insert(inlineNumber);
    crossLines.insert(crosslineNumber);
  }

  // find the min and max to get the extent
  int startCross = *crossLines.begin();
  int endCross = *crossLines.rbegin();
  int crosslineCount = endCross - startCross + 1;
  int startInline = *inLines.begin();
  int endInline = *inLines.rbegin();
  int inlineCount = endInline - startInline + 1;

  auto e = {
    startCross,
    endCross,
    startInline,
    endInline,
    0,
    this->SampleCountPerTrace - 1,
  };
  std::copy(e.begin(), e.end(), extent);
  if (inlineCount <= 1) // should really be 1 in either inline or crossline?
  {
    // this is a 2D dataset
    // watch for cases where there are more traces than crosslines as
    if (static_cast<int>(traceCount) > crosslineCount)
    {
      extent[0] = 0;
      extent[1] = static_cast<int>(traceCount) - 1;
    }
    return false;
  }

  // compute the mapping of indicies into coords if we have three
  if (basisPointCount == 3)
  {
    // compute an orthogonal basis
    double bDot = vtkMath::Dot(iBasis[0], iBasis[1]);
    iBasis[1][0] -= bDot * iBasis[0][0];
    iBasis[1][1] -= bDot * iBasis[0][1];
    vtkMath::Normalize(iBasis[1]);

    // coordinate vectors
    double cBasis[2][3];
    cBasis[0][0] = basisCoords[1][0] - basisCoords[0][0];
    cBasis[0][1] = basisCoords[1][1] - basisCoords[0][1];
    cBasis[0][2] = 0.0;
    cBasis[1][0] = basisCoords[2][0] - basisCoords[0][0] - bDot * cBasis[0][0];
    cBasis[1][1] = basisCoords[2][1] - basisCoords[0][1] - bDot * cBasis[0][1];
    cBasis[1][2] = 0.0;

    // spacing = (unitIndexDir . unitIndexBasis)*coordBasis/indexBasisLength;
    spacing[0][0] =
      iBasis[0][0] * cBasis[0][0] / basisLength[0] + iBasis[1][0] * cBasis[1][0] / basisLength[1];
    spacing[0][1] =
      iBasis[0][0] * cBasis[0][1] / basisLength[0] + iBasis[1][0] * cBasis[1][1] / basisLength[1];
    spacing[0][2] = 0.0;

    spacing[1][0] =
      iBasis[0][1] * cBasis[0][0] / basisLength[0] + iBasis[1][1] * cBasis[1][0] / basisLength[1];
    spacing[1][1] =
      iBasis[0][1] * cBasis[0][1] / basisLength[0] + iBasis[1][1] * cBasis[1][1] / basisLength[1];
    spacing[1][2] = 0.0;

    // The samples are uniformly placed at sample interval depths
    // Dividing by 1000.0 to convert from microseconds to milliseconds.
    spacing[2][0] = 0.0;
    spacing[2][1] = 0.0;
    spacing[2][2] = this->SampleInterval / 1000.0;

    spacingSign[0] = spacing[0][0] >= 0.0 ? 1.0 : -1.0;
    spacingSign[1] = spacing[1][1] >= 0.0 ? 1.0 : -1.0;
    spacingSign[2] = (this->VerticalCRS == 0 ? -1 : 1); // goes

    origin[0] = (startCross - basisIndex[0][0]) * spacing[0][0] +
      (startInline - basisIndex[0][1]) * spacing[1][0] + basisCoords[0][0];
    origin[1] = (startCross - basisIndex[0][0]) * spacing[0][1] +
      (startInline - basisIndex[0][1]) * spacing[1][1] + basisCoords[0][1];
    origin[2] = -spacing[2][2] * (this->SampleCountPerTrace - 1);
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkSegYReaderInternal::ExportData(
  vtkImageData* imageData, int* extent, double origin[3], double spacing[3][3], int* spacingSign)
{
  imageData->SetExtent(extent);
  imageData->SetOrigin(origin);
  imageData->SetSpacing(
    vtkMath::Norm(spacing[0]), vtkMath::Norm(spacing[1]), vtkMath::Norm(spacing[2]));
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
        scalars->SetValue(id++, trace ? trace->Data[destK] : 0.0);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSegYReaderInternal::ExportData(
  vtkStructuredGrid* grid, int* extent, double origin[3], double spacing[3][3])
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
        double x = origin[0] + i * spacing[0][0] + j * spacing[1][0];
        double y = origin[1] + i * spacing[0][1] + j * spacing[1][1];
        double z = sign * k * spacing[2][2];
        if (trace)
        {
          double coordinateMultiplier = decodeMultiplier(trace->CoordinateMultiplier);
          x = coordinateMultiplier * trace->XCoordinate;
          y = coordinateMultiplier * trace->YCoordinate;
          z = sign * k * (trace->SampleInterval / 1000.0);

          scalars->InsertValue(id++, trace->Data[k]);
        }
        else
        {
          scalars->InsertValue(id++, 0.0);
        }
        points->InsertNextPoint(x, y, z);
      }
    }
  }

  grid->SetPoints(points);
  grid->GetPointData()->SetScalars(scalars);
}
