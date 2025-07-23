// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEnSightGoldReader.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStringScanner.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include "vtksys/FStream.hxx"

#include <algorithm>
#include <cctype>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkEnSightGoldReader);

class vtkEnSightGoldReader::FileOffsetMapInternal
{
public:
  std::map<std::string, std::map<int, long>> Map;
};

class vtkEnSightGoldReader::UndefPartialHelper
{
  bool HasUndef = false;
  double Undef = std::nanf("1");

  bool HasPartial = false;
  std::vector<vtkIdType> PartialIndices;

public:
  UndefPartialHelper(const char* line, vtkEnSightGoldReader* self)
  {
    // Look for keyword 'partial' or 'undef':
    auto resultUndefVar =
      vtk::scan<std::string_view, std::string_view>(std::string_view(line), " {:s} {:s}");
    if (resultUndefVar)
    {
      auto undefvar = std::get<1>(resultUndefVar->values());
      char subline[80];
      if (undefvar == "undef")
      {
        self->ReadNextDataLine(subline);
        this->Undef = vtk::scan_value<float>(std::string_view(subline))->value();
        this->HasUndef = true;
      }
      else if (undefvar == "partial")
      {
        self->ReadNextDataLine(subline);
        const int nLines = vtk::scan_int<int>(std::string_view(subline))->value();
        this->HasPartial = true;
        this->PartialIndices.resize(nLines, 0);
        for (int i = 0; i < nLines; ++i)
        {
          self->ReadNextDataLine(subline);
          const vtkIdType val =
            vtk::scan_int<int>(std::string_view(subline))->value() - 1; // EnSight start at 1
          this->PartialIndices[i] = val;
        }
      }
      else
      {
        const std::string undefvarStr(undefvar);
        vtkLogF(ERROR, "Unknown value for undef or partial: %s", undefvarStr.c_str());
      }
    }
  }

