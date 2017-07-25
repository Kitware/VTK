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

#include "SegyReader.h"
#include <assert.h>

#include <set>
#include <vtkArrayData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolygon.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>
#include <vtkStructuredPointsReader.h>

SegyReader::~SegyReader()
{
  for (auto trace : traces)
    delete trace;
}

bool SegyReader::LoadFromFile(string path)
{
  ifstream in(path, ifstream::binary);
  if (!in)
  {
    cout << "File not found:" << path << endl;
    return false;
  }

  readHeader(in);

  int traceStartPos = 3600; // traces start after 3200 + 400 file header
  while (true)
  {
    Trace* pTrace = new Trace();
    if (!traceReader.readTrace(traceStartPos, in, formatCode, pTrace))
    {
      delete pTrace;
      break;
    }
    traces.push_back(pTrace);
  }

  in.close();
  return true;
}

bool SegyReader::readHeader(ifstream& in)
{
  short sampleInterval = IOUtil::Instance()->readShortInteger(
    binaryHeaderBytesPos.SampleInterval, in);
  formatCode =
    IOUtil::Instance()->readShortInteger(binaryHeaderBytesPos.FormatCode, in);
  in.seekg(binaryHeaderBytesPos.MajorVersion, in.beg);
  unsigned char majorVersion = IOUtil::Instance()->readUChar(in);
  unsigned char minorVersion = IOUtil::Instance()->readUChar(in);
  sampleCountPerTrace = IOUtil::Instance()->readShortInteger(
    binaryHeaderBytesPos.NumSamplesPerTrace, in);
  short tracesPerEnsemble = IOUtil::Instance()->readShortInteger(
    binaryHeaderBytesPos.NumberTracesPerEnsemble, in);
  short ensembleType =
    IOUtil::Instance()->readShortInteger(binaryHeaderBytesPos.EnsembleType, in);
  short measurementSystem = IOUtil::Instance()->readShortInteger(
    binaryHeaderBytesPos.MeasurementSystem, in);
  int byteOrderingDetection = IOUtil::Instance()->readLongInteger(
    binaryHeaderBytesPos.ByteOrderingDetection, in);

  std::cout << "Segy version = " << int(majorVersion) << "."
            << int(minorVersion) << std::endl;
  std::cout << "FormatCode = " << formatCode << std::endl;
  std::cout << "ByteOrderingDetection = " << byteOrderingDetection << std::endl;
  std::cout << "SampleCountPerTrace=" << sampleCountPerTrace << std::endl;
  std::cout << "ensembleType=" << ensembleType << std::endl;
  std::cout << "measurementSystem=" << measurementSystem << std::endl;
  std::cout << "sampleInterval=" << sampleInterval << std::endl;
  std::cout << "tracesPerEnsemble=" << tracesPerEnsemble << std::endl
            << std::endl;
  return true;
}

#define PIXEL_TYPE float

bool SegyReader::ExportData3D(vtkImageData* imageData)
{
  set<int> crosslineNumbers, inlineNumbers;
  for (auto trace : traces)
  {
    crosslineNumbers.insert(trace->crosslineNumber);
    inlineNumbers.insert(trace->inlineNumber);
  }

  if (crosslineNumbers.size() < 3 || inlineNumbers.size() < 3)
  {
    return false;
  }

  map<int, vector<Trace*>> cross_inline_map;

  float min_data = INT_MAX;
  float max_data = INT_MIN;

  for (auto trace : traces)
  {
    int cross = trace->crosslineNumber;
    auto pair = cross_inline_map.find(cross);
    if (pair == cross_inline_map.end())
    {
      cross_inline_map.insert(make_pair(cross, vector<Trace*>()));
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

  imageData->SetDimensions(inlineCount, crossLineCount, sampleCountPerTrace);

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
      for (int k = 0; k < sampleCountPerTrace; k++)
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

bool SegyReader::GetImageData(vtkImageData* imageData)
{
  int crosslineNum;
  int minCrossLineNumber = INT_MAX;
  int maxCrossLineNumber = INT_MIN;

  for (auto trace : traces)
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
  imageData->SetDimensions(sampleCountPerTrace, crosslineNumberCount, 1);
  imageData->SetScalarType(type, imageData->GetInformation());
  imageData->SetNumberOfScalarComponents(1, imageData->GetInformation());
  imageData->AllocateScalars(type, 1);

  float min_data = INT_MAX;
  float max_data = INT_MIN;

  for (auto trace : traces)
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

  for (int k = 0; k < sampleCountPerTrace; k++)
  {
    for (int i = 0; i < crosslineNumberCount; i++)
    {
      int aggIndex = i * sampleCountPerTrace + k;

      index = 0;
      remainder = 0;

      for (auto trace : traces)
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

      *(ptr + i * sampleCountPerTrace + k) = 256.0 *
        (traces[index]->data[remainder] - min_data) / (max_data - min_data);
    }
  }

  return true;
}

void SegyReader::AddScalars(vtkStructuredGrid* grid)
{

  vtkSmartPointer<vtkFloatArray> pointData =
    vtkSmartPointer<vtkFloatArray>::New();
  pointData->SetName("trace");
  pointData->SetNumberOfComponents(1);

  int crossLineCount = traces.size();

  pointData->Allocate(crossLineCount * sampleCountPerTrace);

  int j = 0;
  for (int k = 0; k < sampleCountPerTrace; k++)
  {
    for (int i = 0; i < traces.size(); i++)
    {
      pointData->InsertValue(j++, traces[i]->data[k]);
    }
  }

  grid->GetPointData()->SetScalars(pointData);
  grid->GetPointData()->SetActiveScalars("trace");
}

void SegyReader::ExportData2D(vtkStructuredGrid* grid)
{
  grid->SetDimensions(traces.size(), sampleCountPerTrace, 1);
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

  for (int k = 0; k < sampleCountPerTrace; k++)
  {
    for (int i = 0; i < traces.size(); i++)
    {
      auto trace = traces[i];
      float coordinateMultiplier = (trace->CoordinateMultiplier < 0)
        ? 1.0 / (-trace->CoordinateMultiplier)
        : trace->CoordinateMultiplier;
      float x = trace->xCoordinate * coordinateMultiplier;
      float y = trace->yCoordinate * coordinateMultiplier;

      float z = k * trace->SampleInterval / (sampleCountPerTrace - 1);
      points->InsertNextPoint(x, y, z);
    }
  }

  grid->SetPoints(points);
  this->AddScalars(grid);
}
