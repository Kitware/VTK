/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSegYReader.h"

#include "vtkSegYBinaryHeaderBytesPositions.h"
#include "vtkSegYIOUtils.h"
#include "vtkSegYTraceReader.h"

#include "vtkArrayData.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"

#include <iostream>
#include <map>
#include <set>

//-----------------------------------------------------------------------------
vtkSegYReader::vtkSegYReader()
{
  this->BinaryHeaderBytesPos = new vtkSegYBinaryHeaderBytesPositions();
  this->TraceReader = new vtkSegYTraceReader();
  this->VerticalCRS = 0;
}

//-----------------------------------------------------------------------------
vtkSegYReader::~vtkSegYReader()
{
  delete this->BinaryHeaderBytesPos;
  delete this->TraceReader;
  for (auto trace : this->Traces)
    delete trace;
}

//-----------------------------------------------------------------------------
void vtkSegYReader::SetXYCoordBytePositions(int x, int y)
{
  this->TraceReader->SetXYCoordBytePositions(x, y);
}

//-----------------------------------------------------------------------------
void vtkSegYReader::SetVerticalCRS(int v)
{
  this->VerticalCRS = v > 0 ? 1 : 0;
}

//-----------------------------------------------------------------------------
bool vtkSegYReader::LoadFromFile(std::string path)
{
  std::ifstream in(path, std::ifstream::binary);
  if (!in)
  {
    std::cerr << "File not found:" << path << std::endl;
    return false;
  }

  ReadHeader(in);

  int traceStartPos = 3600; // this->Traces start after 3200 + 400 file header
  while (true)
  {
    vtkSegYTrace* pTrace = new vtkSegYTrace();
    if (!this->TraceReader->ReadTrace(
          traceStartPos, in, this->FormatCode, pTrace))
    {
      delete pTrace;
      break;
    }
    this->Traces.push_back(pTrace);
  }

  in.close();
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegYReader::ReadHeader(std::ifstream& in)
{
  short sampleInterval = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->SampleInterval, in);
  this->FormatCode = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->FormatCode, in);
  in.seekg(this->BinaryHeaderBytesPos->MajorVersion, in.beg);
  unsigned char majorVersion = vtkSegYIOUtils::Instance()->readUChar(in);
  unsigned char minorVersion = vtkSegYIOUtils::Instance()->readUChar(in);
  this->SampleCountPerTrace = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->NumSamplesPerTrace, in);
  short tracesPerEnsemble = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->NumberTracesPerEnsemble, in);
  short ensembleType = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->EnsembleType, in);
  short measurementSystem = vtkSegYIOUtils::Instance()->readShortInteger(
    this->BinaryHeaderBytesPos->MeasurementSystem, in);
  int byteOrderingDetection = vtkSegYIOUtils::Instance()->readLongInteger(
    this->BinaryHeaderBytesPos->ByteOrderingDetection, in);

  std::cout << "Segy version = " << int(majorVersion) << "."
            << int(minorVersion) << std::endl;
  std::cout << "FormatCode = " << this->FormatCode << std::endl;
  std::cout << "ByteOrderingDetection = " << byteOrderingDetection << std::endl;
  std::cout << "this->SampleCountPerTrace=" << this->SampleCountPerTrace
            << std::endl;
  std::cout << "ensembleType=" << ensembleType << std::endl;
  std::cout << "measurementSystem=" << measurementSystem << std::endl;
  std::cout << "sampleInterval=" << sampleInterval << std::endl;
  std::cout << "tracesPerEnsemble=" << tracesPerEnsemble << std::endl
            << std::endl;
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegYReader::ExportData3D(vtkImageData* imageData)
{
  std::set<int> crosslineNumbers, inlineNumbers;
  for (auto trace : this->Traces)
  {
    crosslineNumbers.insert(trace->crosslineNumber);
    inlineNumbers.insert(trace->inlineNumber);
  }

  if (crosslineNumbers.size() < 3 || inlineNumbers.size() < 3)
  {
    return false;
  }

  std::map<int, std::vector<vtkSegYTrace*>> cross_inline_map;

  float min_data = INT_MAX;
  float max_data = INT_MIN;

  for (auto trace : this->Traces)
  {
    int cross = trace->crosslineNumber;
    auto pair = cross_inline_map.find(cross);
    if (pair == cross_inline_map.end())
    {
      cross_inline_map.insert(make_pair(cross, std::vector<vtkSegYTrace*>()));
    }
    pair = cross_inline_map.find(cross);
    pair->second.push_back(trace);

    for (auto m : trace->data)
    {
      if (m < min_data)
        min_data = m;
      if (m > max_data)
        max_data = m;
    }
  }

  int crossLineCount = cross_inline_map.size();

  int inlineCount = INT_MAX;
  for (auto pair : cross_inline_map)
  {
    int count = pair.second.size();
    if (count < 3)
      return false;

    if (count < inlineCount)
      inlineCount = count;
  }

  imageData->SetDimensions(
    inlineCount, crossLineCount, this->SampleCountPerTrace);

  int type = VTK_FLOAT;
  imageData->SetScalarType(type, imageData->GetInformation());
  imageData->SetNumberOfScalarComponents(1, imageData->GetInformation());
  imageData->AllocateScalars(type, 1);
  float* ptr = (float*)imageData->GetScalarPointer();

  int i = 0;
  for (auto crossIter = cross_inline_map.begin();
       crossIter != cross_inline_map.end();
       crossIter++)
  {
    for (int j = 0; j < inlineCount; j++)
    {
      for (int k = 0; k < this->SampleCountPerTrace; k++)
      {
        float normalizedData = (crossIter->second[j]->data[k] - min_data) *
          255.0 / (max_data - min_data);

        *(ptr + k * crossLineCount * inlineCount + i * inlineCount + j) =
          normalizedData;
      }
    }
    i++;
  }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegYReader::GetImageData(vtkImageData* imageData)
{
  int crosslineNum;
  int minCrossLineNumber = INT_MAX;
  int maxCrossLineNumber = INT_MIN;

  for (auto trace : this->Traces)
  {
    crosslineNum = trace->crosslineNumber;
    if (crosslineNum == 0)
      break;
    minCrossLineNumber =
      minCrossLineNumber < crosslineNum ? minCrossLineNumber : crosslineNum;
    maxCrossLineNumber =
      maxCrossLineNumber > crosslineNum ? maxCrossLineNumber : crosslineNum;
  }

  int crossLineNumberStep = 1;
  int crosslineNumberCount =
    (maxCrossLineNumber - minCrossLineNumber) / crossLineNumberStep + 1;

  int type = VTK_FLOAT;
  imageData->SetDimensions(this->SampleCountPerTrace, crosslineNumberCount, 1);
  imageData->SetScalarType(type, imageData->GetInformation());
  imageData->SetNumberOfScalarComponents(1, imageData->GetInformation());
  imageData->AllocateScalars(type, 1);

  float min_data = INT_MAX;
  float max_data = INT_MIN;

  for (auto trace : this->Traces)
  {
    for (auto m : trace->data)
    {
      if (m < min_data)
        min_data = m;
      if (m > max_data)
        max_data = m;
    }
  }

  float* ptr = (float*)imageData->GetScalarPointer();

  int index = 0;
  int diff;
  int remainder = 0;
  int dataSize = 0;

  for (int k = 0; k < this->SampleCountPerTrace; k++)
  {
    for (int i = 0; i < crosslineNumberCount; i++)
    {
      int aggIndex = i * this->SampleCountPerTrace + k;

      index = 0;
      remainder = 0;

      for (auto trace : this->Traces)
      {
        dataSize = trace->data.size();
        diff = aggIndex - dataSize;

        if (diff > 0)
        {
          ++index;
          aggIndex = diff;
        }
        else
        {
          remainder = aggIndex % dataSize;
          break;
        }
      }

      *(ptr + i * this->SampleCountPerTrace + k) = 256.0 *
        (this->Traces[index]->data[remainder] - min_data) /
        (max_data - min_data);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkSegYReader::AddScalars(vtkStructuredGrid* grid)
{
  vtkSmartPointer<vtkFloatArray> outScalars =
    vtkSmartPointer<vtkFloatArray>::New();
  outScalars->SetName("trace");
  outScalars->SetNumberOfComponents(1);

  int crossLineCount = this->Traces.size();

  outScalars->Allocate(crossLineCount * this->SampleCountPerTrace);

  int j = 0;
  for (int k = 0; k < this->SampleCountPerTrace; k++)
  {
    for (unsigned int i = 0; i < this->Traces.size(); i++)
    {
      outScalars->InsertValue(j++, this->Traces[i]->data[k]);
    }
  }

  grid->GetPointData()->SetScalars(outScalars);
  grid->GetPointData()->SetActiveScalars("trace");
}

//-----------------------------------------------------------------------------
void vtkSegYReader::ExportData2D(vtkStructuredGrid* grid)
{
  if (!grid)
  {
    return;
  }
  grid->SetDimensions(this->Traces.size(), this->SampleCountPerTrace, 1);
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  for (int k = 0; k < this->SampleCountPerTrace; k++)
  {
    for (unsigned int i = 0; i < this->Traces.size(); i++)
    {
      auto trace = this->Traces[i];
      float coordinateMultiplier = (trace->CoordinateMultiplier < 0)
        ? 1.0 / (-trace->CoordinateMultiplier)
        : trace->CoordinateMultiplier;
      float x = trace->xCoordinate * coordinateMultiplier;
      float y = trace->yCoordinate * coordinateMultiplier;

      // The samples are uniformly placed at sample interval depths
      // Dividing by 1000.0 to convert from microseconds to milliseconds.
      int sign = this->VerticalCRS == 0 ? -1 : 1;
      float z = sign * k * (trace->SampleInterval / 1000.0);
      points->InsertNextPoint(x, y, z);
    }
  }

  grid->SetPoints(points);
  this->AddScalars(grid);
}
