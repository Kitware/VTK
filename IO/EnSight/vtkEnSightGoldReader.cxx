/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightGoldReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEnSightGoldReader.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <cctype>
#include <sstream>
#include <string>
#include <vector>
#include <map>

vtkStandardNewMacro(vtkEnSightGoldReader);


class vtkEnSightGoldReader::FileOffsetMapInternal
{
  public:
    std::map<std::string,
    std::map<int, long> > Map;
};
class vtkEnSightGoldReader::UndefPartialInternal
{
public:
  double UndefCoordinates;
  double UndefBlock;
  double UndefElementTypes;
  std::vector<vtkIdType> PartialCoordinates;
  std::vector<vtkIdType> PartialBlock;
  std::vector<vtkIdType> PartialElementTypes;
};



//----------------------------------------------------------------------------
vtkEnSightGoldReader::vtkEnSightGoldReader()
{
  this->UndefPartial = new vtkEnSightGoldReader::UndefPartialInternal;
  this->FileOffsets = new vtkEnSightGoldReader::FileOffsetMapInternal;

  this->NodeIdsListed = 0;
  this->ElementIdsListed = 0;
  //this->DebugOn();
}
//----------------------------------------------------------------------------

vtkEnSightGoldReader::~vtkEnSightGoldReader()
{
  delete this->UndefPartial;
  delete this->FileOffsets;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadGeometryFile(const char* fileName, int timeStep,
  vtkMultiBlockDataSet *output)
{
  char line[256], subLine[256];
  int partId, realId, i;
  int lineRead;

  // init line and subLine in case ReadLine(.), ReadNextDataLine(.), or
  // sscanf(...) fails while strncmp(..) is still subsequently performed
  // on these two un-assigned char arrays to cause memory leakage, as
  // detected by Valgrind. As an example, VTKData/Data/EnSight/test.geo
  // makes the first sscanf(...) below fail to assign 'subLine' that is
  // though then accessed by strnmp(..) for comparing two char arrays.
  line[0]    = '\0';
  subLine[0] = '\0';

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
    if (sfilename.at(sfilename.length()-1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to geometry file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %*s %s", subLine);
  if (strncmp(subLine, "Binary",6) == 0)
  {
    vtkErrorMacro("This is a binary data set. Try "
      <<"vtkEnSightGoldBinaryReader.");
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    int j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end()
        && this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
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

    while(strncmp(line, "BEGIN TIME STEP", 15) != 0)
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
  sscanf(line, " %*s %*s %s", subLine);
  if (strncmp(subLine, "given", 5) == 0)
  {
    this->NodeIdsListed = 1;
  }
  else if (strncmp(subLine, "ignore", 6) == 0)
  {
    this->NodeIdsListed = 1;
  }
  else
  {
    this->NodeIdsListed = 0;
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strncmp(subLine, "given", 5) == 0)
  {
    this->ElementIdsListed = 1;
  }
  else if (strncmp(subLine, "ignore", 6) == 0)
  {
    this->ElementIdsListed = 1;
  }
  else
  {
    this->ElementIdsListed = 0;
  }

  lineRead = this->ReadNextDataLine(line); // "extents" or "part"
  if (strncmp(line, "extents",7) == 0)
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
    partId = atoi(line) - 1; // EnSight starts #ing at 1.
    realId = this->InsertNewPartId(partId);

    this->ReadNextDataLine(line); // part description line
    char *name = strdup(line);

    // fix to bug #0008305 --- The original "return 1" operation
    // upon "strncmp(line, "interface", 9) == 0"
    // was removed here as 'interface' is NOT a keyword of an EnSight Gold file.

    this->ReadNextDataLine(line);

    if (strncmp(line, "block", 5) == 0)
    {
      if (sscanf(line, " %*s %s", subLine) == 1)
      {
        if (strncmp(subLine, "rectilinear",11) == 0)
        {
          // block rectilinear
          lineRead = this->CreateRectilinearGridOutput(realId, line, name,
            output);
        }
        else if (strncmp(subLine, "uniform",7) == 0)
        {
          // block uniform
          lineRead = this->CreateImageDataOutput(realId, line, name,
            output);
        }
        else
        {
          // block iblanked
          lineRead = this->CreateStructuredGridOutput(realId, line, name,
            output);
        }
      }
      else
      {
        // block
        lineRead = this->CreateStructuredGridOutput(realId, line, name,
          output);
      }
    }
    else
    {
      lineRead = this->CreateUnstructuredGridOutput(realId, line, name, output);
      if (lineRead < 0)
      {
        free(name);
        delete this->IS;
        this->IS = NULL;
        return 0;
      }
    }
    free(name);
  }

  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadMeasuredGeometryFile(
  const char* fileName, int timeStep, vtkMultiBlockDataSet *output)
{
  char line[256], subLine[256];
  vtkPoints *newPoints;
  int i;
  int tempId;
  vtkIdType id;
  float coords[3];
  vtkPolyData *geom;

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
    if (sfilename.at(sfilename.length()-1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to measured geometry file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
  }

  // Skip the description line.  Using ReadLine instead of ReadNextDataLine
  // because the description line could be blank.
  this->ReadLine(line);

  if (sscanf(line, " %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "Binary",6) == 0)
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
      if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end()
        && this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
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

    while(strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
      this->ReadNextDataLine(line);
    }
    this->ReadLine(line);
  }

  this->ReadLine(line); // "particle coordinates"
  this->ReadLine(line);
  this->NumberOfMeasuredPoints = atoi(line);

  vtkDataSet* ds = this->GetDataSetFromBlock(
    output, this->NumberOfGeometryParts);
  if (ds == NULL || !ds->IsA("vtkPolyData"))
  {
    vtkDebugMacro("creating new measured geometry output");
    vtkPolyData* pd = vtkPolyData::New();
    pd->Allocate(this->NumberOfMeasuredPoints);
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
    sscanf(line, " %8d %12e %12e %12e", &tempId, &coords[0], &coords[1],
      &coords[2]);

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

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadScalarsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet *compositeOutput,
  int measured,
  int numberOfComponents,
  int component)
{
  char line[256], formatLine[256], tempLine[256];
  int partId, realId, numPts, i, j, numLines, moreScalars;
  vtkFloatArray *scalars;
  float scalarsRead[6];
  vtkDataSet *output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL ScalarPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length()-1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to scalar per node file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end()
        && this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
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
  }

  this->ReadNextDataLine(line); // skip the description line

  if (measured)
  {
    output = this->GetDataSetFromBlock(
      compositeOutput, this->NumberOfGeometryParts);
    numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      numLines = numPts / 6;
      moreScalars = numPts % 6;

      scalars = vtkFloatArray::New();
      scalars->SetNumberOfTuples(numPts);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numPts * numberOfComponents);

      this->ReadNextDataLine(line);

      for (i = 0; i < numLines; i++)
      {
        sscanf(line, " %12e %12e %12e %12e %12e %12e", &scalarsRead[0],
          &scalarsRead[1], &scalarsRead[2], &scalarsRead[3],
          &scalarsRead[4], &scalarsRead[5]);
        for (j = 0; j < 6; j++)
        {
          scalars->InsertComponent(i*6 + j, component, scalarsRead[j]);
        }
        this->ReadNextDataLine(line);
      }
      strcpy(formatLine, "");
      strcpy(tempLine, "");
      for (j = 0; j < moreScalars; j++)
      {
        strcat(formatLine, " %12e");
        sscanf(line, formatLine, &scalarsRead[j]);
        scalars->InsertComponent(i*6 + j, component, scalarsRead[j]);
        strcat(tempLine, " %*12e");
        strcpy(formatLine, tempLine);
      }
      scalars->SetName(description);
      output->GetPointData()->AddArray(scalars);
      if (!output->GetPointData()->GetScalars())
      {
        output->GetPointData()-> SetScalars(scalars);
      }
      scalars->Delete();
    }
    delete this->IS;
    this->IS = NULL;
    return 1;
  }

  while (this->ReadNextDataLine(line) &&
    strncmp(line, "part",4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      this->ReadNextDataLine(line); // "coordinates" or "block"
      int partial = this->CheckForUndefOrPartial(line);
      if (component == 0)
      {
        scalars = vtkFloatArray::New();
        scalars->SetNumberOfTuples(numPts);
        scalars->SetNumberOfComponents(numberOfComponents);
        scalars->Allocate(numPts * numberOfComponents);
      }
      else
      {
        scalars = (vtkFloatArray*)(output->GetPointData()->
          GetArray(description));
      }

      // If the keyword 'partial' was found, we should replace unspecified
      // coordinate to take the value specified in the 'undef' field
      if( partial )
      {
        int l = 0;
        double val;
        for (i = 0; i < numPts; i++)
        {
          if( i == this->UndefPartial->PartialCoordinates[l] )
          {
            this->ReadNextDataLine(line);
            val = atof( line );
          }
          else
          {
            val = this->UndefPartial->UndefCoordinates;
            l++;
          }
          scalars->InsertComponent(i, component, val);
        }
      }
      else
      {
        for (i = 0; i < numPts; i++)
        {
          this->ReadNextDataLine(line);
          scalars->InsertComponent(i, component, atof(line));
        }
      }

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
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadVectorsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet *compositeOutput,
  int measured)
{
  char line[256], formatLine[256], tempLine[256];
  int partId, realId, numPts, i, j, numLines, moreVectors;
  vtkFloatArray *vectors;
  float vector1[3], vector2[3];
  vtkDataSet *output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL VectorPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length()-1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to vector per node file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {

      if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end()
        && this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
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
  }

  this->ReadNextDataLine(line); // skip the description line

  if (measured)
  {
    output = this->GetDataSetFromBlock(
      compositeOutput, this->NumberOfGeometryParts);
    numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      this->ReadNextDataLine(line);
      numLines = numPts / 2;
      moreVectors = ((numPts * 3) % 6) / 3;
      vectors = vtkFloatArray::New();
      vectors->SetNumberOfTuples(numPts);
      vectors->SetNumberOfComponents(3);
      vectors->Allocate(numPts*3);
      for (i = 0; i < numLines; i++)
      {
        sscanf(line, " %12e %12e %12e %12e %12e %12e", &vector1[0],
          &vector1[1], &vector1[2], &vector2[0], &vector2[1],
          &vector2[2]);
        vectors->InsertTuple(i*2, vector1);
        vectors->InsertTuple(i*2 + 1, vector2);
        this->ReadNextDataLine(line);
      }
      strcpy(formatLine, "");
      strcpy(tempLine, "");
      for (j = 0; j < moreVectors; j++)
      {
        strcat(formatLine, " %12e %12e %12e");
        sscanf(line, formatLine, &vector1[0], &vector1[1], &vector1[2]);
        vectors->InsertTuple(i*2 + j, vector1);
        strcat(tempLine, " %*12e %*12e %*12e");
        strcpy(formatLine, tempLine);
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
    this->IS = NULL;
    return 1;
  }

  while (this->ReadNextDataLine(line) &&
    strncmp(line, "part",4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      vectors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // "coordinates" or "block"
      vectors->SetNumberOfTuples(numPts);
      vectors->SetNumberOfComponents(3);
      vectors->Allocate(numPts*3);
      for (i = 0; i < 3; i++)
      {
        for (j = 0; j < numPts; j++)
        {
          this->ReadNextDataLine(line);
          vectors->InsertComponent(j, i, atof(line));
        }
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
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadTensorsPerNode(const char* fileName, const char* description,
  int timeStep, vtkMultiBlockDataSet *compositeOutput)
{
  char line[256];
  int symmTensorOrder[6] = {0, 1, 2, 3, 5, 4};
  int partId, realId, numPts, i, j;
  vtkFloatArray *tensors;
  vtkDataSet *output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL TensorPerNode variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length()-1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to tensor per node file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end()
        && this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
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
  }

  this->ReadNextDataLine(line); // skip the description line

  while (this->ReadNextDataLine(line) &&
    strncmp(line, "part",4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numPts = output->GetNumberOfPoints();
    if (numPts)
    {
      tensors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // "coordinates" or "block"
      tensors->SetNumberOfTuples(numPts);
      tensors->SetNumberOfComponents(6);
      tensors->Allocate(numPts*6);
      for (i = 0; i < 6; i++)
      {
        for (j = 0; j < numPts; j++)
        {
          this->ReadNextDataLine(line);
          tensors->InsertComponent(j, symmTensorOrder[i], atof(line));
        }
      }
      tensors->SetName(description);
      output->GetPointData()->AddArray(tensors);
      tensors->Delete();
    }
  }

  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadScalarsPerElement(const char* fileName,
  const char* description,
  int timeStep,
  vtkMultiBlockDataSet *compositeOutput,
  int numberOfComponents,
  int component)
{
  char line[256];
  int partId, realId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray *scalars;
  int lineRead, elementType;
  float scalar;
  vtkDataSet *output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL ScalarPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length()-1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to scalar per element file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    int j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end()
        && this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
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
  }

  this->ReadNextDataLine(line); // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part",4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = output->GetNumberOfCells();
    if (numCells)
    {
      this->ReadNextDataLine(line); // element type or "block"
      if (component == 0)
      {
        scalars = vtkFloatArray::New();
        scalars->SetNumberOfComponents(numberOfComponents);
        scalars->SetNumberOfTuples(numCells);
      }
      else
      {
        scalars =
          (vtkFloatArray*)(output->GetCellData()->GetArray(description));
      }

      // need to find out from CellIds how many cells we have of this element
      // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
      if (strncmp(line, "block",5) == 0)
      {
        for (i = 0; i < numCells; i++)
        {
          this->ReadNextDataLine(line);
          scalar = atof(line);
          scalars->InsertComponent(i, component, scalar);
        }
        lineRead = this->ReadNextDataLine(line);
      }
      else
      {
        while (lineRead && strncmp(line, "part",4) != 0 &&
          strncmp(line, "END TIME STEP", 13) != 0)
        {
          elementType = this->GetElementType(line);
          // Check if line contains either 'partial' or 'undef' keyword
          int partial = this->CheckForUndefOrPartial(line);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            delete this->IS;
            this->IS = NULL;
            if (component == 0)
            {
              scalars->Delete();
            }
            return 0;
          }
          idx = this->UnstructuredPartIds->IsId(realId);
          numCellsPerElement =
            this->GetCellIds(idx, elementType)->GetNumberOfIds();
          // If the 'partial' keyword was found, we should replace
          // unspecified coordinate with value specified in the 'undef' section
          if( partial )
          {
            int j = 0;
            for (i = 0; i < numCellsPerElement; i++)
            {
              if( i == this->UndefPartial->PartialElementTypes[j] )
              {
                this->ReadNextDataLine(line);
                scalar = atof(line);
              }
              else
              {
                scalar = this->UndefPartial->UndefElementTypes;
                j++; //go on to the next value in the partial list
              }
              scalars->InsertComponent( this->GetCellIds(idx,
                  elementType)->GetId(i), component, scalar);
            }
          }
          else
          {
            for (i = 0; i < numCellsPerElement; i++)
            {
              this->ReadNextDataLine(line);
              scalar = atof(line);
              scalars->InsertComponent( this->GetCellIds(idx,
                  elementType)->GetId(i), component, scalar);
            }
          }
          lineRead = this->ReadNextDataLine(line);
        } // end while
      } // end else
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
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadVectorsPerElement(const char* fileName,
  const char* description,
  int timeStep,
  vtkMultiBlockDataSet *compositeOutput)
{
  char line[256];
  int partId, realId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray *vectors;
  int lineRead, elementType;
  float value;
  vtkDataSet *output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL VectorPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length()-1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to vector per element file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end()
        && this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
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
  }

  this->ReadNextDataLine(line); // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part",4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = output->GetNumberOfCells();
    if (numCells)
    {
      vectors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // element type or "block"
      vectors->SetNumberOfTuples(numCells);
      vectors->SetNumberOfComponents(3);
      vectors->Allocate(numCells*3);

      // need to find out from CellIds how many cells we have of this element
      // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
      if (strncmp(line, "block",5) == 0)
      {
        for (i = 0; i < 3; i++)
        {
          for (j = 0; j < numCells; j++)
          {
            this->ReadNextDataLine(line);
            value = atof(line);
            vectors->InsertComponent(j, i, value);
          }
        }
        lineRead = this->ReadNextDataLine(line);
      }
      else
      {
        while (lineRead && strncmp(line, "part",4) != 0 &&
          strncmp(line, "END TIME STEP", 13) != 0)
        {
          elementType = this->GetElementType(line);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            delete this->IS;
            this->IS = NULL;
            vectors->Delete();
            return 0;
          }
          idx = this->UnstructuredPartIds->IsId(realId);
          numCellsPerElement =
            this->GetCellIds(idx, elementType)->GetNumberOfIds();
          for (i = 0; i < 3; i++)
          {
            for (j = 0; j < numCellsPerElement; j++)
            {
              this->ReadNextDataLine(line);
              value = atof(line);
              vectors->InsertComponent(this->GetCellIds(idx, elementType)->GetId(j),
                i, value);
            }
          }
          lineRead = this->ReadNextDataLine(line);
        } // end while
      } // end else
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
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadTensorsPerElement(const char* fileName,
  const char* description,
  int timeStep,
  vtkMultiBlockDataSet *compositeOutput)
{
  char line[256];
  int symmTensorOrder[6] = {0, 1, 2, 3, 5, 4};
  int partId, realId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray *tensors;
  int lineRead, elementType;
  float value;
  vtkDataSet *output;

  // Initialize
  //
  if (!fileName)
  {
    vtkErrorMacro("NULL TensorPerElement variable file name");
    return 0;
  }
  std::string sfilename;
  if (this->FilePath)
  {
    sfilename = this->FilePath;
    if (sfilename.at(sfilename.length()-1) != '/')
    {
      sfilename += "/";
    }
    sfilename += fileName;
    vtkDebugMacro("full path to tensor per element file: " << sfilename.c_str());
  }
  else
  {
    sfilename = fileName;
  }

  this->IS = new ifstream(sfilename.c_str(), ios::in);
  if (this->IS->fail())
  {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    delete this->IS;
    this->IS = NULL;
    return 0;
  }

  if (this->UseFileSets)
  {
    int realTimeStep = timeStep - 1;
    // Try to find the nearest time step for which we know the offset
    j = 0;
    for (i = realTimeStep; i >= 0; i--)
    {
      if (this->FileOffsets->Map.find(fileName) != this->FileOffsets->Map.end()
        && this->FileOffsets->Map[fileName].find(i) != this->FileOffsets->Map[fileName].end())
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
  }

  this->ReadNextDataLine(line); // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"

  while (lineRead && strncmp(line, "part",4) == 0)
  {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    realId = this->InsertNewPartId(partId);
    output = this->GetDataSetFromBlock(compositeOutput, realId);
    numCells = output->GetNumberOfCells();
    if (numCells)
    {
      tensors = vtkFloatArray::New();
      this->ReadNextDataLine(line); // element type or "block"
      tensors->SetNumberOfTuples(numCells);
      tensors->SetNumberOfComponents(6);
      tensors->Allocate(numCells*6);

      // need to find out from CellIds how many cells we have of this element
      // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
      if (strncmp(line, "block",5) == 0)
      {
        for (i = 0; i < 6; i++)
        {
          for (j = 0; j < numCells; j++)
          {
            this->ReadNextDataLine(line);
            value = atof(line);
            tensors->InsertComponent(j, symmTensorOrder[i], value);
          }
        }
        lineRead = this->ReadNextDataLine(line);
      }
      else
      {
        while (lineRead && strncmp(line, "part",4) != 0 &&
          strncmp(line, "END TIME STEP", 13) != 0)
        {
          elementType = this->GetElementType(line);
          if (elementType == -1)
          {
            vtkErrorMacro("Unknown element type \"" << line << "\"");
            delete [] this->IS;
            this->IS = NULL;
            tensors->Delete();
            return 0;
          }
          idx = this->UnstructuredPartIds->IsId(realId);
          numCellsPerElement =
            this->GetCellIds(idx, elementType)->GetNumberOfIds();
          for (i = 0; i < 6; i++)
          {
            for (j = 0; j < numCellsPerElement; j++)
            {
              this->ReadNextDataLine(line);
              value = atof(line);
              tensors->InsertComponent(this->GetCellIds(idx, elementType)->GetId(j),
                symmTensorOrder[i], value);
            }
          }
          lineRead = this->ReadNextDataLine(line);
        } // end while
      } // end else
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
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::CreateUnstructuredGridOutput(int partId,
  char line[256],
  const char* name,
  vtkMultiBlockDataSet *compositeOutput)
{
  int lineRead = 1;
  char subLine[256];
  int i, j, k;
  vtkIdType *nodeIds;
  int *intIds;
  int numElements;
  int idx, cellType;
  vtkIdType cellId;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == NULL || !ds->IsA("vtkUnstructuredGrid"))
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

  while(lineRead && strncmp(line, "part", 4) != 0)
  {
    if (strncmp(line, "coordinates", 11) == 0)
    {
      vtkDebugMacro("coordinates");
      int numPts;
      vtkPoints *points = vtkPoints::New();
      double point[3];

      this->ReadNextDataLine(line);
      numPts = atoi(line);
      vtkDebugMacro("num. points: " << numPts);

      points->Allocate(numPts);

      for (i = 0; i < numPts; i++)
      {
        this->ReadNextDataLine(line);
        points->InsertNextPoint(atof(line), 0, 0);
      }
      for (i = 0; i < numPts; i++)
      {
        this->ReadNextDataLine(line);
        points->GetPoint(i, point);
        points->SetPoint(i, point[0], atof(line), 0);
      }
      for (i = 0; i < numPts; i++)
      {
        this->ReadNextDataLine(line);
        points->GetPoint(i, point);
        points->SetPoint(i, point[0], point[1], atof(line));
      }

      lineRead = this->ReadNextDataLine(line);
      sscanf(line, " %s", subLine);

      char *endptr;
      // Testing if we can convert this string to double, ignore result
      double result = strtod(subLine, &endptr);
      static_cast<void>(result);

      if ( subLine != endptr )
      { // necessary if node ids were listed
        for (i = 0; i < numPts; i++)
        {
          points->GetPoint(i, point);
          points->SetPoint(i, point[1], point[2], atof(line));
          lineRead = this->ReadNextDataLine(line);
        }
      }
      output->SetPoints(points);
      points->Delete();
    }
    else if (strncmp(line, "point", 5) == 0)
    {
      int *elementIds;
      vtkDebugMacro("point");

      nodeIds = new vtkIdType[1];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      elementIds = new int[numElements];

      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
        elementIds[i] = atoi(line);
      }
      lineRead = this->ReadNextDataLine(line);
      sscanf(line, " %s", subLine);
      if (isdigit(subLine[0]))
      {
        for (i = 0; i < numElements; i++)
        {
          nodeIds[0] = atoi(line) - 1; // because EnSight ids start at 1
          cellId = output->InsertNextCell(VTK_VERTEX, 1, nodeIds);
          this->GetCellIds(idx, vtkEnSightReader::POINT)->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
        }
      }
      else
      {
        for (i = 0; i < numElements; i++)
        {
          nodeIds[0] = elementIds[i] - 1;
          cellId = output->InsertNextCell(VTK_VERTEX, 1, nodeIds);
          this->GetCellIds(idx, vtkEnSightReader::POINT)->InsertNextId(cellId);
        }
      }

      delete [] nodeIds;
      delete [] elementIds;
    }
    else if (strncmp(line, "g_point", 7) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_point");

      this->ReadNextDataLine(line);
      numElements = atoi(line);

      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
      }
      lineRead = this->ReadNextDataLine(line);
      sscanf(line, " %s", subLine);
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

      nodeIds = new vtkIdType[2];
      intIds = new int[2];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d", &intIds[0], &intIds[1]) != 2)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d", &intIds[0], &intIds[1]);
        for (j = 0; j < 2; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_LINE, 2, nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::BAR2)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "g_bar2", 6) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_bar2");

      intIds = new int[2];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d", &intIds[0], &intIds[1]) != 2)
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
      delete [] intIds;
    }
    else if (strncmp(line, "bar3", 4) == 0)
    {
      vtkDebugMacro("bar3");
      nodeIds = new vtkIdType[3];
      intIds = new int[3];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d", &intIds[0], &intIds[1], &intIds[2]) != 3)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d", &intIds[0], &intIds[1], &intIds[2]);
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
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "g_bar3", 6) == 0)
    {
      // skipping ghost cells
      vtkDebugMacro("g_bar3");
      intIds = new int[2];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %*d %d", &intIds[0], &intIds[1]) != 2)
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
      delete [] intIds;
    }
    else if (strncmp(line, "nsided", 6) == 0)
    {
      int *numNodesPerElement;
      int numNodes;
      std::stringstream* lineStream = new std::stringstream(std::stringstream::out);
      std::stringstream* formatStream = new std::stringstream(std::stringstream::out);
      std::stringstream* tempStream = new std::stringstream(std::stringstream::out);

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      if (this->ElementIdsListed)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }

      numNodesPerElement = new int[numElements];
      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
        numNodesPerElement[i] = atoi(line);
      }

      lineRead = this->ReadNextDataLine(line);
      for (i = 0; i < numElements; i++)
      {
        numNodes = numNodesPerElement[i];
        nodeIds = new vtkIdType[numNodes];
        intIds = new int[numNodesPerElement[i]];

        formatStream->str("");
        tempStream->str("");
        lineStream->str(line);
        lineStream->seekp(0, std::stringstream::end);
        while (! lineRead)
        {
          lineRead = this->ReadNextDataLine(line);
          lineStream->write(line, strlen(line));
          lineStream->seekp(0, std::stringstream::end);
        }
        for (j = 0; j < numNodes; j++)
        {
          formatStream->write(" %d", 3);
          formatStream->seekp(0, std::stringstream::end);
          sscanf(lineStream->str().c_str(), formatStream->str().c_str(), &intIds[numNodes-j-1]);
          tempStream->write(" %*d", 4);
          tempStream->seekp(0, std::stringstream::end);
          formatStream->str(tempStream->str());
          formatStream->seekp(0, std::stringstream::end);
          intIds[numNodes-j-1]--;
          nodeIds[numNodes-j-1] = intIds[numNodes-j-1];
        }
        cellId = output->InsertNextCell(VTK_POLYGON, numNodes, nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::NSIDED)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        delete [] nodeIds;
        delete [] intIds;
      }
      delete lineStream;
      delete formatStream;
      delete tempStream;
      delete [] numNodesPerElement;
    }
    else if (strncmp(line, "g_nsided", 8) == 0)
    {
      // skipping ghost cells
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      for (i = 0; i < numElements*2; i++)
      {
        this->ReadNextDataLine(line);
      }
      lineRead = this->ReadNextDataLine(line);
      if (lineRead)
      {
        sscanf(line, " %s", subLine);
      }
      if (lineRead && isdigit(subLine[0]))
      {
        // We still need to read in the node ids for each element.
        for (i = 0; i < numElements; i++)
        {
          lineRead = this->ReadNextDataLine(line);
        }
      }
    }
    else if (strncmp(line, "tria3", 5) == 0)
    {
      vtkDebugMacro("tria3");
      cellType = vtkEnSightReader::TRIA3;

      nodeIds = new vtkIdType[3];
      intIds = new int[3];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d", &intIds[0], &intIds[1],
          &intIds[2]) != 3)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d", &intIds[0], &intIds[1], &intIds[2]);
        for (j = 0; j < 3; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_TRIANGLE, 3, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "tria6", 5) == 0)
    {
      vtkDebugMacro("tria6");
      cellType = vtkEnSightReader::TRIA6;

      nodeIds = new vtkIdType[6];
      intIds = new int[6];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1],
          &intIds[2], &intIds[3], &intIds[4], &intIds[5]) != 6)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3], &intIds[4], &intIds[5]);
        for (j = 0; j < 6; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_TRIANGLE, 6, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "g_tria3", 7) == 0 ||
      strncmp(line, "g_tria6", 7) == 0)
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

      intIds = new int[3];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d", &intIds[0], &intIds[1],
          &intIds[2]) != 3)
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
      delete [] intIds;
    }
    else if (strncmp(line, "quad4", 5) == 0)
    {
      vtkDebugMacro("quad4");
      cellType = vtkEnSightReader::QUAD4;

      nodeIds = new vtkIdType[4];
      intIds = new int[4];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3]) != 4)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3]);
        for (j = 0; j < 4; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUAD, 4, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "quad8", 5) == 0)
    {
      vtkDebugMacro("quad8");
      cellType = vtkEnSightReader::QUAD8;

      nodeIds = new vtkIdType[8];
      intIds = new int[8];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d",
          &intIds[0], &intIds[1], &intIds[2], &intIds[3],
          &intIds[4], &intIds[5], &intIds[6], &intIds[7]) != 8)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d",
          &intIds[0], &intIds[1], &intIds[2], &intIds[3],
          &intIds[4], &intIds[5], &intIds[6], &intIds[7]);
        for (j = 0; j < 8; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_QUAD, 8, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "g_quad4", 7) == 0 ||
      strncmp(line, "g_quad8", 7) == 0)
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

      intIds = new int[4];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3]) != 4)
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
      delete [] intIds;
    }
    else if (strncmp(line, "nfaced", 6) == 0)
    {
      int *numFacesPerElement;
      int *numNodesPerFace;
      int *nodeMarker;
      int numPts = 0;
      int numFaces = 0;
      int numNodes = 0;
      int faceCount = 0;
      int elementNodeCount = 0;
      std::stringstream* lineStream = new std::stringstream(std::stringstream::out);
      std::stringstream* formatStream = new std::stringstream(std::stringstream::out);
      std::stringstream* tempStream = new std::stringstream(std::stringstream::out);

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      if (this->ElementIdsListed)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }

      numFacesPerElement = new int[numElements];
      for (i = 0; i < numElements; i++)
      {
        this->ReadNextDataLine(line);
        numFacesPerElement[i] = atoi(line);
        numFaces += numFacesPerElement[i];
      }

      numNodesPerFace = new int[numFaces];
      for (i = 0; i < numFaces; i++)
      {
        this->ReadNextDataLine(line);
        numNodesPerFace[i] = atoi(line);
      }

      numPts = output->GetNumberOfPoints();
      nodeMarker = new int[numPts];
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
        intIds = new int[numNodes];

        // Read element node ids
        elementNodeCount = 0;
        for (j = 0; j < numFacesPerElement[i]; j++)
        {
          formatStream->str("");
          tempStream->str("");
          lineStream->str(line);
          lineStream->seekp(0, std::stringstream::end);
          while (! lineRead)
          {
            lineRead = this->ReadNextDataLine(line);
            lineStream->write(line, strlen(line));
            lineStream->seekp(0, std::stringstream::end);
          }
          for (k = 0; k < numNodesPerFace[faceCount + j]; k++)
          {
            formatStream->write(" %d", 3);
            formatStream->seekp(0, std::stringstream::end);
            sscanf(lineStream->str().c_str(), formatStream->str().c_str(), &intIds[elementNodeCount]);
            tempStream->write(" %*d", 4);
            tempStream->seekp(0, std::stringstream::end);
            formatStream->str(tempStream->str());
            formatStream->seekp(0, std::stringstream::end);
            elementNodeCount += 1;
          }
          lineRead = this->ReadNextDataLine(line);
        }

        // prepare an array of Ids describing the vtkPolyhedron object yyy begin
        int         nodeIndx = 0; // indexing the raw array of point Ids
        int         arrayIdx = 0; // indexing the new array of Ids
        vtkIdType * theFaces = new vtkIdType // vtkPolyhedron's info of faces
                               [ elementNodeCount + numFacesPerElement[i] ];
        for ( j = 0; j < numFacesPerElement[i]; j ++ )
        {
          // number of points constituting this face
          theFaces[ arrayIdx ++ ] = numNodesPerFace[ faceCount + j ];

          for (  k = 0;  k < numNodesPerFace[ faceCount + j ];  k ++  )
          {
            // convert EnSight 1-based indexing to VTK 0-based indexing
            theFaces[ arrayIdx ++ ] = intIds[ nodeIndx ++ ] - 1;
          }
        }
        // yyy end

        faceCount += numFacesPerElement[i];

        // Build element
        nodeIds = new vtkIdType[numNodes];
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
        //cellId = output->InsertNextCell( VTK_CONVEX_POINT_SET,
        //                                 elementNodeCount, nodeIds );
        //xxx end

        // insert the cell as a vtkPolyhedron object yyy begin
        cellId = output->InsertNextCell( VTK_POLYHEDRON, elementNodeCount,
                                         nodeIds, numFacesPerElement[i],
                                         theFaces );
        delete [] theFaces;
        theFaces = NULL;
        // yyy end

        this->GetCellIds(idx, vtkEnSightReader::NFACED)->InsertNextId(cellId);
        delete [] nodeIds;
        delete [] intIds;
      }

      delete lineStream;
      delete formatStream;
      delete tempStream;

      delete [] nodeMarker;
      delete [] numNodesPerFace;
      delete [] numFacesPerElement;

    }
    else if (strncmp(line, "tetra4", 6) == 0)
    {
      vtkDebugMacro("tetra4");
      cellType = vtkEnSightReader::TETRA4;

      nodeIds = new vtkIdType[4];
      intIds = new int[4];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1],
          &intIds[2], &intIds[3]) != 4)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3]);
        for (j = 0; j < 4; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_TETRA, 4, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "tetra10", 7) == 0)
    {
      vtkDebugMacro("tetra10");
      cellType = vtkEnSightReader::TETRA10;

      nodeIds = new vtkIdType[10];
      intIds = new int[10];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d %d %d", &intIds[0],
          &intIds[1], &intIds[2], &intIds[3], &intIds[4],
          &intIds[5], &intIds[6], &intIds[7], &intIds[8],
          &intIds[9]) != 10)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d %d %d",
          &intIds[0], &intIds[1], &intIds[2], &intIds[3], &intIds[4],
          &intIds[5], &intIds[6], &intIds[7], &intIds[8], &intIds[9]);
        for (j = 0; j < 10; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_TETRA, 10, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "g_tetra4", 8) == 0 ||
      strncmp(line, "g_tetra10", 9) == 0)
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

      intIds = new int[4];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1],
          &intIds[2], &intIds[3]) != 4)
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
      delete [] intIds;
    }
    else if (strncmp(line, "pyramid5", 8) == 0)
    {
      vtkDebugMacro("pyramid5");
      cellType = vtkEnSightReader::PYRAMID5;

      nodeIds = new vtkIdType[5];
      intIds = new int[5];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3], &intIds[4]) != 5)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3], &intIds[4]);
        for (j = 0; j < 5; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_PYRAMID, 5, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "pyramid13", 9) == 0)
    {
      vtkDebugMacro("pyramid13");
      cellType = vtkEnSightReader::PYRAMID13;

      nodeIds = new vtkIdType[13];
      intIds = new int[13];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d", &intIds[0],
          &intIds[1], &intIds[2], &intIds[3], &intIds[4],
          &intIds[5], &intIds[6], &intIds[7], &intIds[8],
          &intIds[9], &intIds[10], &intIds[11], &intIds[12]) != 13)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d", &intIds[0],
          &intIds[1], &intIds[2], &intIds[3], &intIds[4],
          &intIds[5], &intIds[6], &intIds[7], &intIds[8],
          &intIds[9], &intIds[10], &intIds[11], &intIds[12]);
        for (j = 0; j < 13; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_PYRAMID, 13, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "g_pyramid5", 10) == 0 ||
      strncmp(line, "g_pyramid13", 11) == 0)
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

      intIds = new int[5];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3], &intIds[4]) != 5)
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
      delete [] intIds;
    }
    else if (strncmp(line, "hexa8", 5) == 0)
    {
      vtkDebugMacro("hexa8");
      cellType = vtkEnSightReader::HEXA8;

      nodeIds = new vtkIdType[8];
      intIds = new int[8];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1],
          &intIds[2], &intIds[3], &intIds[4], &intIds[5], &intIds[6],
          &intIds[7]) != 8)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1],
          &intIds[2], &intIds[3], &intIds[4], &intIds[5], &intIds[6],
          &intIds[7]);
        for (j = 0; j < 8; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_HEXAHEDRON, 8, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "hexa20", 6) == 0)
    {
      vtkDebugMacro("hexa20");
      cellType = vtkEnSightReader::HEXA20;

      nodeIds = new vtkIdType[20];
      intIds = new int[20];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
          &intIds[0], &intIds[1], &intIds[2], &intIds[3], &intIds[4],
          &intIds[5], &intIds[6], &intIds[7], &intIds[8], &intIds[9],
          &intIds[10], &intIds[11], &intIds[12], &intIds[13],
          &intIds[14], &intIds[15], &intIds[16], &intIds[17],
          &intIds[18], &intIds[19]) != 20)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
          &intIds[0], &intIds[1], &intIds[2], &intIds[3], &intIds[4],
          &intIds[5], &intIds[6], &intIds[7], &intIds[8], &intIds[9],
          &intIds[10], &intIds[11], &intIds[12], &intIds[13], &intIds[14],
          &intIds[15], &intIds[16], &intIds[17], &intIds[18], &intIds[19]);
        for (j = 0; j < 20; j++)
        {
          intIds[j]--;
          nodeIds[j] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_HEXAHEDRON, 20, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "g_hexa8", 7) == 0 ||
      strncmp(line, "g_hexa20", 8) == 0)
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

      intIds = new int[8];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1],
          &intIds[2], &intIds[3], &intIds[4], &intIds[5], &intIds[6],
          &intIds[7]) != 8)
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
      delete [] intIds;
    }
    else if (strncmp(line, "penta6", 6) == 0)
    {
      const unsigned char wedgeMap[6] = {0, 2, 1, 3, 5, 4};

      vtkDebugMacro("penta6");
      cellType = vtkEnSightReader::PENTA6;

      nodeIds = new vtkIdType[6];
      intIds = new int[6];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1],
          &intIds[2], &intIds[3], &intIds[4], &intIds[5]) != 6)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
          &intIds[3], &intIds[4], &intIds[5]);
        for (j = 0; j < 6; j++)
        {
          intIds[j]--;
          nodeIds[wedgeMap[j]] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_WEDGE, 6, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "penta15", 7) == 0)
    {
      const unsigned char wedgeMap[15] = {0, 2, 1, 3, 5, 4, 8, 7, 6, 11, 10, 9, 12, 14, 13};

      vtkDebugMacro("penta15");
      cellType = vtkEnSightReader::PENTA15;

      nodeIds = new vtkIdType[15];
      intIds = new int[15];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
          &intIds[0], &intIds[1], &intIds[2], &intIds[3], &intIds[4],
          &intIds[5], &intIds[6], &intIds[7], &intIds[8], &intIds[9],
          &intIds[10], &intIds[11], &intIds[12], &intIds[13],
          &intIds[14]) != 15)
      {
        for (i = 0; i < numElements; i++)
        {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
        }
      }
      for (i = 0; i < numElements; i++)
      {
        sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
          &intIds[0], &intIds[1], &intIds[2], &intIds[3], &intIds[4],
          &intIds[5], &intIds[6], &intIds[7], &intIds[8], &intIds[9],
          &intIds[10], &intIds[11], &intIds[12], &intIds[13], &intIds[14]);
        for (j = 0; j < 15; j++)
        {
          intIds[j]--;
          nodeIds[wedgeMap[j]] = intIds[j];
        }
        cellId = output->InsertNextCell(VTK_QUADRATIC_WEDGE, 15, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
      }
      delete [] nodeIds;
      delete [] intIds;
    }
    else if (strncmp(line, "g_penta6", 8) == 0 ||
      strncmp(line, "g_penta15", 9) == 0)
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

      intIds = new int[6];

      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1],
          &intIds[2], &intIds[3], &intIds[4], &intIds[5]) != 6)
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
      delete [] intIds;
    }
    else if (strncmp(line, "END TIME STEP", 13) == 0)
    {
      return 1;
    }
    else if (this->IS->fail())
    {
      //May want consistency check here?
      //vtkWarningMacro("EOF on geometry file");
      return 1;
    }
    else
    {
      vtkErrorMacro("undefined geometry file line");
      return -1;
    }
  }
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::CreateStructuredGridOutput(int partId,
  char line[256],
  const char* name,
  vtkMultiBlockDataSet *compositeOutput)
{
  char subLine[256];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkPoints *points = vtkPoints::New();
  double point[3];
  int numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == NULL || !ds->IsA("vtkStructuredGrid"))
  {
    vtkDebugMacro("creating new structured grid output");
    vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
    this->AddToBlock(compositeOutput, partId, sgrid);
    sgrid->Delete();
    ds = sgrid;
  }

  vtkStructuredGrid* output = vtkStructuredGrid::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  if (sscanf(line, " %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "iblanked",8) == 0)
    {
      iblanked = 1;
    }
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &dimensions[0], &dimensions[1], &dimensions[2]);
  output->SetDimensions(dimensions);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  points->Allocate(numPts);

  for (i = 0; i < numPts; i++)
  {
    this->ReadNextDataLine(line);
    points->InsertNextPoint(atof(line), 0.0, 0.0);
  }
  for (i = 0; i < numPts; i++)
  {
    this->ReadNextDataLine(line);
    points->GetPoint(i, point);
    points->SetPoint(i, point[0], atof(line), point[2]);
  }
  for (i = 0; i < numPts; i++)
  {
    this->ReadNextDataLine(line);
    points->GetPoint(i, point);
    points->SetPoint(i, point[0], point[1], atof(line));
  }
  output->SetPoints(points);
  if (iblanked)
  {
    for (i = 0; i < numPts; i++)
    {
      this->ReadNextDataLine(line);
      if (!atoi(line))
      {
        output->BlankPoint(i);
      }
    }
  }

  points->Delete();
  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::CreateRectilinearGridOutput(int partId,
  char line[256],
  const char* name,
  vtkMultiBlockDataSet *compositeOutput)
{
  char subLine[256];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkFloatArray *xCoords = vtkFloatArray::New();
  vtkFloatArray *yCoords = vtkFloatArray::New();
  vtkFloatArray *zCoords = vtkFloatArray::New();
  int numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == NULL || !ds->IsA("vtkRectilinearGrid"))
  {
    vtkDebugMacro("creating new structured grid output");
    vtkRectilinearGrid* rgrid = vtkRectilinearGrid::New();
    this->AddToBlock(compositeOutput, partId, rgrid);
    rgrid->Delete();

    ds = rgrid;
  }

  vtkRectilinearGrid* output = vtkRectilinearGrid::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  if (sscanf(line, " %*s %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "iblanked",8) == 0)
    {
      iblanked = 1;
    }
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &dimensions[0], &dimensions[1], &dimensions[2]);
  output->SetDimensions(dimensions);
  xCoords->Allocate(dimensions[0]);
  yCoords->Allocate(dimensions[1]);
  zCoords->Allocate(dimensions[2]);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];

  float val;

  for (i = 0; i < dimensions[0]; i++)
  {
    this->ReadNextDataLine(line);
    val = atof(line);
    xCoords->InsertNextTuple(&val);
  }
  for (i = 0; i < dimensions[1]; i++)
  {
    this->ReadNextDataLine(line);
    val = atof(line);
    yCoords->InsertNextTuple(&val);
  }
  for (i = 0; i < dimensions[2]; i++)
  {
    this->ReadNextDataLine(line);
    val = atof(line);
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

  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::CreateImageDataOutput(int partId,
  char line[256],
  const char* name,
  vtkMultiBlockDataSet *compositeOutput)
{
  char subLine[256];
  int lineRead;
  int iblanked = 0;
  int dimensions[3];
  int i;
  float origin[3], delta[3];
  int numPts;

  this->NumberOfNewOutputs++;

  vtkDataSet* ds = this->GetDataSetFromBlock(compositeOutput, partId);
  if (ds == NULL || !ds->IsA("vtkImageData"))
  {
    vtkDebugMacro("creating new image data output");
    vtkImageData* idata = vtkImageData::New();
    this->AddToBlock(compositeOutput, partId, idata);
    idata->Delete();

    ds = idata;
  }

  vtkImageData* output = vtkImageData::SafeDownCast(ds);

  this->SetBlockName(compositeOutput, partId, name);

  if (sscanf(line, " %*s %*s %s", subLine) == 1)
  {
    if (strncmp(subLine, "iblanked",8) == 0)
    {
      iblanked = 1;
    }
  }

  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &dimensions[0], &dimensions[1], &dimensions[2]);
  output->SetDimensions(dimensions);

  for (i = 0; i < 3; i++)
  {
    this->ReadNextDataLine(line);
    sscanf(line, " %f", &origin[i]);
  }
  output->SetOrigin(origin[0], origin[1], origin[2]);

  for (i = 0; i < 3; i++)
  {
    this->ReadNextDataLine(line);
    sscanf(line, " %f", &delta[i]);
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

  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::CheckForUndefOrPartial(const char *line)
{
  char undefvar[16];
  // Look for keyword 'partial' or 'undef':
  int r = sscanf(line, "%*s %15s", undefvar);
  if( r == 1)
  {
    char subline[80];
    if( strcmp(undefvar, "undef") == 0 )
    {
      vtkDebugMacro( "undef: " << line );
      this->ReadNextDataLine(subline);
      double val = atof( subline );
      switch( this->GetSectionType(line) )
      {
      case vtkEnSightReader::COORDINATES:
        this->UndefPartial->UndefCoordinates = val;
        break;
      case vtkEnSightReader::BLOCK:
        this->UndefPartial->UndefBlock = val;
        break;
      case vtkEnSightReader::ELEMENT:
        this->UndefPartial->UndefElementTypes = val;
        break;
      default:
        vtkErrorMacro( << "Unknow section type: " << subline );
      }
      return 0; //meaning 'undef', so no other steps is necesserary
    }
    else if ( strcmp(undefvar, "partial") == 0 )
    {
      vtkDebugMacro( "partial: " << line );
      this->ReadNextDataLine( subline );
      int nLines = atoi(subline);
      vtkIdType val;
      int i;
      switch( this->GetSectionType(line) )
      {
      case vtkEnSightReader::COORDINATES:
        for(i=0; i<nLines; ++i)
        {
          this->ReadNextDataLine(subline);
          val = atoi(subline) - 1; //EnSight start at 1
          this->UndefPartial->PartialCoordinates.push_back(val);
        }
        break;
      case vtkEnSightReader::BLOCK:
        for(i=0; i<nLines; ++i)
        {
          this->ReadNextDataLine(subline);
          val = atoi(subline) - 1; //EnSight start at 1
          this->UndefPartial->PartialBlock.push_back(val);
        }
        break;
      case vtkEnSightReader::ELEMENT:
        for(i=0; i<nLines; ++i)
        {
          this->ReadNextDataLine(subline);
          val = atoi(subline) - 1; //EnSight start at 1
          this->UndefPartial->PartialElementTypes.push_back(val);
        }
        break;
      default:
        vtkErrorMacro( << "Unknow section type: " << subline );
      }
      return 1; //meaning 'partial', so other steps are necesserary
    }
    else
    {
      vtkErrorMacro( << "Unknown value for undef or partial: " << undefvar );
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkEnSightGoldReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
