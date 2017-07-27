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
}

//-----------------------------------------------------------------------------
vtkSegYReader::~vtkSegYReader()
{
  delete this->BinaryHeaderBytesPos;
  delete this->TraceReader;
  for (auto trace : Traces)
    delete trace;
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

  int traceStartPos = 3600; // Traces start after 3200 + 400 file header
  while (true)
  {
    vtkSegYTrace* pTrace = new vtkSegYTrace();
    if (!TraceReader->ReadTrace(traceStartPos, in, FormatCode, pTrace))
    {
      delete pTrace;
      break;
    }
    Traces.push_back(pTrace);
  }

  in.close();
  return true;
}

//-----------------------------------------------------------------------------
bool vtkSegYReader::ReadHeader(std::ifstream& in)
{
  short sampleInterval = vtkSegYIOUtils::Instance()->readShortInteger(
    BinaryHeaderBytesPos->SampleInterval, in);
  FormatCode = vtkSegYIOUtils::Instance()->readShortInteger(
    BinaryHeaderBytesPos->FormatCode, in);
  in.seekg(BinaryHeaderBytesPos->MajorVersion, in.beg);
  unsigned char majorVersion = vtkSegYIOUtils::Instance()->readUChar(in);
  unsigned char minorVersion = vtkSegYIOUtils::Instance()->readUChar(in);
  SampleCountPerTrace = vtkSegYIOUtils::Instance()->readShortInteger(
    BinaryHeaderBytesPos->NumSamplesPerTrace, in);
  short tracesPerEnsemble = vtkSegYIOUtils::Instance()->readShortInteger(
    BinaryHeaderBytesPos->NumberTracesPerEnsemble, in);
  short ensembleType = vtkSegYIOUtils::Instance()->readShortInteger(
    BinaryHeaderBytesPos->EnsembleType, in);
  short measurementSystem = vtkSegYIOUtils::Instance()->readShortInteger(
    BinaryHeaderBytesPos->MeasurementSystem, in);
  int byteOrderingDetection = vtkSegYIOUtils::Instance()->readLongInteger(
    BinaryHeaderBytesPos->ByteOrderingDetection, in);

  std::cout << "Segy version = " << int(majorVersion) << "."
            << int(minorVersion) << std::endl;
  std::cout << "FormatCode = " << FormatCode << std::endl;
  std::cout << "ByteOrderingDetection = " << byteOrderingDetection << std::endl;
  std::cout << "SampleCountPerTrace=" << SampleCountPerTrace << std::endl;
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
  for (auto trace : Traces)
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

  for (auto trace : Traces)
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

  imageData->SetDimensions(inlineCount, crossLineCount, SampleCountPerTrace);

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
      for (int k = 0; k < SampleCountPerTrace; k++)
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

  for (auto trace : Traces)
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
  imageData->SetDimensions(SampleCountPerTrace, crosslineNumberCount, 1);
  imageData->SetScalarType(type, imageData->GetInformation());
  imageData->SetNumberOfScalarComponents(1, imageData->GetInformation());
  imageData->AllocateScalars(type, 1);

  float min_data = INT_MAX;
  float max_data = INT_MIN;

  for (auto trace : Traces)
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

  for (int k = 0; k < SampleCountPerTrace; k++)
  {
    for (int i = 0; i < crosslineNumberCount; i++)
    {
      int aggIndex = i * SampleCountPerTrace + k;

      index = 0;
      remainder = 0;

      for (auto trace : Traces)
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

      *(ptr + i * SampleCountPerTrace + k) = 256.0 *
        (Traces[index]->data[remainder] - min_data) / (max_data - min_data);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkSegYReader::AddScalars(vtkStructuredGrid* grid)
{

  vtkSmartPointer<vtkFloatArray> pointData =
    vtkSmartPointer<vtkFloatArray>::New();
  pointData->SetName("trace");
  pointData->SetNumberOfComponents(1);

  int crossLineCount = Traces.size();

  pointData->Allocate(crossLineCount * SampleCountPerTrace);

  int j = 0;
  for (int k = 0; k < SampleCountPerTrace; k++)
  {
    for (unsigned int i = 0; i < Traces.size(); i++)
    {
      pointData->InsertValue(j++, Traces[i]->data[k]);
    }
  }

  grid->GetPointData()->SetScalars(pointData);
  grid->GetPointData()->SetActiveScalars("trace");
}

//-----------------------------------------------------------------------------
void vtkSegYReader::ExportData2D(vtkStructuredGrid* grid)
{
  grid->SetDimensions(Traces.size(), SampleCountPerTrace, 1);
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  for (int k = 0; k < SampleCountPerTrace; k++)
  {
    for (unsigned int i = 0; i < Traces.size(); i++)
    {
      auto trace = Traces[i];
      float coordinateMultiplier = (trace->CoordinateMultiplier < 0)
        ? 1.0 / (-trace->CoordinateMultiplier)
        : trace->CoordinateMultiplier;
      float x = trace->xCoordinate * coordinateMultiplier;
      float y = trace->yCoordinate * coordinateMultiplier;

      float z = k * trace->SampleInterval / (SampleCountPerTrace - 1);
      points->InsertNextPoint(x, y, z);
    }
  }

  grid->SetPoints(points);
  this->AddScalars(grid);
}