  void ReadArray(
    vtkFloatArray* array, int numberOfComponents, int component, vtkEnSightGoldReader* self)
  {
    if (numberOfComponents == 6)
    {
      // for 6 component tensors, the symmetric tensor components XZ and YZ are interchanged
      // see #10637.
      switch (component)
      {
        case 4:
          component = 5;
          break;

        case 5:
          component = 4;
          break;
      }
    }

    char line[256];
    if (this->HasPartial)
    {
      array->FillTypedComponent(component, std::nanf("1"));
      for (const auto& idx : this->PartialIndices)
      {
        self->ReadNextDataLine(line);
        array->InsertComponent(
          idx, component, vtk::scan_value<float>(std::string_view(line))->value());
      }
    }
    else
    {
      const auto undefValue = std::nanf("1");
      for (vtkIdType cc = 0, max = array->GetNumberOfTuples(); cc < max; ++cc)
      {
        self->ReadNextDataLine(line);
        const double val = vtk::scan_value<float>(std::string_view(line))->value();
        if (this->HasUndef && val == this->Undef)
        {
          array->InsertComponent(cc, component, undefValue);
        }
        else
        {
          array->InsertComponent(cc, component, val);
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
vtkEnSightGoldReader::vtkEnSightGoldReader()
{
  this->FileOffsets = new vtkEnSightGoldReader::FileOffsetMapInternal;

  this->NodeIdsListed = 0;
  this->ElementIdsListed = 0;
  // this->DebugOn();
}
//------------------------------------------------------------------------------

vtkEnSightGoldReader::~vtkEnSightGoldReader()
{
  delete this->FileOffsets;
}

//------------------------------------------------------------------------------
static void vtkEnSightGoldReaderRead4(
  const char* line, int& pointId, float& point1, float& point2, float& point3)
{
  const std::string_view lineView(line);
  pointId = vtk::scan_int<int>(lineView.substr(0, 8))->value();
  point1 = vtk::scan_value<float>(lineView.substr(8, 12))->value();
  point2 = vtk::scan_value<float>(lineView.substr(20, 12))->value();
  point3 = vtk::scan_value<float>(lineView.substr(32, 12))->value();
}

static void vtkEnSightGoldReaderRead6(const char* line, float& point1, float& point2, float& point3,
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

static void vtkEnSightGoldReaderRead1(const char* line, float& point1)
{
  point1 = vtk::scan_value<float>(std::string_view(line))->value();
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadGeometryFile(
  const char* fileName, int timeStep, vtkMultiBlockDataSet* output)
{
  char line[256];
  int partId, realId, i;
  int lineRead;

  // init line and subLine in case ReadLine(.), ReadNextDataLine(.), or
  // sscanf(...) fails while strncmp(..) is still subsequently performed
  // on these two un-assigned char arrays to cause memory leakage, as
  // detected by Valgrind. As an example, VTKData/Data/EnSight/test.geo
  // makes the first sscanf(...) below fail to assign 'subLine' that is
  // though then accessed by strnmp(..) for comparing two char arrays.
  line[0] = '\0';

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("A GeometryFileName must be specified in the case file.");
    return 0;
  }
  std::string sfilename;
  std::string filename_string(fileName);
  char quotes = '\"';
  size_t found = filename_string.find(quotes);
  if (found != std::string::npos)
  {
    filename_string.erase(
      std::remove(filename_string.begin(), filename_string.end(), quotes), filename_string.end());
  }
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += filename_string;
    vtkDebugMacro("full path to geometry file: " << sfilename);
  }
  else
  {
    sfilename = filename_string;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  this->ReadNextDataLine(line);
  auto resultSubLine0 =
    vtk::scan<std::string_view, std::string_view>(std::string_view(line), " {:s} {:s}");
  if (resultSubLine0)
  {
    auto subLine = std::get<1>(resultSubLine0->values());
    if (subLine == "Binary")
    {
      vtkErrorMacro("This is a binary data set. Try "
        << "vtkEnSightGoldBinaryReader.");
      return 0;
    }
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    int j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end() &&
        this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
      {
        this->IS->seekg(this->FileOffsets->Map[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
      this->ReadLine(line);
      if (this->FileOffsets->Map.find(fileName) == this->FileOffsets->Map.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets->Map[fileName] = tsMap;
      }
      this->FileOffsets->Map[fileName][j] = this->IS->tellg();
    }

    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadNextDataLine(line);
    }
    this->ReadLine(line);
  }

  // Skip description lines.  Using ReadLine instead of
  // ReadNextDataLine because the description line could be blank.
  this->ReadLine(line);

  // Read the node id and element id lines.
  this->ReadNextDataLine(line);
  auto resultSubLine1 = vtk::scan<std::string_view, std::string_view, std::string_view>(
    std::string_view(line), " {:s} {:s} {:s}");
  auto subLine = std::get<2>(resultSubLine1->values());
  if (subLine == "given")
  {
    this->NodeIdsListed = 1;
  }
  else if (subLine == "ignore")
  {
    this->NodeIdsListed = 1;
  }
  else
  {
    this->NodeIdsListed = 0;
  }

  this->ReadNextDataLine(line);
  auto resultSubLine2 = vtk::scan<std::string_view, std::string_view, std::string_view>(
    std::string_view(line), " {:s} {:s} {:s}");
  subLine = std::get<2>(resultSubLine2->values());
  if (subLine == "given")
  {
    this->ElementIdsListed = 1;
  }
  else if (subLine == "ignore")
  {
    this->ElementIdsListed = 1;
  }
  else
  {
    this->ElementIdsListed = 0;
  }

  lineRead = this->ReadNextDataLine(line); // "extents" or "part"
  if (strncmp(line, "extents", 7) == 0)
  {
    // Skipping the extent lines for now.
    this->ReadNextDataLine(line);
    this->ReadNextDataLine(line);
    this->ReadNextDataLine(line);
    lineRead = this->ReadNextDataLine(line); // "part"
  }

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->NumberOfGeometryParts++;
    this->ReadNextDataLine(line);
    partId = vtk::scan_int<int>(std::string_view(line))->value() - 1; // EnSight starts #ing at 1.
    realId = this->InsertNewPartId(partId);

    this->ReadNextDataLine(line); // part description line
    char* name = strdup(line);

    // fix to bug #0008305 --- The original "return 1" operation
    // upon "strncmp(line, "interface", 9) == 0"
    // was removed here as 'interface' is NOT a keyword of an EnSight Gold file.

    this->ReadNextDataLine(line);

    if (strncmp(line, "block", 5) == 0)
    {
      auto resultSubLine3 =
        vtk::scan<std::string_view, std::string_view>(std::string_view(line), " {:s} {:s}");
      if (resultSubLine3)
      {
        subLine = std::get<1>(resultSubLine3->values());
        if (subLine == "rectilinear")
        {
          // block rectilinear
          lineRead = this->CreateRectilinearGridOutput(realId, line, name, output);
        }
        else if (subLine == "uniform")
        {
          // block uniform
          lineRead = this->CreateImageDataOutput(realId, line, name, output);
        }
        else
        {
          // block iblanked
          lineRead = this->CreateStructuredGridOutput(realId, line, name, output);
        }
      }
      else
      {
        // block
        lineRead = this->CreateStructuredGridOutput(realId, line, name, output);
      }
    }
    else
    {
      lineRead = this->CreateUnstructuredGridOutput(realId, line, name, output);
      if (lineRead < 0)
      {
        free(name);
        delete this->IS;
        this->IS = nullptr;
        return 0;
      }
    }
    free(name);
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadMeasuredGeometryFile(
  const char* fileName, int timeStep, vtkMultiBlockDataSet* output)
{
  char line[256];
  vtkPoints* newPoints;
  int i;
  int tempId;
  vtkIdType id;
  float coords[3];
  vtkPolyData* geom;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("A MeasuredFileName must be specified in the case file.");
    return 0;
  }
  std::string sfilename;
  std::string filename_string(fileName);
  char quotes = '\"';
  size_t found = filename_string.find(quotes);
  if (found != std::string::npos)
  {
    filename_string.erase(
      std::remove(filename_string.begin(), filename_string.end(), quotes), filename_string.end());
  }
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += filename_string;
    vtkDebugMacro("full path to measured geometry file: " << sfilename);
  }
  else
  {
    sfilename = filename_string;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return 0;
  }

  // Skip the description line.  Using ReadLine instead of ReadNextDataLine
  // because the description line could be blank.
  this->ReadLine(line);

  auto resultSubLine =
    vtk::scan<std::string_view, std::string_view>(std::string_view(line), " {:s} {:s}");
  if (resultSubLine)
  {
    auto subLine = std::get<1>(resultSubLine->values());
    if (subLine == "Binary")
    {
      vtkErrorMacro("This is a binary data set. Try "
        << "vtkEnSight6BinaryReader.");
      return 0;
    }
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    int j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end() &&
        this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
      {
        this->IS->seekg(this->FileOffsets->Map[fileName][i], ios::beg);
        j = i;
        break;
      }
    }

    // Hopefully we are not very far from the timestep we want to use
    // Find it (and cache any timestep we find on the way...)
    while (j++ < realTimeStep)
    {
      while (strncmp(line, "END TIME STEP", 13) != 0)
      {
        this->ReadLine(line);
      }
      this->ReadLine(line);
      if (this->FileOffsets->Map.find(fileName) == this->FileOffsets->Map.end())
      {
        std::map<int, long> tsMap;
        this->FileOffsets->Map[fileName] = tsMap;
      }
      this->FileOffsets->Map[fileName][j] = this->IS->tellg();
    }

    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadNextDataLine(line);
    }
    this->ReadLine(line);
  }

  this->ReadLine(line); // "particle coordinates"
  this->ReadLine(line);
  this->NumberOfMeasuredPoints = vtk::scan_int<int>(std::string_view(line))->value();

  vtkDataSet* ds = this->GetDataSetFromBlock(output, this->NumberOfGeometryParts);
  if (ds == nullptr || !ds->IsA("vtkPolyData"))
  {
    vtkDebugMacro("creating new measured geometry output");
    vtkPolyData* pd = vtkPolyData::New();
    pd->AllocateEstimate(this->NumberOfMeasuredPoints, 1);
    this->AddToBlock(output, this->NumberOfGeometryParts, pd);
    ds = pd;
    pd->Delete();
  }

  geom = vtkPolyData::SafeDownCast(ds);

  newPoints = vtkPoints::New();
  newPoints->Allocate(this->NumberOfMeasuredPoints);

  for (i = 0; i < this->NumberOfMeasuredPoints; i++)
  {
    this->ReadLine(line);
    vtkEnSightGoldReaderRead4(line, tempId, coords[0], coords[1], coords[2]);
    // It seems EnSight always enumerate point indices from 1 to N
    // (not from 0 to N-1) and therefore there is no need to determine
    // flag 'ParticleCoordinatesByIndex'. Instead let's just use 'i',
    // or probably more safely (tempId - 1), as the point index. In this
    // way the geometry that is defined by the datasets mentioned in
    // bug #0008236 can be properly constructed. Fix to bug #0008236.
    id = i;

    newPoints->InsertNextPoint(coords);
    geom->InsertNextCell(VTK_VERTEX, 1, &id);
  }

  geom->SetPoints(newPoints);

  newPoints->Delete();

  return 1;
}

//------------------------------------------------------------------------------
bool vtkEnSightGoldReader::OpenVariableFile(const char* fileName, const char* variableType)
{
  if (!fileName)
  {
    vtkErrorMacro("nullptr " << variableType << " variable file name");
    return false;
  }

  std::string sfilename;
  std::string filename_string(fileName);
  char quotes = '\"';
  size_t found = filename_string.find(quotes);
  if (found != std::string::npos)
  {
    filename_string.erase(
      std::remove(filename_string.begin(), filename_string.end(), quotes), filename_string.end());
  }
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length() - 1) != '/')
    {
      sfilename += "/";
    }
    sfilename += filename_string;
    vtkDebugMacro("full path to variable (" << variableType << ") file: " << sfilename);
  }
  else
  {
    sfilename = filename_string;
  }

  this->IS = new vtksys::ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename);
    delete this->IS;
    this->IS = nullptr;
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkEnSightGoldReader::SkipToTimeStep(const char* fileName, int timeStep)
{
  if (!this->UseFileSets)
  {
    // nothing to do.
    return true;
  }

  const int realTimeStep = timeStep - 1;
  // Try to find the nearest time step for which we know the offset
  int j = 0;
  for (int i = realTimeStep; i >= 0; --i)
  {
    if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end() &&
      this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
    {
      this->IS->seekg(this->FileOffsets->Map[fileName][i], ios::beg);
      j = i;
      break;
    }
  }

  char line[256];
  // Hopefully we are not very far from the timestep we want to use
  // Find it (and cache any timestep we find on the way...)
  while (j++ < realTimeStep)
  {
    this->ReadLine(line);
    while (strncmp(line, "END TIME STEP", 13) != 0)
    {
      this->ReadLine(line);
    }
    if (this->FileOffsets->Map.find(fileName) == this->FileOffsets->Map.end())
    {
      std::map<int, long> tsMap;
      this->FileOffsets->Map[fileName] = tsMap;
    }
    this->FileOffsets->Map[fileName][j] = this->IS->tellg();
  }

  this->ReadLine(line);
  while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
  {
    this->ReadLine(line);
  }

  return true;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadScalarsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int measured, int numberOfComponents,
  int component)
{
  char line[256];
  int partId, realId, numPts, i, j, numLines, moreScalars;
  vtkFloatArray* scalars;
  float scalarsRead[6];
  vtkDataSet* output;

  // Initialize
  //
  if (!this->OpenVariableFile(fileName, "ScalarPerNode"))
  {
    return 0;
  }

  if (!this->SkipToTimeStep(fileName, timeStep))
  {
    return 0;
  }

  this->ReadNextDataLine(line); // skip the description line

  if (measured)
  {
    output = this->GetDataSetFromBlock(compositeOutput, this->NumberOfGeometryParts);
    numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      numLines = numPts / 6;
      moreScalars = numPts % 6;

      scalars = vtkFloatArray::New();
      scalars->SetNumberOfTuples(numPts);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numPts * numberOfComponents);

      for (i = 0; i < numLines; i++)
      {
        this->ReadNextDataLine(line);
        vtkEnSightGoldReaderRead6(line, scalarsRead[0], scalarsRead[1], scalarsRead[2],
          scalarsRead[3], scalarsRead[4], scalarsRead[5]);
        for (j = 0; j < 6; j++)
        {
          scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
        }
      }
      this->ReadNextDataLine(line);
      for (j = 0; j < moreScalars; j++)
      {
        vtkEnSightGoldReaderRead1(line + j * 12, scalarsRead[j]);
        scalars->InsertComponent(i * 6 + j, component, scalarsRead[j]);
      }
      scalars->SetName(description);
      output->GetPointData()->AddArray(scalars);
      if (!output->GetPointData()->GetScalars())
      {
        output->GetPointData()->SetScalars(scalars);
      }
      scalars->Delete();
    }
    delete this->IS;
    this->IS = nullptr;
    return 1;
  }

  while (this->ReadNextDataLine(line) && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = vtk::scan_int<int>(std::string_view(line))->value() - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      this->ReadNextDataLine(line); // "coordinates" or "block"

      UndefPartialHelper helper(line, this);
      if (component == 0)
      {
        scalars = vtkFloatArray::New();
        scalars->SetNumberOfComponents(numberOfComponents);
        scalars->SetNumberOfTuples(numPts);
      }
      else
      {
        scalars = (vtkFloatArray*)(output->GetPointData()->GetArray(description));
      }

      helper.ReadArray(scalars, numberOfComponents, component, this);
      if (component == 0)
      {
        scalars->SetName(description);
        output->GetPointData()->AddArray(scalars);
        if (!output->GetPointData()->GetScalars())
        {
          output->GetPointData()->SetScalars(scalars);
        }
        scalars->Delete();
      }
      else
      {
        output->GetPointData()->AddArray(scalars);
      }
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadVectorsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int measured)
{
  char line[256];
  int partId, realId, numPts, i, j, numLines, moreVectors;
  vtkFloatArray* vectors;
  float vector1[3], vector2[3];
  vtkDataSet* output;

  // Initialize
  if (!this->OpenVariableFile(fileName, "VectorPerNode"))
  {
    return 0;
  }

  if (!this->SkipToTimeStep(fileName, timeStep))
  {
    return 0;
  }

  this->ReadNextDataLine(line); // skip the description line

  if (measured)
  {
    output = this->GetDataSetFromBlock(compositeOutput, this->NumberOfGeometryParts);
    numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      this->ReadNextDataLine(line);
      numLines = numPts / 2;
      moreVectors = ((numPts * 3) % 6) / 3;
      vectors = vtkFloatArray::New();
      vectors->SetNumberOfTuples(numPts);
      vectors->SetNumberOfComponents(3);
      vectors->Allocate(numPts * 3);
      for (i = 0; i < numLines; i++)
      {
        vtkEnSightGoldReaderRead6(
          line, vector1[0], vector1[1], vector1[2], vector2[0], vector2[1], vector2[2]);
        vectors->InsertTuple(i * 2, vector1);
        vectors->InsertTuple(i * 2 + 1, vector2);
        this->ReadNextDataLine(line);
      }
      for (j = 0; j < moreVectors; j++)
      {
        vtkEnSightGoldReaderRead1(line + j * 36, vector1[0]);
        vtkEnSightGoldReaderRead1(line + j * 36 + 12, vector1[1]);
        vtkEnSightGoldReaderRead1(line + j * 36 + 24, vector1[2]);
        vectors->InsertTuple(i * 2 + j, vector1);
      }
      vectors->SetName(description);
      output->GetPointData()->AddArray(vectors);
      if (!output->GetPointData()->GetVectors())
      {
        output->GetPointData()->SetVectors(vectors);
      }
      vectors->Delete();
    }
    delete this->IS;
    this->IS = nullptr;
    return 1;
  }

  while (this->ReadNextDataLine(line) && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = vtk::scan_int<int>(std::string_view(line))->value() - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      this->ReadNextDataLine(line); // "coordinates" or "block"

      vectors = vtkFloatArray::New();
      vectors->SetNumberOfComponents(3);
      vectors->SetNumberOfTuples(numPts);

      UndefPartialHelper helper(line, this);
      for (i = 0; i < 3; i++)
      {
        helper.ReadArray(vectors, 3, i, this);
      }

      vectors->SetName(description);
      output->GetPointData()->AddArray(vectors);
      if (!output->GetPointData()->GetVectors())
      {
        output->GetPointData()->SetVectors(vectors);
      }
      vectors->Delete();
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadAsymmetricTensorsPerNode(const char* fileName,
  const char* description, int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  // Initialize
  if (!this->OpenVariableFile(fileName, "TensorPerNode"))
  {
    return 0;
  }

  if (!this->SkipToTimeStep(fileName, timeStep))
  {
    return 0;
  }

  std::string line;
  line.resize(80);

  // C++11 compatible way to get a pointer to underlying data
  // data() could be used with C++17
  char* linePtr = line.data();
  this->ReadNextDataLine(linePtr); // skip the description line

  while (this->ReadNextDataLine(linePtr) && line.compare(0, 4, "part") == 0)
  {
    this->ReadNextDataLine(linePtr);
    int partId =
      vtk::scan_int<int>(std::string_view(line))->value() - 1; // EnSight starts #ing with 1.
    int realId = this->InsertNewPartId(partId);
    vtkDataSet* output = this->GetDataSetFromBlock(compositeOutput, realId);
    int numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      vtkNew<vtkFloatArray> tensors;
      this->ReadNextDataLine(linePtr); // "coordinates" or "block"
      tensors->SetNumberOfComponents(9);
      tensors->SetNumberOfTuples(numPts);
      tensors->SetName(description);

      UndefPartialHelper helper(linePtr, this);
      for (int i = 0; i < 9; i++)
      {
        helper.ReadArray(tensors, 9, i, this);
      }
      output->GetPointData()->AddArray(tensors);
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadTensorsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  // Initialize
  if (!this->OpenVariableFile(fileName, "TensorPerNode"))
  {
    return 0;
  }

  if (!this->SkipToTimeStep(fileName, timeStep))
  {
    return 0;
  }

  char line[256];
  this->ReadNextDataLine(line); // skip the description line
  while (this->ReadNextDataLine(line) && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    int partId =
      vtk::scan_int<int>(std::string_view(line))->value() - 1; // EnSight starts #ing with 1.
    int realId = this->InsertNewPartId(partId);
    auto output = this->GetDataSetFromBlock(compositeOutput, realId);
    auto numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      auto tensors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // "coordinates" or "block"
      tensors->SetNumberOfComponents(6);
      tensors->SetNumberOfTuples(numPts);

      UndefPartialHelper helper(line, this);
      for (int i = 0; i < 6; i++)
      {
        helper.ReadArray(tensors, 6, i, this);
      }
      tensors->SetName(description);
      output->GetPointData()->AddArray(tensors);
      tensors->Delete();
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadScalarsPerElement(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput, int numberOfComponents, int component)
{

  // Initialize
  if (!this->OpenVariableFile(fileName, "ScalarPerElement"))
  {
    return 0;
  }

  if (!this->SkipToTimeStep(fileName, timeStep))
  {
    return 0;
  }

  char line[256];
  this->ReadNextDataLine(line);                // skip the description line
  int lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    auto partId =
      vtk::scan_int<int>(std::string_view(line))->value() - 1; // EnSight starts #ing with 1.
    auto realId = this->InsertNewPartId(partId);
    auto output = this->GetDataSetFromBlock(compositeOutput, realId);
    auto numCells = output->GetNumberOfCells();
    if (numCells)
    {
      this->ReadNextDataLine(line); // element type or "block"

      vtkFloatArray* scalars = nullptr;
      if (component == 0)
      {
        scalars = vtkFloatArray::New();
        scalars->SetNumberOfComponents(numberOfComponents);
        scalars->SetNumberOfTuples(numCells);
      }
      else
      {
        scalars = (vtkFloatArray*)(output->GetCellData()->GetArray(description));
      }

      // For element data (aka cell data), "part" may be followed by "[element type]";
      // if so, we need to read data in chunks rather than whole.
      if (strncmp(line, "block", 5) == 0)
      {
        // phew! no chunks, simply read all cell data.
        UndefPartialHelper helper(line, this);
        helper.ReadArray(scalars, numberOfComponents, component, this);
        lineRead = this->ReadNextDataLine(line);
      }
      else
      {
        // read one element type at a time.
        while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
        {
          const int elementType = this->GetElementType(line);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            delete this->IS;
            this->IS = nullptr;
            if (component == 0)
            {
              scalars->Delete();
            }
            return 0;
          }
          const auto idx = this->UnstructuredPartIds->IsId(realId);
          auto dstIds = this->GetCellIds(idx, elementType);
          const auto numCellsPerElement = dstIds->GetNumberOfIds();

          vtkNew<vtkIdList> srcIds;
          srcIds->SetNumberOfIds(numCellsPerElement);
          std::iota(srcIds->begin(), srcIds->end(), 0);

          UndefPartialHelper helper(line, this);

          vtkNew<vtkFloatArray> subArray;
          subArray->SetNumberOfComponents(numberOfComponents);
          subArray->SetNumberOfTuples(numCellsPerElement);
          if (component != 0)
          {
            // `scalars` already has some partial values. let's copy them first.
            subArray->InsertTuples(srcIds, dstIds, scalars);
          }
          helper.ReadArray(subArray, numberOfComponents, component, this);
          scalars->InsertTuples(dstIds, srcIds, subArray);

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
    else
    {
      lineRead = this->ReadNextDataLine(line);
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadVectorsPerElement(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  // Initialize
  if (!this->OpenVariableFile(fileName, "VectorPerElement"))
  {
    return 0;
  }

  if (!this->SkipToTimeStep(fileName, timeStep))
  {
    return 0;
  }

  char line[256];
  this->ReadNextDataLine(line);                // skip the description line
  int lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    const auto partId =
      vtk::scan_int<int>(std::string_view(line))->value() - 1; // EnSight starts #ing with 1.
    const auto realId = this->InsertNewPartId(partId);
    auto output = this->GetDataSetFromBlock(compositeOutput, realId);
    const auto numCells = output->GetNumberOfCells();
    if (numCells)
    {
      auto vectors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // element type or "block"
      vectors->SetNumberOfTuples(numCells);
      vectors->SetNumberOfComponents(3);
      vectors->Allocate(numCells * 3);

      // For element data (aka cell data), "part" may be followed by "[element type]";
      // if so, we need to read data in chunks rather than whole.
      if (strncmp(line, "block", 5) == 0)
      {
        // phew! no chunks, simply read all cell data.
        UndefPartialHelper helper(line, this);

        for (int i = 0; i < 3; i++)
        {
          helper.ReadArray(vectors, 3, i, this);
        }
        lineRead = this->ReadNextDataLine(line);
      }
      else
      {
        // read one element type at a time.
        while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
        {
          const int elementType = this->GetElementType(line);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            delete this->IS;
            this->IS = nullptr;
            vectors->Delete();
            return 0;
          }
          const auto idx = this->UnstructuredPartIds->IsId(realId);
          auto dstIds = this->GetCellIds(idx, elementType);
          const auto numCellsPerElement = dstIds->GetNumberOfIds();

          vtkNew<vtkFloatArray> subArray;
          subArray->SetNumberOfComponents(3);
          subArray->SetNumberOfTuples(numCellsPerElement);

          UndefPartialHelper helper(line, this);
          for (int i = 0; i < 3; i++)
          {
            helper.ReadArray(subArray, 3, i, this);
          }

          vtkNew<vtkIdList> srcIds;
          srcIds->SetNumberOfIds(numCellsPerElement);
          std::iota(srcIds->begin(), srcIds->end(), 0);
          vectors->InsertTuples(dstIds, srcIds, subArray);

          lineRead = this->ReadNextDataLine(line);
        } // end while
      }   // end else
      vectors->SetName(description);
      output->GetCellData()->AddArray(vectors);
      if (!output->GetCellData()->GetVectors())
      {
        output->GetCellData()->SetVectors(vectors);
      }
      vectors->Delete();
    }
    else
    {
      lineRead = this->ReadNextDataLine(line);
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadAsymmetricTensorsPerElement(const char* fileName,
  const char* description, int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  // Initialize
  if (!this->OpenVariableFile(fileName, "AsymetricTensorPerElement"))
  {
    return 0;
  }

  if (!this->SkipToTimeStep(fileName, timeStep))
  {
    return 0;
  }

  std::string line;
  line.resize(80);

  // C++11 compatible way to get a pointer to underlying data
  // data() could be used with C++17
  char* linePtr = line.data();
  this->ReadNextDataLine(linePtr);                // skip the description line
  int lineRead = this->ReadNextDataLine(linePtr); // "part"

  while (lineRead && line.compare(0, 4, "part") == 0)
  {
    this->ReadNextDataLine(linePtr);
    int partId =
      vtk::scan_int<int>(std::string_view(line))->value() - 1; // EnSight starts #ing with 1.
    int realId = this->InsertNewPartId(partId);
    vtkDataSet* output = this->GetDataSetFromBlock(compositeOutput, realId);
    int numCells = output->GetNumberOfCells();
    if (numCells)
    {
      vtkNew<vtkFloatArray> tensors;
      this->ReadNextDataLine(linePtr); // element type or "block"
      tensors->SetNumberOfComponents(9);
      tensors->SetNumberOfTuples(numCells);
      tensors->SetName(description);

      // For element data (aka cell data), "part" may be followed by "[element type]";
      // if so, we need to read data in chunks rather than whole.
      if (line.compare(0, 5, "block") == 0)
      {
        // phew! no chunks, simply read all cell data.
        UndefPartialHelper helper(linePtr, this);
        for (int i = 0; i < 9; i++)
        {
          helper.ReadArray(tensors, 9, i, this);
        }
        lineRead = this->ReadNextDataLine(linePtr);
      }
      else
      {
        // read one element type at a time.
        while (
          lineRead && line.compare(0, 4, "part") != 0 && line.compare(0, 13, "END TIME STEP") != 0)
        {
          int elementType = this->GetElementType(linePtr);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            delete[] this->IS;
            this->IS = nullptr;
            return 0;
          }
          int idx = this->UnstructuredPartIds->IsId(realId);
          auto dstIds = this->GetCellIds(idx, elementType);
          auto numCellsPerElement = dstIds->GetNumberOfIds();

          vtkNew<vtkFloatArray> subArray;
          subArray->SetNumberOfComponents(9);
          subArray->SetNumberOfTuples(numCellsPerElement);

          UndefPartialHelper helper(linePtr, this);
          for (int i = 0; i < 9; i++)
          {
            helper.ReadArray(subArray, 9, i, this);
          }

          vtkNew<vtkIdList> srcIds;
          srcIds->SetNumberOfIds(numCellsPerElement);
          std::iota(srcIds->begin(), srcIds->end(), 0);
          tensors->InsertTuples(dstIds, srcIds, subArray);

          lineRead = this->ReadNextDataLine(linePtr);
        } // end while
      }   // end else
      output->GetCellData()->AddArray(tensors);
    }
    else
    {
      lineRead = this->ReadNextDataLine(linePtr);
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadTensorsPerElement(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet* compositeOutput)
{
  // Initialize
  if (!this->OpenVariableFile(fileName, "TensorPerElement"))
  {
    vtkErrorMacro("Empty TensorPerElement variable file name");
    return 0;
  }

  if (!this->SkipToTimeStep(fileName, timeStep))
  {
    return 0;
  }

  char line[256];
  this->ReadNextDataLine(line);                // skip the description line
  int lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part", 4) == 0)
  {
    this->ReadNextDataLine(line);
    const auto partId =
      vtk::scan_int<int>(std::string_view(line))->value() - 1; // EnSight starts #ing with 1.
    const auto realId = this->InsertNewPartId(partId);
    auto output = this->GetDataSetFromBlock(compositeOutput, realId);
    const auto numCells = output->GetNumberOfCells();
    if (numCells)
    {
      auto tensors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // element type or "block"
      tensors->SetNumberOfTuples(numCells);
      tensors->SetNumberOfComponents(6);
      tensors->Allocate(numCells * 6);

      // For element data (aka cell data), "part" may be followed by "[element type]";
      // if so, we need to read data in chunks rather than whole.
      if (strncmp(line, "block", 5) == 0)
      {
        // phew! no chunks, simply read all cell data.
        UndefPartialHelper helper(line, this);
        for (int i = 0; i < 6; i++)
        {
          helper.ReadArray(tensors, 6, i, this);
        }
        lineRead = this->ReadNextDataLine(line);
      }
      else
      {
        // read one element type at a time.
        while (lineRead && strncmp(line, "part", 4) != 0 && strncmp(line, "END TIME STEP", 13) != 0)
        {
          const auto elementType = this->GetElementType(line);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            delete[] this->IS;
            this->IS = nullptr;
            tensors->Delete();
            return 0;
          }
          const auto idx = this->UnstructuredPartIds->IsId(realId);
          auto dstIds = this->GetCellIds(idx, elementType);
          const auto numCellsPerElement = dstIds->GetNumberOfIds();

          vtkNew<vtkFloatArray> subArray;
          subArray->SetNumberOfComponents(6);
          subArray->SetNumberOfTuples(numCellsPerElement);

          UndefPartialHelper helper(line, this);
          for (int i = 0; i < 6; i++)
          {
            helper.ReadArray(subArray, 6, i, this);
          }

          vtkNew<vtkIdList> srcIds;
          srcIds->SetNumberOfIds(numCellsPerElement);
          std::iota(srcIds->begin(), srcIds->end(), 0);
          tensors->InsertTuples(dstIds, srcIds, subArray);

          lineRead = this->ReadNextDataLine(line);
        } // end while
      }   // end else
      tensors->SetName(description);
      output->GetCellData()->AddArray(tensors);
      tensors->Delete();
    }
    else
    {
      lineRead = this->ReadNextDataLine(line);
    }
  }

  delete this->IS;
  this->IS = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::CreateUnstructuredGridOutput(
  int partId, char line[256], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  int lineRead = 1;
  int i, j, k;
  int numElements;
  int idx, cellType;
  vtkIdType cellId;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == nullptr || !ds->IsA("vtkUnstructuredGrid"))
  {
    vtkDebugMacro("creating new unstructured output");
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::New();
    this->AddToBlock(compositeOutput, partId, ugrid);
    ugrid->Delete();
    ds = ugrid;

    this->UnstructuredPartIds->InsertNextId(partId);
  }

  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  // Clear all cell ids from the last execution, if any.
  idx = this->UnstructuredPartIds->IsId(partId);
  for (i = 0; i < 16; i++)
  {
    this->GetCellIds(idx, i)->Reset();
  }

  output->Allocate(1000);

  while (lineRead && strncmp(line, "part", 4) != 0)
  {
    if (strncmp(line, "coordinates", 11) == 0)
    {
      vtkDebugMacro("coordinates");
      int numPts;
      vtkPoints* points = vtkPoints::New();
      double point[3];

      this->ReadNextDataLine(line);
      numPts = vtk::scan_int<int>(std::string_view(line))->value();
      vtkDebugMacro("num. points: " << numPts);

      points->Allocate(numPts);

      for (i = 0; i < numPts; i++)
      {
        this->ReadNextDataLine(line);
        points->InsertNextPoint(vtk::scan_value<float>(std::string_view(line))->value(), 0, 0);
      }
      for (i = 0; i < numPts; i++)
      {
        this->ReadNextDataLine(line);
        points->GetPoint(i, point);
        points->SetPoint(i, point[0], vtk::scan_value<float>(std::string_view(line))->value(), 0);
      }
      for (i = 0; i < numPts; i++)
      {
        this->ReadNextDataLine(line);
        points->GetPoint(i, point);
        points->SetPoint(
          i, point[0], point[1], vtk::scan_value<float>(std::string_view(line))->value());
      }

      lineRead = this->ReadNextDataLine(line);
      auto subLine = vtk::scan_value<std::string_view>(std::string_view(line))->value();

      // Testing if we can convert this string to double, ignore result
      auto result = vtk::scan_value<double>(subLine);
      if (result)
      { // necessary if node ids were listed
        for (i = 0; i < numPts; i++)
        {
          points->GetPoint(i, point);
          points->SetPoint(
            i, point[1], point[2], vtk::scan_value<float>(std::string_view(line))->value());
          lineRead = this->ReadNextDataLine(line);
        }
      }
      output->SetPoints(points);
      points->Delete();
    }
    else if (strncmp(line, "point", 5) == 0)
    {
      vtkDebugMacro("point");

      vtkIdType nodeIds;
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      std::vector<int> elementIds(numElements);

      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
        elementIds[i] = vtk::scan_int<int>(std::string_view(line))->value();
      }
      lineRead = this->ReadNextDataLine(line);
      auto subLine = vtk::scan_value<std::string_view>(std::string_view(line))->value();
      if (isdigit(subLine[0]))
      {
        for (i = 0; i < numElements; i++)
        {
          nodeIds = vtk::scan_int<int>(std::string_view(line))->value() -
            1; // because EnSight ids start at 1
          cellId = output->InsertNextCell(VTK_VERTEX, 1, &nodeIds);
          this->GetCellIds(idx, vtkEnSightReader::POINT)->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
        }
      }
      else
      {
        for (i = 0; i < numElements; i++)
        {
          nodeIds = elementIds[i] - 1;
          cellId = output->InsertNextCell(VTK_VERTEX, 1, &nodeIds);
          this->GetCellIds(idx, vtkEnSightReader::POINT)->InsertNextId(cellId);
        }
      }
    }
    else if (strncmp(line, "g_point", 7) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_point");

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();

      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
      }
      lineRead = this->ReadNextDataLine(line);
      auto subLine = vtk::scan_value<std::string_view>(std::string_view(line))->value();
      if (isdigit(subLine[0]))
      {
        for (i = 0; i < numElements; i++)
        {
          lineRead = this->ReadNextDataLine(line);
        }
      }
    }
    else if (strncmp(line, "bar2", 4) == 0)
    {
      vtkDebugMacro("bar2");

      vtkIdType nodeIds[2];
      int intIds[2];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int>(std::string_view(line), " {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt2 = vtk::scan<int, int>(std::string_view(line), " {:d} {:d}");
        std::tie(intIds[0], intIds[1]) = resultInt2->values();
        for (j = 0; j < 2; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_LINE, 2, nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::BAR2)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "g_bar2", 6) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_bar2");

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int>(std::string_view(line), " {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "bar3", 4) == 0)
    {
      vtkDebugMacro("bar3");
      vtkIdType nodeIds[3];
      int intIds[3];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int>(std::string_view(line), " {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt3 = vtk::scan<int, int, int>(std::string_view(line), " {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2]) = resultInt3->values();
        for (j = 0; j < 3; j++)
        {
          intIds[j]--;
        }
        nodeIds[0] = intIds[0];
        nodeIds[1] = intIds[2];
        nodeIds[2] = intIds[1];

        cellId = output->InsertNextCell(VTK_QUADRATIC_EDGE, 3, nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::BAR3)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "g_bar3", 6) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_bar3");

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int>(std::string_view(line), " {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "nsided", 6) == 0)
    {
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      if (this->ElementIdsListed)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }

      std::vector<int> numNodesPerElement(numElements);
      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
        numNodesPerElement[i] = vtk::scan_int<int>(std::string_view(line))->value();
      }

      lineRead = this->ReadNextDataLine(line);
      for (i = 0; i < numElements; i++)
      {
        const auto& numNodes = numNodesPerElement[i];
        std::vector<vtkIdType> nodeIds(numNodes);

        std::stringstream lineStream;
        lineStream.str(line);
        lineStream.seekp(0, std::stringstream::end);
        while (!lineRead)
        {
          lineRead = this->ReadNextDataLine(line);
          lineStream.write(line, strlen(line));
          lineStream.seekp(0, std::stringstream::end);
        }
        const std::string fullLine = lineStream.str();
        std::string_view current(fullLine.data(), fullLine.size());
        for (j = 0; j < numNodes; j++)
        {
          auto resultInt = vtk::scan_int<vtkIdType>(current);
          nodeIds[numNodes - j - 1] = resultInt->value() - 1; // EnSight ids start at 1
          current = std::string_view(resultInt->range().data(), resultInt->range().size());
        }
        cellId = output->InsertNextCell(VTK_POLYGON, numNodes, nodeIds.data());
        this->GetCellIds(idx, vtkEnSightReader::NSIDED)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "g_nsided", 8) == 0)
    {
      // skipping ghost cells
      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      for (i = 0; i < numElements * 2; i++)
      {
        this->ReadNextDataLine(line);
      }
      lineRead = this->ReadNextDataLine(line);
      if (lineRead)
      {
        auto subLine = vtk::scan_value<std::string_view>(std::string_view(line))->value();
        if (isdigit(subLine[0]))
        {
          // We still need to read in the node ids for each element.
          for (i = 0; i < numElements; i++)
          {
            lineRead = this->ReadNextDataLine(line);
          }
        }
      }
    }
    else if (strncmp(line, "tria3", 5) == 0)
    {
      vtkDebugMacro("tria3");
      cellType = vtkEnSightReader::TRIA3;

      vtkIdType nodeIds[3];
      int intIds[3];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int>(std::string_view(line), " {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt3 = vtk::scan<int, int, int>(std::string_view(line), " {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2]) = resultInt3->values();
        for (j = 0; j < 3; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_TRIANGLE, 3, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "tria6", 5) == 0)
    {
      vtkDebugMacro("tria6");
      cellType = vtkEnSightReader::TRIA6;

      vtkIdType nodeIds[6];
      int intIds[6];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int, int>(
            std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt6 = vtk::scan<int, int, int, int, int, int>(
          std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5]) =
          resultInt6->values();
        for (j = 0; j < 6; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_TRIANGLE, 6, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "g_tria3", 7) == 0 || strncmp(line, "g_tria6", 7) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_tria6", 7) == 0)
      {
        vtkDebugMacro("g_tria6");
        cellType = vtkEnSightReader::TRIA6;
      }
      else
      {
        vtkDebugMacro("g_tria3");
        cellType = vtkEnSightReader::TRIA3;
      }

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int>(std::string_view(line), " {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "quad4", 5) == 0)
    {
      vtkDebugMacro("quad4");
      cellType = vtkEnSightReader::QUAD4;

      vtkIdType nodeIds[4];
      int intIds[4];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int>(std::string_view(line), " {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt4 =
          vtk::scan<int, int, int, int>(std::string_view(line), " {:d} {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3]) = resultInt4->values();
        for (j = 0; j < 4; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUAD, 4, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "quad8", 5) == 0)
    {
      vtkDebugMacro("quad8");
      cellType = vtkEnSightReader::QUAD8;

      vtkIdType nodeIds[8];
      int intIds[8];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int, int, int, int>(
            std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt8 = vtk::scan<int, int, int, int, int, int, int, int>(
          std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5], intIds[6],
          intIds[7]) = resultInt8->values();
        for (j = 0; j < 8; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_QUAD, 8, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "g_quad4", 7) == 0 || strncmp(line, "g_quad8", 7) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_quad8", 7) == 0)
      {
        vtkDebugMacro("g_quad8");
        cellType = vtkEnSightReader::QUAD8;
      }
      else
      {
        vtkDebugMacro("g_quad4");
        cellType = vtkEnSightReader::QUAD4;
      }

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int>(std::string_view(line), " {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "nfaced", 6) == 0)
    {
      int numPts = 0;
      int numFaces = 0;
      int numNodes = 0;
      int faceCount = 0;
      int elementNodeCount = 0;

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      if (this->ElementIdsListed)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }

      std::vector<int> numFacesPerElement(numElements);
      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
        numFacesPerElement[i] = vtk::scan_int<int>(std::string_view(line))->value();
        numFaces += numFacesPerElement[i];
      }

      std::vector<int> numNodesPerFace(numFaces);
      for (i = 0; i < numFaces; i++)
      {
        this->ReadNextDataLine(line);
        numNodesPerFace[i] = vtk::scan_int<int>(std::string_view(line))->value();
      }

      numPts = output->GetNumberOfPoints();
      std::vector<int> nodeMarker(numPts);
      for (i = 0; i < numPts; i++)
      {
        nodeMarker[i] = -1;
      }

      lineRead = this->ReadNextDataLine(line);
      for (i = 0; i < numElements; i++)
      {
        numNodes = 0;
        for (j = 0; j < numFacesPerElement[i]; j++)
        {
          numNodes += numNodesPerFace[faceCount + j];
        }
        std::vector<int> intIds(numNodes);

        // Read element node ids
        elementNodeCount = 0;
        for (j = 0; j < numFacesPerElement[i]; j++)
        {
          std::stringstream lineStream;
          lineStream.str(line);
          lineStream.seekp(0, std::stringstream::end);
          while (!lineRead)
          {
            lineRead = this->ReadNextDataLine(line);
            lineStream.write(line, strlen(line));
            lineStream.seekp(0, std::stringstream::end);
          }
          const std::string fullLine = lineStream.str();
          std::string_view current(fullLine.data(), fullLine.size());
          for (k = 0; k < numNodesPerFace[faceCount + j]; k++)
          {
            auto resultInt = vtk::scan_int<int>(current);
            intIds[elementNodeCount] = resultInt->value();
            elementNodeCount += 1;
            current = std::string_view(resultInt->range().data(), resultInt->range().size());
          }
          lineRead = this->ReadNextDataLine(line);
        }

        // prepare an array of Ids describing the vtkPolyhedron object yyy begin
        int nodeIndx = 0;              // indexing the raw array of point Ids
        vtkNew<vtkCellArray> theFaces; // vtkPolyhedron's info of faces
        for (j = 0; j < numFacesPerElement[i]; j++)
        {
          // number of points constituting this face
          theFaces->InsertNextCell(numNodesPerFace[faceCount + j]);

          for (k = 0; k < numNodesPerFace[faceCount + j]; k++)
          {
            // convert EnSight 1-based indexing to VTK 0-based indexing
            theFaces->InsertCellPoint(intIds[nodeIndx++] - 1);
          }
        }
        // yyy end

        faceCount += numFacesPerElement[i];

        // Build element
        std::vector<vtkIdType> nodeIds(numNodes);
        elementNodeCount = 0;
        for (j = 0; j < numNodes; j++)
        {
          if (nodeMarker[intIds[j] - 1] < i)
          {
            nodeIds[elementNodeCount] = intIds[j] - 1;
            nodeMarker[intIds[j] - 1] = i;
            elementNodeCount += 1;
          }
        }
        // xxx begin
        // cellId = output->InsertNextCell( VTK_CONVEX_POINT_SET,
        //                                 elementNodeCount, nodeIds );
        // xxx end

        // insert the cell as a vtkPolyhedron object yyy begin
        cellId = output->InsertNextCell(VTK_POLYHEDRON, elementNodeCount, nodeIds.data(), theFaces);
        // yyy end

        this->GetCellIds(idx, vtkEnSightReader::NFACED)->InsertNextId(cellId);
      }
    }
    else if (strncmp(line, "tetra4", 6) == 0)
    {
      vtkDebugMacro("tetra4");
      cellType = vtkEnSightReader::TETRA4;

      vtkIdType nodeIds[4];
      int intIds[4];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int>(std::string_view(line), " {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt4 =
          vtk::scan<int, int, int, int>(std::string_view(line), " {:d} {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3]) = resultInt4->values();
        for (j = 0; j < 4; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_TETRA, 4, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "tetra10", 7) == 0)
    {
      vtkDebugMacro("tetra10");
      cellType = vtkEnSightReader::TETRA10;

      vtkIdType nodeIds[10];
      int intIds[10];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int, int, int, int, int, int>(
            std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt10 = vtk::scan<int, int, int, int, int, int, int, int, int, int>(
          std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5], intIds[6],
          intIds[7], intIds[8], intIds[9]) = resultInt10->values();
        for (j = 0; j < 10; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_TETRA, 10, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "g_tetra4", 8) == 0 || strncmp(line, "g_tetra10", 9) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_tetra10", 9) == 0)
      {
        vtkDebugMacro("g_tetra10");
        cellType = vtkEnSightReader::TETRA10;
      }
      else
      {
        vtkDebugMacro("g_tetra4");
        cellType = vtkEnSightReader::TETRA4;
      }

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int>(std::string_view(line), " {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "pyramid5", 8) == 0)
    {
      vtkDebugMacro("pyramid5");
      cellType = vtkEnSightReader::PYRAMID5;

      vtkIdType nodeIds[5];
      int intIds[5];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int>(std::string_view(line), " {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt5 =
          vtk::scan<int, int, int, int, int>(std::string_view(line), " {:d} {:d} {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4]) = resultInt5->values();
        for (j = 0; j < 5; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_PYRAMID, 5, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "pyramid13", 9) == 0)
    {
      vtkDebugMacro("pyramid13");
      cellType = vtkEnSightReader::PYRAMID13;

      vtkIdType nodeIds[13];
      int intIds[13];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int, int, int, int, int, int, int, int, int>(
            std::string_view(line),
            " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt13 =
          vtk::scan<int, int, int, int, int, int, int, int, int, int, int, int, int>(
            std::string_view(line),
            " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5], intIds[6],
          intIds[7], intIds[8], intIds[9], intIds[10], intIds[11], intIds[12]) =
          resultInt13->values();
        for (j = 0; j < 13; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_PYRAMID, 13, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "g_pyramid5", 10) == 0 || strncmp(line, "g_pyramid13", 11) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_pyramid13", 11) == 0)
      {
        vtkDebugMacro("g_pyramid13");
        cellType = vtkEnSightReader::PYRAMID13;
      }
      else
      {
        vtkDebugMacro("g_pyramid5");
        cellType = vtkEnSightReader::PYRAMID5;
      }

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int>(std::string_view(line), " {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "hexa8", 5) == 0)
    {
      vtkDebugMacro("hexa8");
      cellType = vtkEnSightReader::HEXA8;

      vtkIdType nodeIds[8];
      int intIds[8];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int, int, int, int>(
            std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt8 = vtk::scan<int, int, int, int, int, int, int, int>(
          std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5], intIds[6],
          intIds[7]) = resultInt8->values();
        for (j = 0; j < 8; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_HEXAHEDRON, 8, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "hexa20", 6) == 0)
    {
      vtkDebugMacro("hexa20");
      cellType = vtkEnSightReader::HEXA20;

      vtkIdType nodeIds[20];
      int intIds[20];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int,
            int, int, int, int>(std::string_view(line),
            " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} "
            "{:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt20 = vtk::scan<int, int, int, int, int, int, int, int, int, int, int, int,
          int, int, int, int, int, int, int, int>(std::string_view(line),
          " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} "
          "{:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5], intIds[6],
          intIds[7], intIds[8], intIds[9], intIds[10], intIds[11], intIds[12], intIds[13],
          intIds[14], intIds[15], intIds[16], intIds[17], intIds[18], intIds[19]) =
          resultInt20->values();
        for (j = 0; j < 20; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "g_hexa8", 7) == 0 || strncmp(line, "g_hexa20", 8) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_hexa20", 8) == 0)
      {
        vtkDebugMacro("g_hexa20");
        cellType = vtkEnSightReader::HEXA20;
      }
      else
      {
        vtkDebugMacro("g_hexa8");
        cellType = vtkEnSightReader::HEXA8;
      }

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int, int, int, int>(
            std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "penta6", 6) == 0)
    {
      constexpr unsigned char wedgeMap[6] = { 0, 2, 1, 3, 5, 4 };

      vtkDebugMacro("penta6");
      cellType = vtkEnSightReader::PENTA6;

      vtkIdType nodeIds[6];
      int intIds[6];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int, int>(
            std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt6 = vtk::scan<int, int, int, int, int, int>(
          std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5]) =
          resultInt6->values();
        for (j = 0; j < 6; j++)
        {
          intIds[j]--;
          nodeIds[wedgeMap[j]] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_WEDGE, 6, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "penta15", 7) == 0)
    {
      constexpr unsigned char wedgeMap[15] = { 0, 2, 1, 3, 5, 4, 8, 7, 6, 11, 10, 9, 12, 14, 13 };

      vtkDebugMacro("penta15");
      cellType = vtkEnSightReader::PENTA15;

      vtkIdType nodeIds[15];
      int intIds[15];

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int, int, int, int, int, int, int, int, int, int, int>(
            std::string_view(line),
            " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        auto resultInt15 =
          vtk::scan<int, int, int, int, int, int, int, int, int, int, int, int, int, int, int>(
            std::string_view(line),
            " {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d} {:d}");
        std::tie(intIds[0], intIds[1], intIds[2], intIds[3], intIds[4], intIds[5], intIds[6],
          intIds[7], intIds[8], intIds[9], intIds[10], intIds[11], intIds[12], intIds[13],
          intIds[14]) = resultInt15->values();
        for (j = 0; j < 15; j++)
        {
          intIds[j]--;
          nodeIds[wedgeMap[j]] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_WEDGE, 15, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "g_penta6", 8) == 0 || strncmp(line, "g_penta15", 9) == 0)
    {
      // skipping ghost cells
      if (strncmp(line, "g_penta15", 9) == 0)
      {
        vtkDebugMacro("g_penta15");
        cellType = vtkEnSightReader::PENTA15;
      }
      else
      {
        vtkDebugMacro("g_penta6");
        cellType = vtkEnSightReader::PENTA6;
      }

      this->ReadNextDataLine(line);
      numElements = vtk::scan_int<int>(std::string_view(line))->value();
      this->ReadNextDataLine(line);
      if (!vtk::scan<int, int, int, int, int, int>(
            std::string_view(line), " {:d} {:d} {:d} {:d} {:d} {:d}"))
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        lineRead = this->ReadNextDataLine(line);
      }
    }
    else if (strncmp(line, "END TIME STEP", 13) == 0)
    {
      return 1;
    }
    else if (this->IS->fail())
    {
      // May want consistency check here?
      // vtkWarningMacro("EOF on geometry file");
      return 1;
    }
    else
    {
      vtkErrorMacro("undefined geometry file line");
      return -1;
    }
  }

  this->ApplyRigidBodyTransforms(partId, name, output);

  return lineRead;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::CreateStructuredGridOutput(
  int partId, char line[256], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkPoints* points = vtkPoints::New();
  double point[3];
  int numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == nullptr || !ds->IsA("vtkStructuredGrid"))
  {
    vtkDebugMacro("creating new structured grid output");
    vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
    this->AddToBlock(compositeOutput, partId, sgrid);
    sgrid->Delete();
    ds = sgrid;
  }

  vtkStructuredGrid* output = vtkStructuredGrid::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  auto resultSubLine =
    vtk::scan<std::string_view, std::string_view>(std::string_view(line), " {:s} {:s}");
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

  for (i = 0; i < numPts; i++)
  {
    this->ReadNextDataLine(line);
    points->InsertNextPoint(vtk::scan_value<float>(std::string_view(line))->value(), 0.0, 0.0);
  }
  for (i = 0; i < numPts; i++)
  {
    this->ReadNextDataLine(line);
    points->GetPoint(i, point);
    points->SetPoint(
      i, point[0], vtk::scan_value<float>(std::string_view(line))->value(), point[2]);
  }
  for (i = 0; i < numPts; i++)
  {
    this->ReadNextDataLine(line);
    points->GetPoint(i, point);
    points->SetPoint(
      i, point[0], point[1], vtk::scan_value<float>(std::string_view(line))->value());
  }
  output->SetPoints(points);
  if (iblanked)
  {
    for (i = 0; i < numPts; i++)
    {
      this->ReadNextDataLine(line);
      if (!vtk::scan_int<int>(std::string_view(line))->value())
      {
        output->BlankPoint(i);
      }
    }
  }

  this->ApplyRigidBodyTransforms(partId, name, output);

  points->Delete();
  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::CreateRectilinearGridOutput(
  int partId, char line[256], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkFloatArray* xCoords = vtkFloatArray::New();
  vtkFloatArray* yCoords = vtkFloatArray::New();
  vtkFloatArray* zCoords = vtkFloatArray::New();
  int numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == nullptr || !ds->IsA("vtkRectilinearGrid"))
  {
    vtkDebugMacro("creating new structured grid output");
    vtkRectilinearGrid* rgrid = vtkRectilinearGrid::New();
    this->AddToBlock(compositeOutput, partId, rgrid);
    rgrid->Delete();

    ds = rgrid;
  }

  vtkRectilinearGrid* output = vtkRectilinearGrid::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  auto resultSubLine = vtk::scan<std::string_view, std::string_view, std::string_view>(
    std::string_view(line), " {:s} {:s} {:s}");
  if (resultSubLine)
  {
    auto subLine = std::get<2>(resultSubLine->values());
    if (subLine == "iblanked")
    {
      iblanked = 1;
    }
  }

  this->ReadNextDataLine(line);
  auto resultDimensions = vtk::scan<int, int, int>(std::string_view(line), " {:d} {:d} {:d}");
  std::tie(dimensions[0], dimensions[1], dimensions[2]) = resultDimensions->values();
  output->SetDimensions(dimensions);
  xCoords->Allocate(dimensions[0]);
  yCoords->Allocate(dimensions[1]);
  zCoords->Allocate(dimensions[2]);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];

  float val = 1;

  for (i = 0; i < dimensions[0]; i++)
  {
    this->ReadNextDataLine(line);
    val = vtk::scan_value<float>(std::string_view(line))->value();
    xCoords->InsertNextTuple(&val);
  }
  for (i = 0; i < dimensions[1]; i++)
  {
    this->ReadNextDataLine(line);
    val = vtk::scan_value<float>(std::string_view(line))->value();
    yCoords->InsertNextTuple(&val);
  }
  for (i = 0; i < dimensions[2]; i++)
  {
    this->ReadNextDataLine(line);
    val = vtk::scan_value<float>(std::string_view(line))->value();
    zCoords->InsertNextTuple(&val);
  }
  if (iblanked)
  {
    vtkDebugMacro("VTK does not handle blanking for rectilinear grids.");
    for (i = 0; i < numPts; i++)
    {
      this->ReadNextDataLine(line);
    }
  }

  output->SetXCoordinates(xCoords);
  output->SetYCoordinates(yCoords);
  output->SetZCoordinates(zCoords);

  xCoords->Delete();
  yCoords->Delete();
  zCoords->Delete();

  this->ApplyRigidBodyTransforms(partId, name, output);

  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//------------------------------------------------------------------------------
int vtkEnSightGoldReader::CreateImageDataOutput(
  int partId, char line[256], const char* name, vtkMultiBlockDataSet* compositeOutput)
{
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int i;
  float origin[3], delta[3];
  int numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == nullptr || !ds->IsA("vtkImageData"))
  {
    vtkDebugMacro("creating new image data output");
    vtkImageData* idata = vtkImageData::New();
    this->AddToBlock(compositeOutput, partId, idata);
    idata->Delete();

    ds = idata;
  }

  vtkImageData* output = vtkImageData::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  auto resultSubLine = vtk::scan<std::string_view, std::string_view, std::string_view>(
    std::string_view(line), " {:s} {:s} {:s}");
  if (resultSubLine)
  {
    auto subLine = std::get<2>(resultSubLine->values());
    if (subLine == "iblanked")
    {
      iblanked = 1;
    }
  }

  this->ReadNextDataLine(line);
  auto resultDimensions = vtk::scan<int, int, int>(std::string_view(line), " {:d} {:d} {:d}");
  std::tie(dimensions[0], dimensions[1], dimensions[2]) = resultDimensions->values();
  output->SetDimensions(dimensions);

  for (i = 0; i < 3; i++)
  {
    this->ReadNextDataLine(line);
    origin[i] = vtk::scan_value<float>(std::string_view(line))->value();
  }
  output->SetOrigin(origin[0], origin[1], origin[2]);

  for (i = 0; i < 3; i++)
  {
    this->ReadNextDataLine(line);
    delta[i] = vtk::scan_value<float>(std::string_view(line))->value();
  }

  output->SetSpacing(delta[0], delta[1], delta[2]);

  if (iblanked)
  {
    vtkDebugMacro("VTK does not handle blanking for image data.");
    numPts = dimensions[0] * dimensions[1] * dimensions[2];
    for (i = 0; i < numPts; i++)
    {
      this->ReadNextDataLine(line);
    }
  }

  this->ApplyRigidBodyTransforms(partId, name, output);

  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//------------------------------------------------------------------------------
void vtkEnSightGoldReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
