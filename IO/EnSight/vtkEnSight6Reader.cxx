// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEnSight6Reader.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStringScanner.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtksys/FStream.hxx"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkEnSight6Reader);

//------------------------------------------------------------------------------
vtkEnSight6Reader::vtkEnSight6Reader()
{
  this->NumberOfUnstructuredPoints = 0;
  this->UnstructuredPoints = nullptr;
  this->UnstructuredNodeIds = nullptr;
}

//------------------------------------------------------------------------------
vtkEnSight6Reader::~vtkEnSight6Reader()
{
  this->CleanUpCache();
}

//------------------------------------------------------------------------------
static void vtkEnSight6ReaderRead4(
  const char* line, int& pointId, float& point1, float& point2, float& point3)
{
  const std::string_view lineView(line);
  pointId = vtk::scan_int<int>(lineView.substr(0, 8))->value();
  point1 = vtk::scan_value<float>(lineView.substr(8, 12))->value();
  point2 = vtk::scan_value<float>(lineView.substr(20, 12))->value();
  point3 = vtk::scan_value<float>(lineView.substr(32, 12))->value();
}

static void vtkEnSight6ReaderRead3(const char* line, float& point1, float& point2, float& point3)
{
  const std::string_view lineView(line);
  point1 = vtk::scan_value<float>(lineView.substr(0, 12))->value();
  point2 = vtk::scan_value<float>(lineView.substr(12, 12))->value();
  point3 = vtk::scan_value<float>(lineView.substr(24, 12))->value();
}

static void vtkEnSight6ReaderRead6(const char* line, float& point1, float& point2, float& point3,
  float& point4, float& point5, float& point6)
{
  const std::string_view lineView(line);
  point1 = vtk::scan_value<float>(lineView.substr(0, 12))->value();
  point2 = vtk::scan_value<float>(lineView.substr(12, 12))->value();
  point3 = vtk::scan_value<float>(lineView.substr(24, 12))->value();
  point4 = vtk::scan_value<float>(lineView.substr(36, 12))->value();
  point5 = vtk::scan_value<float>(lineView.substr(48, 12))->value();
  point6 = vtk::scan_value<float>(lineView.substr(60, 12))->value();
}

static void vtkEnSight6ReaderRead1(const char* line, float& point1)
{
  point1 = vtk::scan_value<float>(std::string_view(line))->value();
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::ReadGeometryFile(
  const char* fileName, int timeStep, vtkMultiBlockDataSet* output)
{
  char line[256];
  int partId;
  int lineRead;
  int pointId;
  float point[3];
  int i, j;
  int pointIdsListed;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("A GeometryFileName must be specified in the case file.");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to geometry file: " << sfilename);
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  this->ReadLine(line);

  auto resultSubLine0 =
    vtk::scan<std::string_view, std::string_view>(std::string_view(line), "{:s} {:s}");
  if (resultSubLine0)
  {
    auto subLine = std::get<1>(resultSubLine0->values());
    if (subLine == "Binary")
    {
      vtkErrorMacro("This is a binary data set. Try vtkEnSight6BinaryReader.");
      return 0;
    }
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->RemoveLeadingBlanks(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
        this->RemoveLeadingBlanks(line);
      }
      this->ReadLine(line);
    }

    this->RemoveLeadingBlanks(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadNextDataLine(line);
      this->RemoveLeadingBlanks(line);
    }
    this->ReadLine(line);
  }

  // Skip description line.  Using ReadLine instead of
  // ReadNextDataLine because the description line could be blank.
  this->ReadLine(line);

  this->CleanUpCache();

  // Read the node id and element id lines.
  this->ReadLine(line);
  auto resultSubLine1 = vtk::scan<std::string_view, std::string_view, std::string_view>(
    std::string_view(line), "{:s} {:s} {:s}");
  auto subLine = std::get<2>(resultSubLine1->values());
  if (subLine == "given")
  {
    this->UnstructuredNodeIds = vtkIdTypeArray::New();
    pointIdsListed = 1;
  }
  else if (subLine == "ignore")
  {
    pointIdsListed = 1;
  }
  else
  {
    pointIdsListed = 0;
  }

  this->ReadNextDataLine(line);

  this->ReadNextDataLine(line); // "coordinates"
  this->ReadNextDataLine(line);
  this->NumberOfUnstructuredPoints = vtk::scan_int<int>(std::string_view(line))->value();
  this->UnstructuredPoints = vtkPoints::New();
  this->UnstructuredPoints->Allocate(this->NumberOfUnstructuredPoints);

  int* tmpIds = new int[this->NumberOfUnstructuredPoints];

  int maxId = 0;

  for (j = 0; j < this->NumberOfUnstructuredPoints; j++)
  {
    this->ReadNextDataLine(line);
    if (pointIdsListed)
    {
      // point ids listed
      vtkEnSight6ReaderRead4(line, pointId, point[0], point[1], point[2]);
      if (this->UnstructuredNodeIds)
      {
        tmpIds[j] = pointId;
        maxId = std::max(pointId, maxId);
      }
      this->UnstructuredPoints->InsertNextPoint(point);
    }
    else
    {
      vtkEnSight6ReaderRead3(line, point[0], point[1], point[2]);
      this->UnstructuredPoints->InsertNextPoint(point);
    }
  }

  if (this->UnstructuredNodeIds)
  {
    this->UnstructuredNodeIds->SetNumberOfComponents(1);
    this->UnstructuredNodeIds->SetNumberOfTuples(maxId);
    this->UnstructuredNodeIds->FillComponent(0, -1);

    for (j = 0; j < this->NumberOfUnstructuredPoints; j++)
    {
      this->UnstructuredNodeIds->InsertValue(tmpIds[j] - 1, j);
    }
  }
  delete[] tmpIds;

  lineRead = this->ReadNextDataLine(line); // "part"

  vtk::scan_result_type<std::string_view, int> resultPartId;
  while (lineRead && ((resultPartId = vtk::scan<int>(std::string_view(line), " part {:d}"))))
  {
    this->NumberOfGeometryParts++;
    partId = resultPartId->value() - 1; // EnSight starts #ing at 1.
    int realId = this->InsertNewPartId(partId);

    this->ReadLine(line); // part description line
    char* name = strdup(line);
    this->ReadNextDataLine(line);
    this->RemoveLeadingBlanks(line);

    if (strncmp(line, "block", 5) == 0)
    {
      lineRead = this->CreateStructuredGridOutput(realId, line, name, output);
    }
    else
    {
      lineRead = this->CreateUnstructuredGridOutput(realId, line, name, output);
    }
    free(name);
  }

  delete this->IS;
  this->IS = nullptr;
  if (this->UnstructuredNodeIds)
  {
    this->UnstructuredNodeIds->Delete();
    this->UnstructuredNodeIds = nullptr;
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::ReadMeasuredGeometryFile(
  const char* fileName, int timeStep, vtkMultiBlockDataSet* output)
{
  char line[256];
  vtkPoints* newPoints;
  int i;
  vtkIdType id;
  int tempId;
  float coords[3];

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("A MeasuredFileName must be specified in the case file.");
    return 0;
  }

  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to measured geometry file: " << sfilename);
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  this->ReadLine(line);

  auto resultSubLine =
    vtk::scan<std::string_view, std::string_view>(std::string_view(line), "{:s} {:s}");
  if (resultSubLine)
  {
    auto subLine = std::get<1>(resultSubLine->values());
    if (subLine == "Binary")
    {
      vtkErrorMacro("This is a binary data set. Try vtkEnSight6BinaryReader.");
      return 0;
    }
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->RemoveLeadingBlanks(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
        this->RemoveLeadingBlanks(line);
      }
      this->ReadLine(line);
    }

    this->RemoveLeadingBlanks(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
    }
    this->ReadLine(line);
  }

  this->ReadLine(line); // "particle coordinates"
  this->ReadLine(line);
  this->NumberOfMeasuredPoints = vtk::scan_int<int>(std::string_view(line))->value();

  this->NumberOfNewOutputs++;

  if (this->GetDataSetFromBlock(output, this->NumberOfGeometryParts) == nullptr ||
    !this->GetDataSetFromBlock(output, this->NumberOfGeometryParts)->IsA("vtkPolyData"))
  {
    vtkDebugMacro("creating new measured geometry output");
    vtkPolyData* pd = vtkPolyData::New();
    this->AddToBlock(output, this->NumberOfGeometryParts, pd);
    pd->Delete();
  }

  vtkPolyData* pd =
    vtkPolyData::SafeDownCast(this->GetDataSetFromBlock(output, this->NumberOfGeometryParts));
  pd->AllocateEstimate(this->NumberOfMeasuredPoints, 1);

  newPoints = vtkPoints::New();
  newPoints->Allocate(this->NumberOfMeasuredPoints);

  for (i = 0; i < this->NumberOfMeasuredPoints; i++)
  {
    this->ReadLine(line);
    vtkEnSight6ReaderRead4(line, tempId, coords[0], coords[1], coords[2]);
    id = this->ParticleCoordinatesByIndex ? i : tempId;
    newPoints->InsertNextPoint(coords);
    pd->InsertNextCell(VTK_VERTEX, 1, &id);
  }

  pd->SetPoints(newPoints);

  newPoints->Delete();

  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::ReadScalarsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int measured, int numberOfComponents,
  int component)
{
  char line[256];
  int partId, numPts, i, j;
  vtkFloatArray* scalars;
  int numLines, moreScalars;
  float scalarsRead[6];
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("nullptr ScalarPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to scalar per node file: " << sfilename);
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
        this->RemoveLeadingBlanks(line);
      }
    }

    this->ReadLine(line);
    this->RemoveLeadingBlanks(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
    }
  }

  this->ReadLine(line); // skip the description line

  this->ReadNextDataLine(line); // 1st data line or part #
  this->RemoveLeadingBlanks(line);
  if (strncmp(line, "part", 4) != 0)
  {
    int allocatedScalars = 0;
    // There are 6 values per line, and one scalar per point.
    if (!measured)
    {
      numPts = this->UnstructuredPoints->GetNumberOfPoints();
    }
    else
    {
      numPts = this->GetDataSetFromBlock(compositeOutput, this->NumberOfGeometryParts)
                 ->GetNumberOfPoints();
    }
    numLines = numPts / 6;
    moreScalars = numPts % 6;
    if (component == 0)
    {
      scalars = vtkFloatArray::New();
      scalars->SetNumberOfTuples(numPts);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numPts * numberOfComponents);
      allocatedScalars = 1;
    }
    else
    {
      // It does not matter which unstructured part we get the point data from
      // because it is the same for all of them.
      partId = this->UnstructuredPartIds->GetId(0);
      scalars = static_cast<vtkFloatArray*>(
        this->GetDataSetFromBlock(compositeOutput, partId)->GetPointData()->GetArray(description));
    }
    for (i = 0; i < numLines; i++)
    {
      vtkEnSight6ReaderRead6(line, scalarsRead[0], scalarsRead[1], scalarsRead[2], scalarsRead[3],
        scalarsRead[4], scalarsRead[5]);
      for (j = 0; j < 6; j++)
      {
        scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
      }
      this->ReadNextDataLine(line);
    }
    for (j = 0; j < moreScalars; j++)
    {
      vtkEnSight6ReaderRead1(line + j * 12, scalarsRead[j]);
      scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
    }
    if (moreScalars != 0)
    {
      this->ReadLine(line);
    }

    scalars->SetName(description);
    if (!measured)
    {
      for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
      {
        partId = this->UnstructuredPartIds->GetId(i);
        output = this->GetDataSetFromBlock(compositeOutput, partId);
        if (output)
        {
          if (component == 0)
          {
            output->GetPointData()->AddArray(scalars);
            if (!output->GetPointData()->GetScalars())
            {
              output->GetPointData()->SetScalars(scalars);
            }
          }
          else
          {
            output->GetPointData()->AddArray(scalars);
          }
        }
      }
    }
    else
    {
      output = this->GetDataSetFromBlock(compositeOutput, this->NumberOfGeometryParts);
      if (output)
      {
        output->GetPointData()->AddArray(scalars);
        if (!output->GetPointData()->GetScalars())
        {
          output->GetPointData()->SetScalars(scalars);
        }
      }
    }
    if (allocatedScalars)
    {
      scalars->Delete();
    }
  }

  this->RemoveLeadingBlanks(line);
  // scalars for structured parts
  while (strncmp(line, "part", 4) == 0)
  {
    int allocatedScalars = 0;
    partId = vtk::scan<int>(std::string_view(line), " part {:d}")->value() -
      1; // EnSight starts #ing at 1.
    int realId = this->InsertNewPartId(partId);

    output = this->GetDataSetFromBlock(compositeOutput, realId);
    if (output == nullptr)
    {
      vtkErrorMacro("Could not get output for part " << partId);
      vtkErrorMacro("Got part from line: " << line);
      return 0;
    }

    this->ReadNextDataLine(line); // block
    numPts = output->GetNumberOfPoints();
    numLines = numPts / 6;
    moreScalars = numPts % 6;
    if (component == 0)
    {
      scalars = vtkFloatArray::New();
      scalars->SetNumberOfTuples(numPts);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numPts * numberOfComponents);
      allocatedScalars = 1;
    }
    else
    {
      scalars = (vtkFloatArray*)(output->GetPointData()->GetArray(description));
    }
    for (i = 0; i < numLines; i++)
    {
      this->ReadNextDataLine(line);
      vtkEnSight6ReaderRead6(line, scalarsRead[0], scalarsRead[1], scalarsRead[2], scalarsRead[3],
        scalarsRead[4], scalarsRead[5]);
      for (j = 0; j < 6; j++)
      {
        scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
      }
    }
    this->ReadNextDataLine(line);
    for (j = 0; j < moreScalars; j++)
    {
      vtkEnSight6ReaderRead1(line + j * 12, scalarsRead[j]);
      scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
    }
    if (component == 0)
    {
      scalars->SetName(description);
      output->GetPointData()->AddArray(scalars);
      if (!output->GetPointData()->GetScalars())
      {
        output->GetPointData()->SetScalars(scalars);
      }
    }
    else
    {
      output->GetPointData()->AddArray(scalars);
    }
    this->ReadNextDataLine(line);
    if (allocatedScalars)
    {
      scalars->Delete();
    }
    this->RemoveLeadingBlanks(line);
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::ReadVectorsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int measured)
{
  char line[256];
  int partId, numPts, i, j, k;
  vtkFloatArray* vectors;
  int numLines, moreVectors;
  float vector1[3], vector2[3], values[6];
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("nullptr VectorPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to vector per node file: " << sfilename);
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
        this->RemoveLeadingBlanks(line);
      }
    }

    this->ReadLine(line);
    this->RemoveLeadingBlanks(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
    }
  }

  this->ReadLine(line); // skip the description line

  this->ReadNextDataLine(line); // 1st data line or part #
  this->RemoveLeadingBlanks(line);
  if (strncmp(line, "part", 4) != 0)
  {
    // There are 6 values per line, and 3 values (or 1 vector) per point.
    if (!measured)
    {
      numPts = this->UnstructuredPoints->GetNumberOfPoints();
    }
    else
    {
      numPts = this->GetDataSetFromBlock(compositeOutput, this->NumberOfGeometryParts)
                 ->GetNumberOfPoints();
    }
    numLines = numPts / 2;
    moreVectors = ((numPts * 3) % 6) / 3;
    vectors = vtkFloatArray::New();
    vectors->SetNumberOfTuples(numPts);
    vectors->SetNumberOfComponents(3);
    vectors->SetName(description);
    vectors->Allocate(numPts * 3);
    for (i = 0; i < numLines; i++)
    {
      vtkEnSight6ReaderRead6(
        line, vector1[0], vector1[1], vector1[2], vector2[0], vector2[1], vector2[2]);
      vectors->InsertTuple(i * 2, vector1);
      vectors->InsertTuple(i * 2 + 1, vector2);
      this->ReadNextDataLine(line);
    }
    for (j = 0; j < moreVectors; j++)
    {
      vtkEnSight6ReaderRead1(line + j * 36, vector1[0]);
      vtkEnSight6ReaderRead1(line + j * 36 + 12, vector1[1]);
      vtkEnSight6ReaderRead1(line + j * 36 + 24, vector1[2]);
      vectors->InsertTuple(i * 2 + j, vector1);
    }
    if (moreVectors != 0)
    {
      this->ReadLine(line);
    }

    if (!measured)
    {
      for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
      {
        partId = this->UnstructuredPartIds->GetId(i);
        output = this->GetDataSetFromBlock(compositeOutput, partId);
        if (output)
        {
          output->GetPointData()->AddArray(vectors);
          if (!output->GetPointData()->GetVectors())
          {
            output->GetPointData()->SetVectors(vectors);
          }
        }
      }
    }
    else
    {
      output = this->GetDataSetFromBlock(compositeOutput, this->NumberOfGeometryParts);
      output->GetPointData()->AddArray(vectors);
      if (!output->GetPointData()->GetVectors())
      {
        output->GetPointData()->SetVectors(vectors);
      }
    }
    vectors->Delete();
  }

  // vectors for structured parts
  this->RemoveLeadingBlanks(line);
  while (strncmp(line, "part", 4) == 0)
  {
    partId = vtk::scan<int>(std::string_view(line), " part {:d}")->value() -
      1; // EnSight starts #ing at 1.
    int realId = this->InsertNewPartId(partId);

    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = output->GetNumberOfPoints();
    numLines = numPts / 6;
    moreVectors = numPts % 6;
    vectors = vtkFloatArray::New();
    vectors->SetNumberOfTuples(numPts);
    vectors->SetNumberOfComponents(3);
    vectors->SetName(description);
    vectors->Allocate(numPts * 3);

    for (k = 0; k < 3; k++)
    {
      for (i = 0; i < numLines; i++)
      {
        this->ReadNextDataLine(line);
        vtkEnSight6ReaderRead6(
          line, values[0], values[1], values[2], values[3], values[4], values[5]);
        for (j = 0; j < 6; j++)
        {
          vectors->InsertComponent(i * 6 + j, k, values[j]);
        }
      }

      if (moreVectors)
      {
        this->ReadNextDataLine(line);
        for (j = 0; j < moreVectors; j++)
        {
          vtkEnSight6ReaderRead1(line + j * 12, values[j]);
          vectors->InsertComponent(i * 6 + j, k, values[j]);
        }
      }
    }
    output->GetPointData()->AddArray(vectors);
    if (!output->GetPointData()->GetVectors())
    {
      output->GetPointData()->SetVectors(vectors);
    }
    vectors->Delete();

    this->ReadNextDataLine(line);
    this->RemoveLeadingBlanks(line);
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::ReadAsymmetricTensorsPerNode(const char* vtkNotUsed(fileName),
  const char* vtkNotUsed(description), int vtkNotUsed(timeStep),
  vtkMultiBlockDataSet* vtkNotUsed(compositeOutput))
{
  vtkErrorMacro("Asymmetric Tensors are not supported by Ensight6 ASCII files");
  return 0;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::ReadTensorsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  char line[256];
  int partId, numPts, i, j, k;
  vtkFloatArray* tensors;
  int numLines, moreTensors;
  float tensor[6], values[6];
  int lineRead;
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("nullptr TensorSymmPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to tensor symm per node file: " << sfilename);
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
        this->RemoveLeadingBlanks(line);
      }
    }

    this->ReadLine(line);
    this->RemoveLeadingBlanks(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
      this->RemoveLeadingBlanks(line);
    }
  }

  this->ReadLine(line); // skip the description line

  lineRead = this->ReadNextDataLine(line); // 1st data line or part #
  this->RemoveLeadingBlanks(line);
  if (strncmp(line, "part", 4) != 0)
  {
    // There are 6 values per line, and 6 values (or 1 tensor) per point.
    numPts = this->UnstructuredPoints->GetNumberOfPoints();
    numLines = numPts;
    tensors = vtkFloatArray::New();
    tensors->SetNumberOfTuples(numPts);
    tensors->SetNumberOfComponents(6);
    tensors->SetName(description);
    tensors->Allocate(numPts * 6);
    for (i = 0; i < numLines; i++)
    {
      vtkEnSight6ReaderRead6(
        line, tensor[0], tensor[1], tensor[2], tensor[3], tensor[5], tensor[4]);
      tensors->InsertTuple(i, tensor);
      lineRead = this->ReadNextDataLine(line);
    }

    for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
    {
      partId = this->UnstructuredPartIds->GetId(i);
      output = this->GetDataSetFromBlock(compositeOutput, partId);
      if (output)
      {
        output->GetPointData()->AddArray(tensors);
      }
    }
    tensors->Delete();
  }

  // vectors for structured parts
  this->RemoveLeadingBlanks(line);
  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    assert(false);
    // code below does not make sens and is not tested
    partId = vtk::scan<int>(std::string_view(line), " part {:d}")->value() -
      1; // EnSight starts #ing at 1.
    int realId = this->InsertNewPartId(partId);
    this->ReadNextDataLine(line); // block
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = output->GetNumberOfPoints();
    numLines = numPts / 6;
    moreTensors = numPts % 6;
    tensors = vtkFloatArray::New();
    tensors->SetNumberOfTuples(numPts);
    tensors->SetNumberOfComponents(6);
    tensors->SetName(description);
    tensors->Allocate(numPts * 6);

    for (k = 0; k < 6; k++)
    {
      for (i = 0; i < numLines; i++)
      {
        this->ReadNextDataLine(line);
        vtkEnSight6ReaderRead6(
          line, values[0], values[1], values[2], values[3], values[5], values[4]);
        for (j = 0; j < 6; j++)
        {
          tensors->InsertComponent(i * 6 + j, k, values[j]);
        }
      }

      if (moreTensors)
      {
        this->ReadNextDataLine(line);
        for (j = 0; j < moreTensors; j++)
        {
          vtkEnSight6ReaderRead1(line + j * 12, values[j]);
          tensors->InsertComponent(i * 6 + j, k, values[j]);
        }
      }
    }
    output->GetPointData()->AddArray(tensors);
    tensors->Delete();
    lineRead = this->ReadNextDataLine(line);
    this->RemoveLeadingBlanks(line);
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::ReadScalarsPerElement(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int numberOfComponents, int component)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray* scalars;
  int lineRead, elementType;
  float scalarsRead[6];
  int numLines, moreScalars;
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("nullptr ScalarPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to scalars per element file: " << sfilename);
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line);                    // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    partId = vtk::scan<int>(std::string_view(line), " part {:d}")->value() -
      1; // EnSight starts #ing at 1.
    int realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = output->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    if (component == 0)
    {
      scalars = vtkFloatArray::New();
      scalars->SetNumberOfTuples(numCells);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numCells * numberOfComponents);
    }
    else
    {
      scalars = (vtkFloatArray*)(output->GetCellData()->GetArray(description));
    }

    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
    {
      numLines = numCells / 6;
      moreScalars = numCells % 6;
      for (i = 0; i < numLines; i++)
      {
        this->ReadNextDataLine(line);
        vtkEnSight6ReaderRead6(line, scalarsRead[0], scalarsRead[1], scalarsRead[2], scalarsRead[3],
          scalarsRead[4], scalarsRead[5]);
        for (j = 0; j < 6; j++)
        {
          scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
        }
      }
      lineRead = this->ReadNextDataLine(line);

      if (moreScalars)
      {
        for (j = 0; j < moreScalars; j++)
        {
          vtkEnSight6ReaderRead1(line + j * 12, scalarsRead[j]);
          scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
        }
      }
    }
    else
    {
      while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
      {
        elementType = this->GetElementType(line);
        if (elementType < 0)
        {
          vtkErrorMacro("invalid element type");
          delete this->IS;
          this->IS = nullptr;
          return 0;
        }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        numLines = numCellsPerElement / 6;
        moreScalars = numCellsPerElement % 6;
        for (i = 0; i < numLines; i++)
        {
          this->ReadNextDataLine(line);
          vtkEnSight6ReaderRead6(line, scalarsRead[0], scalarsRead[1], scalarsRead[2],
            scalarsRead[3], scalarsRead[4], scalarsRead[5]);
          for (j = 0; j < 6; j++)
          {
            scalars->InsertComponent(
              this->GetCellIds(idx, elementType)->GetId(i * 6 + j), component, scalarsRead[j]);
          }
        }
        if (moreScalars)
        {
          this->ReadNextDataLine(line);
          for (j = 0; j < moreScalars; j++)
          {
            vtkEnSight6ReaderRead1(line + j * 12, scalarsRead[j]);
            scalars->InsertComponent(
              this->GetCellIds(idx, elementType)->GetId(i * 6 + j), component, scalarsRead[j]);
          }
        }
        lineRead = this->ReadNextDataLine(line);
      } // end while
    }   // end else
    if (component == 0)
    {
      scalars->SetName(description);
      output->GetCellData()->AddArray(scalars);
      if (!output->GetCellData()->GetScalars())
      {
        output->GetCellData()->SetScalars(scalars);
      }
      scalars->Delete();
    }
    else
    {
      output->GetCellData()->AddArray(scalars);
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::ReadVectorsPerElement(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, j, k, idx;
  vtkFloatArray* vectors;
  int lineRead, elementType;
  float values[6], vector1[3], vector2[3];
  int numLines, moreVectors;
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("nullptr VectorPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to vector per element file: " << sfilename);
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line);                    // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    vectors = vtkFloatArray::New();
    partId = vtk::scan<int>(std::string_view(line), " part {:d}")->value() -
      1; // EnSight starts #ing at 1.
    int realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = output->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    vectors->SetNumberOfTuples(numCells);
    vectors->SetNumberOfComponents(3);
    vectors->SetName(description);
    vectors->Allocate(numCells * 3);

    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
    {
      numLines = numCells / 6;
      moreVectors = numCells % 6;

      for (k = 0; k < 3; k++)
      {
        for (i = 0; i < numLines; i++)
        {
          this->ReadNextDataLine(line);
          vtkEnSight6ReaderRead6(
            line, values[0], values[1], values[2], values[3], values[4], values[5]);
          for (j = 0; j < 6; j++)
          {
            vectors->InsertComponent(i * 6 + j, k, values[j]);
          }
        }
        if (moreVectors)
        {
          this->ReadNextDataLine(line);
          for (j = 0; j < moreVectors; j++)
          {
            vtkEnSight6ReaderRead1(line + j * 12, values[j]);
            vectors->InsertComponent(i * 6 + j, k, values[j]);
          }
        }
      }
      lineRead = this->ReadNextDataLine(line);
    }
    else
    {
      while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
      {
        elementType = this->GetElementType(line);
        if (elementType < 0)
        {
          vtkErrorMacro("invalid element type");
          delete this->IS;
          this->IS = nullptr;
          return 0;
        }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        numLines = numCellsPerElement / 2;
        moreVectors = ((numCellsPerElement * 3) % 6) / 3;

        for (i = 0; i < numLines; i++)
        {
          this->ReadNextDataLine(line);
          vtkEnSight6ReaderRead6(
            line, vector1[0], vector1[1], vector1[2], vector2[0], vector2[1], vector2[2]);

          vectors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(2 * i), vector1);
          vectors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(2 * i + 1), vector2);
        }
        if (moreVectors)
        {
          this->ReadNextDataLine(line);
          for (j = 0; j < moreVectors; j++)
          {
            vtkEnSight6ReaderRead1(line + j * 36, vector1[0]);
            vtkEnSight6ReaderRead1(line + j * 36 + 12, vector1[1]);
            vtkEnSight6ReaderRead1(line + j * 36 + 24, vector1[2]);
            vectors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(2 * i + j), vector1);
          }
        }
        lineRead = this->ReadNextDataLine(line);
      } // end while
    }   // end else
    output->GetCellData()->AddArray(vectors);
    if (!output->GetCellData()->GetVectors())
    {
      output->GetCellData()->SetVectors(vectors);
    }
    vectors->Delete();
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::ReadAsymmetricTensorsPerElement(const char* vtkNotUsed(fileName),
  const char* vtkNotUsed(description), int vtkNotUsed(timeStep),
  vtkMultiBlockDataSet* vtkNotUsed(compositeOutput))
{
  vtkErrorMacro("Asymmetric Tensors are not supported by Ensight6 ASCII files");
  return 0;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::ReadTensorsPerElement(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, j, k, idx;
  vtkFloatArray* tensors;
  int lineRead, elementType;
  float values[6], tensor[6];
  int numLines, moreTensors;
  vtkDataSet* output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("nullptr TensorPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to tensor per element file: " << sfilename);
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  if (this->UseFileSets)
  {
    for (i = 0; i < timeStep - 1; i++)
    {
      this->ReadLine(line);
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
    }

    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadLine(line);
    }
  }

  this->ReadLine(line);                    // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    tensors = vtkFloatArray::New();
    partId = vtk::scan<int>(std::string_view(line), " part {:d}")->value() -
      1; // EnSight starts #ing at 1.
    int realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = output->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    tensors->SetNumberOfTuples(numCells);
    tensors->SetNumberOfComponents(6);
    tensors->SetName(description);
    tensors->Allocate(numCells * 6);

    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
    {
      assert(false);
      // Code below does not make sense and is not tested

      numLines = numCells / 6;
      moreTensors = numCells % 6;

      for (k = 0; k < 6; k++)
      {
        for (i = 0; i < numLines; i++)
        {
          this->ReadNextDataLine(line);
          vtkEnSight6ReaderRead6(
            line, values[0], values[1], values[2], values[3], values[5], values[4]);
          for (j = 0; j < 6; j++)
          {
            tensors->InsertComponent(i * 6 + j, k, values[j]);
          }
        }
        if (moreTensors)
        {
          this->ReadNextDataLine(line);
          for (j = 0; j < moreTensors; j++)
          {
            vtkEnSight6ReaderRead1(line + j * 12, values[j]);
            tensors->InsertComponent(i * 6 + j, k, values[j]);
          }
        }
      }
      lineRead = this->ReadNextDataLine(line);
    }
    else
    {
      while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
      {
        elementType = this->GetElementType(line);
        if (elementType < 0)
        {
          vtkErrorMacro("invalid element type");
          delete this->IS;
          this->IS = nullptr;
          return 0;
        }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        numLines = numCellsPerElement;

        for (i = 0; i < numLines; i++)
        {
          this->ReadNextDataLine(line);
          vtkEnSight6ReaderRead6(
            line, tensor[0], tensor[1], tensor[2], tensor[3], tensor[5], tensor[4]);
          tensors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(i), tensor);
        }
        lineRead = this->ReadNextDataLine(line);
      } // end while
    }   // end else
    output->GetCellData()->AddArray(tensors);
    tensors->Delete();
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::CreateUnstructuredGridOutput(
  int partId, char line[256], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  int lineRead = 1;
  int i, j;
  int numElements;
  int idx, cellId, cellType, testId;

  this->NumberOfNewOutputs++;

  if (this->GetDataSetFromBlock(compositeOutput, partId) == nullptr ||
    !this->GetDataSetFromBlock(compositeOutput, partId)->IsA("vtkUnstructuredGrid"))
  {
    vtkDebugMacro("creating new unstructured output");
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::New();
    this->AddToBlock(compositeOutput, partId, ugrid);
    ugrid->Delete();

    this->UnstructuredPartIds->InsertNextId(partId);
  }

  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(this->GetDataSetFromBlock(compositeOutput, partId));

  this->SetBlockName(compositeOutput, partId, name);

  // Clear all cell ids from the last execution, if any.
  idx = this->UnstructuredPartIds->IsId(partId);
  for (i = 0; i < vtkEnSightReader::NUMBER_OF_ELEMENT_TYPES; i++)
  {
    this->GetCellIds(idx, i)->Reset();
  }

  output->Allocate(1000);

  while (lineRead && !vtk::scan<int>(std::string_view(line), " part {:d}"))
  {
    this->RemoveLeadingBlanks(line);
    if (strncmp(line, "point", 5) == 0)
    {
      vtkDebugMacro("point");

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();

      lineRead = this->ReadNextDataLine(line);

      vtkIdType nodeIds;
      for (i = 0; i < numElements; i++)
      {
        const std::string_view lineView(line);
        auto resultSubLine = vtk::scan<std::string_view, std::string_view>(lineView, "{:s} {:s}");
        if (resultSubLine)
        {
          auto subLine = std::get<1>(resultSubLine->values());
          // element ids listed
          // EnSight ids start at 1
          if (this->UnstructuredNodeIds)
          {
            nodeIds = this->UnstructuredNodeIds->GetValue(vtk::scan_int<int>(subLine)->value() - 1);
          }
          else
          {
            nodeIds = vtk::scan_int<int>(subLine)->value() - 1;
          }
        }
        else
        {
          if (this->UnstructuredNodeIds)
          {
            nodeIds =
              this->UnstructuredNodeIds->GetValue(vtk::scan_int<int>(lineView)->value() - 1);
          }
          else
          {
            nodeIds = vtk::scan_int<int>(lineView)->value() - 1;
          }
        }
        cellId = output->InsertNextCell(VTK_VERTEX, 1, &nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::POINT)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "bar2", 4) == 0)
    {
      vtkDebugMacro("bar2");

      vtkIdType nodeIds[2];
      int intIds[2];
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        auto resultInt3 = vtk::scan<int, int, int>(std::string_view(line), " {:d} {:d} {:d}");
        if (resultInt3)
        {
          std::tie(std::ignore, intIds[0], intIds[1]) = resultInt3->values();
        }
        else
        {
          auto resultInt2 = vtk::scan<int, int>(std::string_view(line), " {:d} {:d}");
          std::tie(intIds[0], intIds[1]) = resultInt2->values();
        }
        for (j = 0; j < 2; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 2; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 2; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_LINE, 2, nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::BAR2)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "bar3", 4) == 0)
    {
      vtkDebugMacro("bar3");
      vtkDebugMacro("Only vertex nodes of this element will be read.");
      int intIds[2];
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      lineRead = this->ReadNextDataLine(line);

      vtkIdType nodeIds[2];
      for (i = 0; i < numElements; i++)
      {
        std::string_view lineView(line);
        if (auto resultInt4 = vtk::scan<int, int, int, int>(lineView, " {:d} {:d} {:d} {:d}"))
        {
          std::tie(std::ignore, intIds[0], std::ignore, intIds[1]) = resultInt4->values();
        }
        else
        {
          auto resultInt3 = vtk::scan<int, int, int>(lineView, " {:d} {:d} {:d}");
          std::tie(intIds[0], std::ignore, intIds[1]) = resultInt3->values();
        }
        for (j = 0; j < 2; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 2; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 2; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_LINE, 2, nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::BAR3)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "tria3", 5) == 0 || strncmp(line, "tria6", 5) == 0)
    {
      if (strncmp(line, "tria6", 5) == 0)
      {
        vtkDebugMacro("tria6");
        vtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::TRIA6;
      }
      else
      {
        vtkDebugMacro("tria3");
        cellType = vtkEnSightReader::TRIA3;
      }

      vtkIdType nodeIds[3];
      int intIds[3];
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        const std::string_view lineView(line);
        auto resultInt4 = vtk::scan<int, int, int, int>(lineView, " {:d} {:d} {:d} {:d}");
        if (resultInt4 && cellType == vtkEnSightReader::TRIA3)
        {
          std::tie(std::ignore, intIds[0], intIds[1], intIds[2]) = resultInt4->values();
        }
        else
        {
          auto resultInt7 = vtk::scan<int, int, int, int, int, int, int>(
            lineView, " {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
          if (resultInt7 && cellType == vtkEnSightReader::TRIA6)
          {
            std::tie(std::ignore, intIds[0], intIds[1], intIds[2], std::ignore, std::ignore,
              testId) = resultInt7->values();
          }
          else
          {
            auto resultInt3 = vtk::scan<int, int, int>(lineView, " {:d} {:d} {:d}");
            std::tie(intIds[0], intIds[1], intIds[2]) = resultInt3->values();
          }
        }
        for (j = 0; j < 3; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 3; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 3; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_TRIANGLE, 3, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "quad4", 5) == 0 || strncmp(line, "quad8", 5) == 0)
    {
      if (strncmp(line, "quad8", 5) == 0)
      {
        vtkDebugMacro("quad8");
        vtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::QUAD8;
      }
      else
      {
        vtkDebugMacro("quad4");
        cellType = vtkEnSightReader::QUAD4;
      }

      vtkIdType nodeIds[4];
      int intIds[4];
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        const std::string_view lineView(line);
        auto resultInt5 = vtk::scan<int, int, int, int, int>(lineView, " {:d} {:d} {:d} {:d} {:d}");
        if (resultInt5 && cellType == vtkEnSightReader::QUAD4)
        {
          std::tie(std::ignore, intIds[0], intIds[1], intIds[2], intIds[3]) = resultInt5->values();
        }
        else
        {
          auto resultInt9 = vtk::scan<int, int, int, int, int, int, int, int, int>(
            lineView, " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
          if (resultInt9 && cellType == vtkEnSightReader::QUAD8)
          {
            std::tie(std::ignore, intIds[0], intIds[1], intIds[2], intIds[3], std::ignore,
              std::ignore, std::ignore, testId) = resultInt9->values();
          }
          else
          {
            auto resultInt4 = vtk::scan<int, int, int, int>(lineView, " {:d} {:d} {:d} {:d}");
            std::tie(intIds[0], intIds[1], intIds[2], intIds[3]) = resultInt4->values();
          }
        }
        for (j = 0; j < 4; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 4; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 4; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUAD, 4, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "tetra4", 6) == 0 || strncmp(line, "tetra10", 7) == 0)
    {
      if (strncmp(line, "tetra10", 7) == 0)
      {
        vtkDebugMacro("tetra10");
        vtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::TETRA10;
      }
      else
      {
        vtkDebugMacro("tetra4");
        cellType = vtkEnSightReader::TETRA4;
      }

      vtkIdType nodeIds[4];
      int intIds[4];
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        const std::string_view lineView(line);
        auto resultInt5 = vtk::scan<int, int, int, int, int>(lineView, " {:d} {:d} {:d} {:d} {:d}");
        if (resultInt5 && cellType == vtkEnSightReader::TETRA4)
        {
          std::tie(std::ignore, intIds[0], intIds[1], intIds[2], intIds[3]) = resultInt5->values();
        }
        else
        {
          auto resultInt11 = vtk::scan<int, int, int, int, int, int, int, int, int, int, int>(
            lineView, " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
          if (resultInt11 && cellType == vtkEnSightReader::TETRA10)
          {
            std::tie(std::ignore, intIds[0], intIds[1], intIds[2], intIds[3], std::ignore,
              std::ignore, std::ignore, std::ignore, std::ignore, testId) = resultInt11->values();
          }
          else
          {
            auto resultInt4 = vtk::scan<int, int, int, int>(lineView, " {:d} {:d} {:d} {:d}");
            std::tie(intIds[0], intIds[1], intIds[2], intIds[3]) = resultInt4->values();
          }
        }
        for (j = 0; j < 4; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 4; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 4; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_TETRA, 4, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "pyramid5", 8) == 0 || strncmp(line, "pyramid13", 9) == 0)
    {
      if (strncmp(line, "pyramid13", 9) == 0)
      {
        vtkDebugMacro("pyramid13");
        vtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::PYRAMID13;
      }
      else
      {
        vtkDebugMacro("pyramid5");
        cellType = vtkEnSightReader::PYRAMID5;
      }

      vtkIdType nodeIds[5];
      int intIds[5];
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        const std::string_view lineView(line);
        auto resultInt6 =
          vtk::scan<int, int, int, int, int, int>(lineView, " {:d} {:d} {:d} {:d} {:d} {:d}");
        if (resultInt6 && cellType == vtkEnSightReader::PYRAMID5)
        {
          std::tie(std::ignore, intIds[0], intIds[1], intIds[2], intIds[3], intIds[4]) =
            resultInt6->values();
        }
        else
        {
          auto resultInt14 =
            vtk::scan<int, int, int, int, int, int, int, int, int, int, int, int, int, int>(
              lineView, " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
          if (resultInt14 && cellType == vtkEnSightReader::PYRAMID13)
          {
            std::tie(std::ignore, intIds[0], intIds[1], intIds[2], intIds[3], intIds[4],
              std::ignore, std::ignore, std::ignore, std::ignore, std::ignore, std::ignore,
              std::ignore, testId) = resultInt14->values();
          }
          else
          {
            auto resultInt5 =
              vtk::scan<int, int, int, int, int>(lineView, " {:d} {:d} {:d} {:d} {:d}");
            std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4]) = resultInt5->values();
          }
        }
        for (j = 0; j < 5; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 5; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 5; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_PYRAMID, 5, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "hexa8", 5) == 0 || strncmp(line, "hexa20", 6) == 0)
    {
      if (strncmp(line, "hexa20", 6) == 0)
      {
        vtkDebugMacro("hexa20");
        vtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::HEXA20;
      }
      else
      {
        vtkDebugMacro("hexa8");
        cellType = vtkEnSightReader::HEXA8;
      }

      vtkIdType nodeIds[8];
      int intIds[8];
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      lineRead = this->ReadNextDataLine(line);

      for (i = 0; i < numElements; i++)
      {
        const std::string_view lineView(line);
        auto resultInt9 = vtk::scan<int, int, int, int, int, int, int, int, int>(
          lineView, " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
        if (resultInt9 && cellType == vtkEnSightReader::HEXA8)
        {
          std::tie(std::ignore, intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5],
            intIds[6], intIds[7]) = resultInt9->values();
        }
        else
        {
          auto resultInt21 = vtk::scan<int, int, int, int, int, int, int, int, int, int, int, int,
            int, int, int, int, int, int, int, int, int>(lineView,
            " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} "
            "{:d} {:d} {:d} {:d}");
          if (resultInt21 && cellType == vtkEnSightReader::HEXA20)
          {
            std::tie(std::ignore, intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5],
              intIds[6], intIds[7], std::ignore, std::ignore, std::ignore, std::ignore, std::ignore,
              std::ignore, std::ignore, std::ignore, std::ignore, std::ignore, std::ignore,
              testId) = resultInt21->values();
          }
          else
          {
            auto resultInt8 = vtk::scan<int, int, int, int, int, int, int, int>(
              lineView, " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
            std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5], intIds[6],
              intIds[7]) = resultInt8->values();
          }
        }
        for (j = 0; j < 8; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 8; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 8; j++)
        {
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_HEXAHEDRON, 8, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "penta6", 6) == 0 || strncmp(line, "penta15", 7) == 0)
    {
      if (strncmp(line, "penta15", 7) == 0)
      {
        vtkDebugMacro("penta15");
        vtkDebugMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::PENTA15;
      }
      else
      {
        vtkDebugMacro("penta6");
        cellType = vtkEnSightReader::PENTA6;
      }

      vtkIdType nodeIds[6];
      int intIds[6];
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      lineRead = this->ReadNextDataLine(line);

      constexpr unsigned char penta6Map[6] = { 0, 2, 1, 3, 5, 4 };
      for (i = 0; i < numElements; i++)
      {
        const std::string_view lineView(line);
        auto resultInt7 = vtk::scan<int, int, int, int, int, int, int>(
          lineView, " {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
        if (resultInt7 && cellType == vtkEnSightReader::PENTA6)
        {
          std::tie(std::ignore, intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5]) =
            resultInt7->values();
        }
        else
        {
          auto resultInt16 = vtk::scan<int, int, int, int, int, int, int, int, int, int, int, int,
            int, int, int, int>(lineView,
            " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
          if (resultInt16 && cellType == vtkEnSightReader::PENTA15)
          {
            std::tie(std::ignore, intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5],
              std::ignore, std::ignore, std::ignore, std::ignore, std::ignore, std::ignore,
              std::ignore, std::ignore, testId) = resultInt16->values();
          }
          else
          {
            auto resultInt6 =
              vtk::scan<int, int, int, int, int, int>(lineView, " {:d} {:d} {:d} {:d} {:d} {:d}");
            std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5]) =
              resultInt6->values();
          }
        }
        for (j = 0; j < 6; j++)
        {
          intIds[j]--;
        }
        if (this->UnstructuredNodeIds)
        {
          for (j = 0; j < 6; j++)
          {
            intIds[j] = this->UnstructuredNodeIds->GetValue(intIds[j]);
          }
        }
        for (j = 0; j < 6; j++)
        {
          nodeIds[penta6Map[j]] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_WEDGE, 6, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "END TIME STEP", 13) == 0)
    {
      break;
    }
  }

  output->SetPoints(this->UnstructuredPoints);

  return lineRead;
}

//------------------------------------------------------------------------------
int vtkEnSight6Reader::CreateStructuredGridOutput(
  int partId, char line[256], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int i, j;
  vtkPoints* points = vtkPoints::New();
  double point[3];
  int numPts, numLines, moreCoords, moreBlanking;
  float coords[6];
  int iblanks[10];

  this->NumberOfNewOutputs++;

  if (this->GetDataSetFromBlock(compositeOutput, partId) == nullptr ||
    !this->GetDataSetFromBlock(compositeOutput, partId)->IsA("vtkStructuredGrid"))
  {
    vtkDebugMacro("creating new structured grid output");
    vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
    this->AddToBlock(compositeOutput, partId, sgrid);
    sgrid->Delete();
  }

  vtkStructuredGrid* output =
    vtkStructuredGrid::SafeDownCast(this->GetDataSetFromBlock(compositeOutput, partId));
  this->SetBlockName(compositeOutput, partId, name);

  auto resultSubLine =
    vtk::scan<std::string_view, std::string_view>(std::string_view(line), "{:s} {:s}");
  if (resultSubLine)
  {
    auto subLine = std::get<1>(resultSubLine->values());
    if (subLine == "iblanked")
    {
      iblanked = 1;
    }
  }

  this->ReadNextDataLine(line);
  auto resultDimensions = vtk::scan<int, int, int>(std::string_view(line), " {:d} {:d} {:d}");
  std::tie(dimensions[0], dimensions[1], dimensions[2]) = resultDimensions->values();
  output->SetDimensions(dimensions);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  points->Allocate(numPts);

  numLines = numPts / 6; // integer division
  moreCoords = numPts % 6;

  for (i = 0; i < numLines; i++)
  {
    this->ReadNextDataLine(line);
    vtkEnSight6ReaderRead6(line, coords[0], coords[1], coords[2], coords[3], coords[4], coords[5]);
    for (j = 0; j < 6; j++)
    {
      points->InsertNextPoint(coords[j], 0.0, 0.0);
    }
  }
  if (moreCoords != 0)
  {
    this->ReadNextDataLine(line);
    for (j = 0; j < moreCoords; j++)
    {
      vtkEnSight6ReaderRead1(line + j * 12, coords[j]);
      points->InsertNextPoint(coords[j], 0.0, 0.0);
    }
  }
  for (i = 0; i < numLines; i++)
  {
    this->ReadNextDataLine(line);
    vtkEnSight6ReaderRead6(line, coords[0], coords[1], coords[2], coords[3], coords[4], coords[5]);
    for (j = 0; j < 6; j++)
    {
      points->GetPoint(i * 6 + j, point);
      points->SetPoint(i * 6 + j, point[0], static_cast<double>(coords[j]), point[2]);
    }
  }
  if (moreCoords != 0)
  {
    this->ReadNextDataLine(line);
    for (j = 0; j < moreCoords; j++)
    {
      vtkEnSight6ReaderRead1(line + j * 12, coords[j]);
      points->GetPoint(i * 6 + j, point);
      points->SetPoint(i * 6 + j, point[0], static_cast<double>(coords[j]), point[2]);
    }
  }
  for (i = 0; i < numLines; i++)
  {
    this->ReadNextDataLine(line);
    vtkEnSight6ReaderRead6(line, coords[0], coords[1], coords[2], coords[3], coords[4], coords[5]);
    for (j = 0; j < 6; j++)
    {
      points->GetPoint(i * 6 + j, point);
      points->SetPoint(i * 6 + j, point[0], point[1], static_cast<double>(coords[j]));
    }
  }
  if (moreCoords != 0)
  {
    this->ReadNextDataLine(line);
    for (j = 0; j < moreCoords; j++)
    {
      vtkEnSight6ReaderRead1(line + j * 12, coords[j]);
      points->GetPoint(i * 6 + j, point);
      points->SetPoint(i * 6 + j, point[0], point[1], static_cast<double>(coords[j]));
    }
  }

  numLines = numPts / 10;
  moreBlanking = numPts % 10;
  output->SetPoints(points);
  if (iblanked)
  {
    for (i = 0; i < numLines; i++)
    {
      this->ReadNextDataLine(line);
      auto resultInt10 = vtk::scan<int, int, int, int, int, int, int, int, int, int>(
        std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
      std::tie(iblanks[0], iblanks[1], iblanks[2], iblanks[3], iblanks[4], iblanks[5], iblanks[6],
        iblanks[7], iblanks[8], iblanks[9]) = resultInt10->values();
      for (j = 0; j < 10; j++)
      {
        if (!iblanks[j])
        {
          output->BlankPoint(i * numLines + j);
        }
      }
    }
    if (moreBlanking != 0)
    {
      this->ReadNextDataLine(line);
      std::string_view current(line);
      for (j = 0; j < moreBlanking; j++)
      {
        auto result = vtk::scan_int<int>(current);
        iblanks[j] = result->value();
        if (!iblanks[j])
        {
          output->BlankPoint(i * numLines + j);
        }
        current = std::string_view(result->range().data(), result->range().size());
      }
    }
  }

  points->Delete();
  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//------------------------------------------------------------------------------
void vtkEnSight6Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkEnSight6Reader::CleanUpCache()
{
  if (this->UnstructuredPoints)
  {
    this->NumberOfUnstructuredPoints = 0;
    this->UnstructuredPoints->Delete();
    this->UnstructuredPoints = nullptr;
  }
  if (this->UnstructuredNodeIds)
  {
    this->UnstructuredNodeIds->Delete();
    this->UnstructuredNodeIds = nullptr;
  }
}

VTK_ABI_NAMESPACE_END
