/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSight6BinaryReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEnSight6BinaryReader.h"

#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"

#include <sys/stat.h>
#include <ctype.h>
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkEnSight6BinaryReader, "1.36");
vtkStandardNewMacro(vtkEnSight6BinaryReader);

//----------------------------------------------------------------------------
vtkEnSight6BinaryReader::vtkEnSight6BinaryReader()
{
  this->NumberOfUnstructuredPoints = 0;
  this->UnstructuredPoints = vtkPoints::New();
  this->UnstructuredNodeIds = NULL;

  this->IFile = NULL;

  this->FileSize = 0;
}

//----------------------------------------------------------------------------
vtkEnSight6BinaryReader::~vtkEnSight6BinaryReader()
{
  if (this->UnstructuredNodeIds)
    {
    this->UnstructuredNodeIds->Delete();
    this->UnstructuredNodeIds = NULL;
    }
  this->UnstructuredPoints->Delete();
  this->UnstructuredPoints = NULL;
  
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::OpenFile(const char* filename)
{
  if (!filename)
    {
    vtkErrorMacro(<<"Missing filename.");
    return 0;
    }

  // Close file from any previous image
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
  
  // Open the new file
  vtkDebugMacro(<< "Opening file " << filename);
  struct stat fs;
  if ( !stat( filename, &fs) )
    {
    // Find out how big the file is.
    this->FileSize = (int)(fs.st_size);

#ifdef _WIN32
    this->IFile = new ifstream(filename, ios::in | ios::binary);
#else
    this->IFile = new ifstream(filename, ios::in);
#endif
    }
  else
    {
    vtkErrorMacro("stat failed.");
    return 0;
    }
  if (! this->IFile || this->IFile->fail())
    {
    vtkErrorMacro(<< "Could not open file " << filename); 
    return 0;
    }
  return 1;
}



//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::ReadGeometryFile(char* fileName, int timeStep)
{
  char line[80], subLine[80];
  int partId;
  int lineRead;
  float *coordinateArray;
  int i;
  int pointIdsListed;
  int *pointIds;
  
  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("A GeometryFileName must be specified in the case file.");
    return 0;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    sfilename += fileName;
    vtkDebugMacro("full path to geometry file: " << sfilename.c_str());
    }
  else
    {
    sfilename = fileName;
    }
  
  if (this->OpenFile(sfilename.c_str()) == 0)
    {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
    }
  
  lineRead = this->ReadLine(line);
  sscanf(line, " %*s %s", subLine);
  if (strcmp(subLine, "Binary") != 0 &&
      strcmp(subLine, "binary") != 0)
    {
    vtkErrorMacro("This is not an EnSight6 binary file. Try "
                  << "vtkEnSight6Reader.");
    return 0;
    }
  
  if (this->UseFileSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      this->SkipTimeStep();
      }
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0 && lineRead)
      {
      lineRead = this->ReadLine(line);
      }
    }

  // Skip the 2 description lines.  Using ReadLine instead of ReadLine
  // because the description line could be blank.
  this->ReadLine(line);
  this->ReadLine(line);
  
  // Read the node id and element id lines.
  this->ReadLine(line); // node id *
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0)
    {
    this->UnstructuredNodeIds = vtkIdTypeArray::New();
    pointIdsListed = 1;
    }
  else if (strcmp(subLine, "ignore") == 0)
    {
    pointIdsListed = 1;
    }
  else
    {
    pointIdsListed = 0;
    }
  
  this->ReadLine(line); // element id *
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0 || strcmp(subLine, "ignore") == 0)
    {
    this->ElementIdsListed = 1;
    }
  else
    {
    this->ElementIdsListed = 0;
    }
  
  this->ReadLine(line); // "coordinates"
  this->ReadIntNumber(&this->NumberOfUnstructuredPoints); // number of points
  
  this->UnstructuredPoints->SetNumberOfPoints(
                               this->NumberOfUnstructuredPoints);
  
  if (pointIdsListed)
    {
    pointIds = new int[this->NumberOfUnstructuredPoints];
    this->ReadIntArray(pointIds, this->NumberOfUnstructuredPoints);

    if (this->UnstructuredNodeIds)
      {
      int maxId = 0;
      for (i = 0; i < this->NumberOfUnstructuredPoints; i++)
        {
        if (pointIds[i] > maxId)
          {
          maxId = pointIds[i];
          }
        }
      
      this->UnstructuredNodeIds->Allocate(maxId);
      this->UnstructuredNodeIds->FillComponent(0, -1);

      for (i = 0; i < this->NumberOfUnstructuredPoints; i++)
        {
        this->UnstructuredNodeIds->InsertValue(pointIds[i]-1, i);
        }
      }
    delete [] pointIds;
    }
  
  coordinateArray = (float*)(this->UnstructuredPoints->GetVoidPointer(0));
  this->ReadFloatArray(coordinateArray, this->NumberOfUnstructuredPoints * 3);
  
  lineRead = this->ReadLine(line); // "part"
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing at 1.
    
    this->ReadLine(line); // part description line
    char *name = strdup(line);
    lineRead = this->ReadLine(line);
    
    if (strncmp(line, "block", 5) == 0)
      {
      lineRead = this->CreateStructuredGridOutput(partId, line, name);
      }
    else
      {
      lineRead = this->CreateUnstructuredGridOutput(partId, line, name);
      }
    free(name);
    }
  
  // Close file from any previous image
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkEnSight6BinaryReader::SkipTimeStep()
{
  char line[80], subLine[80];
  int lineRead;
  int pointIdsListed;

  this->ReadLine(line);
  while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
    this->ReadLine(line);
    }
  
  // Skip the 2 description lines.
  this->ReadLine(line);
  this->ReadLine(line);
  
  // Read the node id and element id lines.
  this->ReadLine(line); // node id *
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0 ||
      strcmp(subLine, "ignore") == 0)
    {
    pointIdsListed = 1;
    }
  else
    {
    pointIdsListed = 0;
    }
  
  this->ReadLine(line); // element id *
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0 || strcmp(subLine, "ignore") == 0)
    {
    this->ElementIdsListed = 1;
    }
  else
    {
    this->ElementIdsListed = 0;
    }
  
  this->ReadLine(line); // "coordinates"
  this->ReadIntNumber(&this->NumberOfUnstructuredPoints); // number of points
  
  if (pointIdsListed)
    { // skip point ids.
    this->IFile->seekg((sizeof(int)*this->NumberOfUnstructuredPoints), ios::cur);
    }
  
  this->IFile->seekg((sizeof(float)*3*this->NumberOfUnstructuredPoints), ios::cur);
  
  lineRead = this->ReadLine(line); // "part"
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    this->ReadLine(line); // part description line
    lineRead = this->ReadLine(line);
    
    if (strncmp(line, "block", 5) == 0)
      {
      lineRead = this->SkipStructuredGrid(line);
      }
    else
      {
      lineRead = this->SkipUnstructuredGrid(line);
      }
    }
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::SkipStructuredGrid(char line[256])
{
  char subLine[80];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int numPts;

  if (sscanf(line, " %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "iblanked") == 0)
      {
      iblanked = 1;
      }
    }

  // Read these separately to get byte order set.
  this->ReadIntNumber(dimensions);
  this->ReadIntNumber(dimensions+1);
  this->ReadIntNumber(dimensions+2);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  
  // Skip coordinates.
  this->IFile->seekg((sizeof(float)*3*numPts), ios::cur);
  
  if (iblanked)
    { // skip blanking array.
    this->IFile->seekg((sizeof(int)*numPts), ios::cur);
    }
  
  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::SkipUnstructuredGrid(char line[256])
{
  int lineRead = 1;
  int numElements;
  int cellType;
  
  while(lineRead && strncmp(line, "part", 4) != 0)
    {
    if (strncmp(line, "point", 5) == 0)
      {
      vtkDebugMacro("point");
      
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        { // skip element ids.
        this->IFile->seekg((sizeof(int)*numElements), ios::cur);
        }
      
      // nodeIdList
      this->IFile->seekg((sizeof(int)*numElements), ios::cur);
      }
    else if (strncmp(line, "bar2", 4) == 0)
      {
      vtkDebugMacro("bar2");
      
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        { // skip element ids.
        this->IFile->seekg((sizeof(int)*numElements), ios::cur);
        }
      
      // Skip node ids.
      this->IFile->seekg((sizeof(int)*2*numElements), ios::cur);
      }
    else if (strncmp(line, "bar3", 4) == 0)
      {
      vtkDebugMacro("bar3");
      vtkWarningMacro("Only vertex nodes of this element will be read.");
      
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->seekg((sizeof(int)*numElements), ios::cur);
        }
      
      // Skip node ids.      
      this->IFile->seekg((sizeof(int)*3*numElements), ios::cur);
      }
    else if (strncmp(line, "tria3", 5) == 0 ||
             strncmp(line, "tria6", 5) == 0)
      {
      if (strncmp(line, "tria3", 5) == 0)
        {
        vtkDebugMacro("tria3");
        cellType = vtkEnSightReader::TRIA3;
        }
      else
        {
        vtkDebugMacro("tria6");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::TRIA6;
        }
      
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->seekg((sizeof(int)*numElements), ios::cur);
        }
      
      if (cellType == vtkEnSightReader::TRIA3)
        {
        this->IFile->seekg((sizeof(int)*3*numElements), ios::cur);
        }
      else
        {
        this->IFile->seekg((sizeof(int)*6*numElements), ios::cur);
        }      
      }
    else if (strncmp(line, "quad4", 5) == 0 ||
             strncmp(line, "quad8", 5) == 0)
      {
      if (strncmp(line, "quad8", 5) == 0)
        {
        vtkDebugMacro("quad8");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::QUAD8;
        }
      else
        {
        vtkDebugMacro("quad4");
        cellType = vtkEnSightReader::QUAD4;
        }
      
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->seekg((sizeof(int)*numElements), ios::cur);
        }
      
      if (cellType == vtkEnSightReader::QUAD4)
        {
        this->IFile->seekg((sizeof(int)*4*numElements), ios::cur);
        }
      else
        {
        this->IFile->seekg((sizeof(int)*8*numElements), ios::cur);
        }
      }
    else if (strncmp(line, "tetra4", 6) == 0 ||
             strncmp(line, "tetra10", 7) == 0)
      {
      if (strncmp(line, "tetra10", 7) == 0)
        {
        vtkDebugMacro("tetra10");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::TETRA10;
        }
      else
        {
        vtkDebugMacro("tetra4");
        cellType = vtkEnSightReader::TETRA4;
        }
      
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->seekg((sizeof(int)*numElements), ios::cur);
        }
      
      if (cellType == vtkEnSightReader::TETRA4)
        {
        this->IFile->seekg((sizeof(int)*4*numElements), ios::cur);
        }
      else
        {
        this->IFile->seekg((sizeof(int)*10*numElements), ios::cur);
        }
      }
    else if (strncmp(line, "pyramid5", 8) == 0 ||
             strncmp(line, "pyramid13", 9) == 0)
      {
      if (strncmp(line, "pyramid13", 9) == 0)
        {
        vtkDebugMacro("pyramid13");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::PYRAMID13;
        }
      else
        {
        vtkDebugMacro("pyramid5");
        cellType = vtkEnSightReader::PYRAMID5;
        }

      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->seekg((sizeof(int)*numElements), ios::cur);
        }
      
      if (cellType == vtkEnSightReader::PYRAMID5)
        {
        this->IFile->seekg((sizeof(int)*5*numElements), ios::cur);
        }
      else
        {
        this->IFile->seekg((sizeof(int)*13*numElements), ios::cur);
        }
      }
    else if (strncmp(line, "hexa8", 5) == 0 ||
             strncmp(line, "hexa20", 6) == 0)
      {
      if (strncmp(line, "hexa20", 6) == 0)
        {
        vtkDebugMacro("hexa20");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::HEXA20;
        }
      else
        {
        vtkDebugMacro("hexa8");
        cellType = vtkEnSightReader::HEXA8;
        }
      
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->seekg((sizeof(int)*numElements), ios::cur);
        }
      
      if (cellType == vtkEnSightReader::HEXA8)
        {
        this->IFile->seekg((sizeof(int)*8*numElements), ios::cur);
        }
      else
        {
        this->IFile->seekg((sizeof(int)*20*numElements), ios::cur);
        }
      }
    else if (strncmp(line, "penta6", 6) == 0 ||
             strncmp(line, "penta15", 7) == 0)
      {
      if (strncmp(line, "penta15", 7) == 0)
        {
        vtkDebugMacro("penta15");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::PENTA15;
        }
      else
        {
        vtkDebugMacro("penta6");
        cellType = vtkEnSightReader::PENTA6;
        }
      
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->seekg((sizeof(int)*numElements), ios::cur);
        }
      
      if (cellType == vtkEnSightReader::PENTA6)
        {
        this->IFile->seekg((sizeof(int)*6*numElements), ios::cur);
        }
      else
        {
        this->IFile->seekg((sizeof(int)*15*numElements), ios::cur);
        }
      }
    else if (strncmp(line, "END TIME STEP", 13) == 0)
      {
      break;
      }
    lineRead = this->ReadLine(line);
    }
  
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::ReadMeasuredGeometryFile(char* fileName,
                                                      int timeStep)
{
  char line[80], subLine[80];
  int i;
  int *pointIds;
  float *xCoords, *yCoords, *zCoords;
  vtkPoints *points = vtkPoints::New();
  vtkPolyData *pd = vtkPolyData::New();
  
  this->NumberOfNewOutputs++;

  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("A MeasuredFileName must be specified in the case file.");
    return 0;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    sfilename += fileName;
    vtkDebugMacro("full path to measured geometry file: " 
                  << sfilename.c_str());
    }
  else
    {
    sfilename = fileName;
    }
  
  if (this->OpenFile(sfilename.c_str()) == 0)
    {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
    }

  if (this->GetOutput(this->NumberOfGeometryParts) &&
      ! this->GetOutput(this->NumberOfGeometryParts)->IsA("vtkPolyData"))
    {
    vtkErrorMacro("Cannot change type of output");
    this->OutputsAreValid = 0;
    return 0;
    }
  
  this->ReadLine(line);
  sscanf(line, " %*s %s", subLine);
  if (strcmp(subLine, "Binary") != 0)
    {
    vtkErrorMacro("This is not a binary data set. Try "
                  << "vtkEnSightGoldReader.");
    return 0;
    }
  
  if (this->UseFileSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
        {
        this->ReadLine(line);
        }
      
      // Skip the description line.
      this->ReadLine(line);
      
      this->ReadLine(line); // "particle coordinates"
      
      this->ReadIntNumber(&this->NumberOfMeasuredPoints);
      
      pointIds = new int[this->NumberOfMeasuredPoints];
      xCoords = new float [this->NumberOfMeasuredPoints];
      yCoords = new float [this->NumberOfMeasuredPoints];
      zCoords = new float [this->NumberOfMeasuredPoints];
      
      this->ReadIntArray(pointIds, this->NumberOfMeasuredPoints);
      this->ReadFloatArray(xCoords, this->NumberOfMeasuredPoints);
      this->ReadFloatArray(yCoords, this->NumberOfMeasuredPoints);
      this->ReadFloatArray(zCoords, this->NumberOfMeasuredPoints);
      
      delete [] pointIds;
      delete [] xCoords;
      delete [] yCoords;
      delete [] zCoords;
      
      this->ReadLine(line); // END TIME STEP
      }
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  // Skip the description line.
  this->ReadLine(line);

  this->ReadLine(line); // "particle coordinates"
  
  this->ReadIntNumber(&this->NumberOfMeasuredPoints);
  
  this->MeasuredNodeIds->Allocate(this->NumberOfMeasuredPoints);

  pointIds = new int[this->NumberOfMeasuredPoints];
  xCoords = new float [this->NumberOfMeasuredPoints];
  yCoords = new float [this->NumberOfMeasuredPoints];
  zCoords = new float [this->NumberOfMeasuredPoints];
  points->Allocate(this->NumberOfMeasuredPoints);
  pd->Allocate(this->NumberOfMeasuredPoints);
  
  this->ReadIntArray(pointIds, this->NumberOfMeasuredPoints);
  this->ReadFloatArray(xCoords, this->NumberOfMeasuredPoints);
  this->ReadFloatArray(yCoords, this->NumberOfMeasuredPoints);
  this->ReadFloatArray(zCoords, this->NumberOfMeasuredPoints);
  
  for (i = 0; i < this->NumberOfMeasuredPoints; i++)
    {
    this->MeasuredNodeIds->InsertNextId(pointIds[i]);
    points->InsertNextPoint(xCoords[i], yCoords[i], zCoords[i]);
    pd->InsertNextCell(VTK_VERTEX, 1, (vtkIdType*)&pointIds[i]);
    }

  pd->SetPoints(points);
  this->SetNthOutput(this->NumberOfGeometryParts, pd);
  
  points->Delete();
  pd->Delete();
  delete [] pointIds;
  delete [] xCoords;
  delete [] yCoords;
  delete [] zCoords;
  
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::ReadScalarsPerNode(char* fileName,
                                                char* description,
                                                int timeStep, int measured,
                                                int numberOfComponents,
                                                int component)
{
  char line[80];
  int partId, numPts, numParts, i;
  vtkFloatArray *scalars = NULL;
  float* scalarsRead;
  long pos;
  vtkDataSet *output;
  int lineRead;
  
  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("NULL ScalarPerNode variable file name");
    return 0;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    sfilename += fileName;
    vtkDebugMacro("full path to scalar per node file: " << sfilename.c_str());
    }
  else
    {
    sfilename = fileName;
    }
  
  if (this->OpenFile(sfilename.c_str()) == 0)
    {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
    }

  if (this->UseFileSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
        {
        this->ReadLine(line);
        }
      this->ReadLine(line); // skip the description line
      
      pos = this->IFile->tellg();
      this->ReadLine(line); // 1st data line or part #
      if (strncmp(line, "part", 4) != 0)
        {
        this->IFile->seekg(pos, ios::beg);
        if (!measured)
          {
          numPts = this->UnstructuredPoints->GetNumberOfPoints();
          }
        else
          {
          numPts = this->GetOutput(this->NumberOfGeometryParts)->
            GetNumberOfPoints();
          }

        // Here I am assuming that we are skiping over data
        // we do not need to read.
        //scalarsRead = new float[numPts];
        //this->ReadFloatArray(scalarsRead, numPts);
        //delete [] scalarsRead;
        this->IFile->seekg((long)(numPts)*(long)(sizeof(float)), ios::cur);
        if (this->IFile->fail())
          {
          vtkErrorMacro("File seek failed.");
          }
        }
      
      // scalars for structured parts
      while (this->ReadLine(line) && strncmp(line, "part", 4) == 0)
        {
        sscanf(line, " part %d", &partId);
        partId--;
        this->ReadLine(line); // block
        numPts = this->GetOutput(partId)->GetNumberOfPoints();

        // Here I am assuming that we are skiping over data
        // we do not need to read.
        //scalarsRead = new float[numPts];
        //this->ReadFloatArray(scalarsRead, numPts);
        //delete [] scalarsRead;
        this->IFile->seekg((long)(numPts)*(long)(sizeof(float)), ios::cur);
        if (this->IFile->fail())
          {
          vtkErrorMacro("File seek failed.");
          }
        //scalarsRead = new float[numPts];
        //this->ReadFloatArray(scalarsRead, numPts);
        //delete [] scalarsRead;
        }
      }
    lineRead = this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0 && lineRead)
      {
      lineRead = this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line

  pos = this->IFile->tellg();
  lineRead = this->ReadLine(line); // 1st data line or part #
  if (strncmp(line, "part", 4) != 0)
    {
    this->IFile->seekg(pos, ios::beg);
    if (!measured)
      {
      numPts = this->UnstructuredPoints->GetNumberOfPoints();
      }
    else
      {
      numPts = this->GetOutput(this->NumberOfGeometryParts)->
        GetNumberOfPoints();
      }
    if (component == 0)
      {
      scalars = vtkFloatArray::New();
      scalars->SetNumberOfTuples(numPts);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numPts * numberOfComponents);
      }
    else
      {
      partId = this->UnstructuredPartIds->GetId(0);
      scalars = (vtkFloatArray*)(this->GetOutput(partId)->GetPointData()->
                                 GetArray(description));
      }
    scalarsRead = new float[numPts];
    this->ReadFloatArray(scalarsRead, numPts);
    for (i = 0; i < numPts; i++)
      {
      scalars->InsertComponent(i, component, scalarsRead[i]);
      }
    
    if (!measured)
      {
      numParts = this->UnstructuredPartIds->GetNumberOfIds();
      for (i = 0; i < numParts; i++)
        {
        partId = this->UnstructuredPartIds->GetId(i);
        output = this->GetOutput(partId);
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
        }
      }
    else
      {
      scalars->SetName(description);
      output = this->GetOutput(this->NumberOfGeometryParts);
      output->GetPointData()->AddArray(scalars);
      if (!output->GetPointData()->GetScalars())
        {
        output->GetPointData()->SetScalars(scalars);
        }
      }
    delete [] scalarsRead;
    if (component == 0)
      {
      scalars->Delete();
      scalars = NULL;
      }
    }

  // scalars for structured parts
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    sscanf(line, " part %d", &partId);
    partId--;
    output = this->GetOutput(partId);
    if (output == NULL)
      {
      vtkErrorMacro("Could not get output for part " << partId);
      vtkErrorMacro("Got part from line: " << line);
      return 0;
      }
    
    this->ReadLine(line); // block
    numPts = output->GetNumberOfPoints();
    scalarsRead = new float[numPts];
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
    this->ReadFloatArray(scalarsRead, numPts);
    for (i = 0; i < numPts; i++)
      {
      scalars->InsertComponent(i, component, scalarsRead[i]);        
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
    delete [] scalarsRead;
    lineRead = this->ReadLine(line);
    }
  
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::ReadVectorsPerNode(char* fileName,
                                                char* description,
                                                int timeStep, int measured)
{
  char line[80];
  int partId, numPts, i;
  vtkFloatArray *vectors;
  float vector[3];
  float *vectorsRead;
  long pos;
  vtkDataSet *output;
  int lineRead;
  
  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("NULL VectorPerNode variable file name");
    return 0;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    sfilename += fileName;
    vtkDebugMacro("full path to vector per node file: " << sfilename.c_str());
    }
  else
    {
    sfilename = fileName;
    }
  
  if (this->OpenFile(sfilename.c_str()) == 0)
    {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
    }

  if (this->UseFileSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
        {
        this->ReadLine(line);
        }
      this->ReadLine(line); // skip the description line
      
      pos = this->IFile->tellg();
      this->ReadLine(line); // 1st data line or part #
      if (strncmp(line, "part", 4) != 0)
        {
        this->IFile->seekg(pos, ios::beg);
        if (!measured)
          {
          numPts = this->UnstructuredPoints->GetNumberOfPoints();
          }
        else
          {
          numPts = this->GetOutput(this->NumberOfGeometryParts)->
            GetNumberOfPoints();
          }
        
        vectorsRead = new float[numPts*3];
        this->ReadFloatArray(vectorsRead, numPts*3);

        delete [] vectorsRead;
        }
      
      // vectors for structured parts
      while (this->ReadLine(line) && strncmp(line, "part", 4) == 0)
        {
        sscanf(line, " part %d", &partId);
        partId--;
        this->ReadLine(line); // block
        numPts = this->GetOutput(partId)->GetNumberOfPoints();
        vectorsRead = new float[numPts*3];
        
        this->ReadFloatArray(vectorsRead, numPts*3);

        delete [] vectorsRead;
        }
      }
    lineRead = this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0 && lineRead)
      {
      lineRead = this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line

  pos = this->IFile->tellg();
  lineRead = this->ReadLine(line); // 1st data line or part #
  if (strncmp(line, "part", 4) != 0)
    {
    this->IFile->seekg(pos, ios::beg);
    if (!measured)
      {
      numPts = this->UnstructuredPoints->GetNumberOfPoints();
      }
    else
      {
      numPts = this->GetOutput(this->NumberOfGeometryParts)->
        GetNumberOfPoints();
      }
    
    vectors = vtkFloatArray::New();
    vectors->SetNumberOfTuples(numPts);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numPts*3);
    vectorsRead = new float[numPts*3];
    this->ReadFloatArray(vectorsRead, numPts*3);
    for (i = 0; i < numPts; i++)
      {
      vector[0] = vectorsRead[3*i];
      vector[1] = vectorsRead[3*i+1];
      vector[2] = vectorsRead[3*i+2];
      vectors->InsertTuple(i, vector);
      }
    
    if (!measured)
      {
      for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
        {
        partId = this->UnstructuredPartIds->GetId(i);
        output = this->GetOutput(partId);
        vectors->SetName(description);
        output->GetPointData()->AddArray(vectors);
        if (!output->GetPointData()->GetVectors())
          {
          output->GetPointData()->SetVectors(vectors);
          }
        }
      }
    else
      {
      vectors->SetName(description);
      output = this->GetOutput(this->NumberOfGeometryParts);
      output->GetPointData()->AddArray(vectors);
      if (!output->GetPointData()->GetVectors())
        {
        output->GetPointData()->SetVectors(vectors);
        }
      }
    
    vectors->Delete();
    delete [] vectorsRead;
    }

  // vectors for structured parts
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    sscanf(line, " part %d", &partId);
    partId--;
    output = this->GetOutput(partId);
    this->ReadLine(line); // block
    numPts = output->GetNumberOfPoints();
    vectors = vtkFloatArray::New();
    vectors->SetNumberOfTuples(numPts);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numPts*3);
    vectorsRead = new float[numPts*3];
    
    this->ReadFloatArray(vectorsRead, numPts*3);
    for (i = 0; i < numPts; i++)
      {
      vector[0] = vectorsRead[3*i];
      vector[1] = vectorsRead[3*i+1];
      vector[2] = vectorsRead[3*i+2];
      vectors->InsertTuple(i, vector);
      }
      
    vectors->SetName(description);
    output->GetPointData()->AddArray(vectors);
    if (!output->GetPointData()->GetVectors())
      {
      output->GetPointData()->SetVectors(vectors);
      }
    vectors->Delete();
    delete [] vectorsRead;
    lineRead = this->ReadLine(line);
    }
  
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::ReadTensorsPerNode(char* fileName,
                                                char* description,
                                                int timeStep)
{
  char line[80];
  int partId, numPts, i;
  vtkFloatArray *tensors;
  float tensor[6];
  float* tensorsRead;
  long pos;
  vtkDataSet *output;
  int lineRead;
  
  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("NULL TensorSymmPerNode variable file name");
    return 0;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    sfilename += fileName;
    vtkDebugMacro("full path to tensor symm per node file: " 
                  << sfilename.c_str());
    }
  else
    {
    sfilename = fileName;
    }
  
  if (this->OpenFile(sfilename.c_str()) == 0)
    {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
    }

  if (this->UseTimeSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
        {
        this->ReadLine(line);
        }
      this->ReadLine(line); // skip the description line
      
      pos = this->IFile->tellg();
      this->ReadLine(line); // 1st data line or part #
      if (strncmp(line, "part", 4) != 0)
        {
        this->IFile->seekg(pos, ios::beg);
        numPts = this->UnstructuredPoints->GetNumberOfPoints();
        tensorsRead = new float[numPts*6];
        this->ReadFloatArray(tensorsRead, numPts*6);

        delete [] tensorsRead;
        }
      
      // vectors for structured parts
      while (this->ReadLine(line) &&
             strncmp(line, "part", 4) == 0)
        {
        sscanf(line, " part %d", &partId);
        partId--;
        this->ReadLine(line); // block
        numPts = this->GetOutput(partId)->GetNumberOfPoints();
        tensorsRead = new float[numPts*6];
        this->ReadFloatArray(tensorsRead, numPts*6);

        delete [] tensorsRead;
        }      
      }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line

  pos = this->IFile->tellg();
  lineRead = this->ReadLine(line); // 1st data line or part #
  if (strncmp(line, "part", 4) != 0)
    {
    this->IFile->seekg(pos, ios::beg);
    numPts = this->UnstructuredPoints->GetNumberOfPoints();
    tensors = vtkFloatArray::New();
    tensors->SetNumberOfTuples(numPts);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numPts*6);
    tensorsRead = new float[numPts*6];
    this->ReadFloatArray(tensorsRead, numPts*6);
    for (i = 0; i < numPts; i++)
      {
      tensor[0] = tensorsRead[6*i];
      tensor[1] = tensorsRead[6*i+1];
      tensor[2] = tensorsRead[6*i+2];
      tensor[3] = tensorsRead[6*i+3];
      tensor[4] = tensorsRead[6*i+4];
      tensor[5] = tensorsRead[6*i+5];
      tensors->InsertTuple(i, tensor);
      }

    for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
      {
      partId = this->UnstructuredPartIds->GetId(i);
      tensors->SetName(description);
      this->GetOutput(partId)->GetPointData()->AddArray(tensors);
      }
    tensors->Delete();
    delete [] tensorsRead;
    }

  // vectors for structured parts
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    sscanf(line, " part %d", &partId);
    partId--;
    output = this->GetOutput(partId);
    this->ReadLine(line); // block
    numPts = output->GetNumberOfPoints();
    tensors = vtkFloatArray::New();
    tensors->SetNumberOfTuples(numPts);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numPts*6);
    tensorsRead = new float[numPts*6];
    this->ReadFloatArray(tensorsRead, numPts*6);
    
    for (i = 0; i < numPts; i++)
      {
      tensor[0] = tensorsRead[6*i];
      tensor[1] = tensorsRead[6*i+1];
      tensor[2] = tensorsRead[6*i+2];
      tensor[3] = tensorsRead[6*i+3];
      tensor[4] = tensorsRead[6*i+4];
      tensor[5] = tensorsRead[6*i+5];      
      tensors->InsertTuple(i, tensor);
      }
    
    tensors->SetName(description);
    output->GetPointData()->AddArray(tensors);
    tensors->Delete();
    delete [] tensorsRead;
    
    lineRead = this->ReadLine(line);
    }
  
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::ReadScalarsPerElement(char* fileName,
                                                   char* description,
                                                   int timeStep,
                                                   int numberOfComponents,
                                                   int component)
{
  char line[80];
  int partId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray *scalars;
  int elementType;
  float* scalarsRead;
  int lineRead;
  vtkDataSet *output;
  
  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("NULL ScalarPerElement variable file name");
    return 0;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    sfilename += fileName;
    vtkDebugMacro("full path to scalar per element file: " 
                  << sfilename.c_str());
    }
  else
    {
    sfilename = fileName;
    }
  
  if (this->OpenFile(sfilename.c_str()) == 0) 
   {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
    }

  if (this->UseFileSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
        {
        this->ReadLine(line);
        }
      this->ReadLine(line); // skip the description line
      lineRead = this->ReadLine(line);
      
      while (lineRead && strncmp(line, "part", 4) == 0)
        {
        sscanf(line, " part %d", &partId);
        partId--; // EnSight starts #ing with 1.
        numCells = this->GetOutput(partId)->GetNumberOfCells();
        lineRead = this->ReadLine(line); // element type or "block"
        
        // need to find out from CellIds how many cells we have of this element
        // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
        if (strcmp(line, "block") != 0)
          {
          while (lineRead && strncmp(line, "part", 4) != 0 &&
                 strncmp(line, "END TIME STEP", 13) != 0)
            {
            elementType = this->GetElementType(line);
            if (elementType < 0)
              {
              vtkErrorMacro("invalid element type");
              this->IFile->close();
              delete this->IFile;
              this->IFile = NULL;
              return 0;
              }
            idx = this->UnstructuredPartIds->IsId(partId);
            numCellsPerElement = this->GetCellIds(idx, elementType)->
              GetNumberOfIds();
            scalarsRead = new float[numCellsPerElement];
            this->ReadFloatArray(scalarsRead, numCellsPerElement);

            delete [] scalarsRead;
            lineRead = this->ReadLine(line);
            } // end while
          }
        else
          {
          scalarsRead = new float[numCells];
          this->ReadFloatArray(scalarsRead, numCells);

          delete [] scalarsRead;
          lineRead = this->ReadLine(line);
          }
        }
      }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line
  lineRead = this->ReadLine(line);
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing with 1.
    output = this->GetOutput(partId);
    numCells = output->GetNumberOfCells();
    lineRead = this->ReadLine(line); // element type or "block"
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
    if (strcmp(line, "block") != 0)
      {
      while (lineRead && strncmp(line, "part", 4) != 0 &&
        strncmp(line, "END TIME STEP", 13) != 0)
        {
        elementType = this->GetElementType(line);
        if (elementType < 0)
          {
          vtkErrorMacro("invalid element type");
          this->IFile->close();
          delete this->IFile;
          this->IFile = NULL;
          return 0;
          }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        scalarsRead = new float[numCellsPerElement];
        this->ReadFloatArray(scalarsRead, numCellsPerElement);
        for (i = 0; i < numCellsPerElement; i++)
          {
          scalars->InsertComponent(this->GetCellIds(idx, elementType)->GetId(i),
                                   component, scalarsRead[i]);
          }
        delete [] scalarsRead;
        lineRead = this->ReadLine(line);
        } // end while
      }
    else
      {
      scalarsRead = new float[numCells];
      this->ReadFloatArray(scalarsRead, numCells);
      for (i = 0; i < numCells; i++)
        {
        scalars->InsertComponent(i, component, scalarsRead[i]);
        }
      delete [] scalarsRead;
      lineRead = this->ReadLine(line);
      }
    
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
  
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::ReadVectorsPerElement(char* fileName,
                                                   char* description,
                                                   int timeStep)
{
  char line[80];
  int partId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray *vectors;
  int elementType;
  float vector[3];
  float *vectorsRead;
  int lineRead;
  vtkDataSet *output;
  
  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("NULL VectorPerElement variable file name");
    return 0;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    sfilename += fileName;
    vtkDebugMacro("full path to vector per element file: " 
                  << sfilename.c_str());
    }
  else
    {
    sfilename = fileName;
    }
  
  if (this->OpenFile(sfilename.c_str()) == 0)
    {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
    }

  if (this->UseFileSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
        {
        this->ReadLine(line);
        }
      this->ReadLine(line); // skip the description line
      lineRead = this->ReadLine(line);
      
      while (lineRead && strncmp(line, "part", 4) == 0)
        {
        sscanf(line, " part %d", &partId);
        partId--; // EnSight starts #ing with 1.
        numCells = this->GetOutput(partId)->GetNumberOfCells();
        lineRead = this->ReadLine(line); // element type or "block"
        
        // need to find out from CellIds how many cells we have of this element
        // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
        if (strcmp(line, "block") != 0)
          {
          while (lineRead && strncmp(line, "part", 4) != 0 &&
                  strncmp(line, "END TIME STEP", 13) != 0)
            {
            elementType = this->GetElementType(line);
            if (elementType < 0)
              {
              vtkErrorMacro("invalid element type");
              delete this->IS;
              this->IS = NULL;
              return 0;
              }
            idx = this->UnstructuredPartIds->IsId(partId);
            numCellsPerElement =
              this->GetCellIds(idx, elementType)->GetNumberOfIds();
            vectorsRead = new float[numCellsPerElement*3];
            this->ReadFloatArray(vectorsRead, numCellsPerElement*3);

            delete [] vectorsRead;
            lineRead = this->ReadLine(line);
            } // end while
          }
        else
          {
          vectorsRead = new float[numCells*3];
          this->ReadFloatArray(vectorsRead, numCells*3);

          delete [] vectorsRead;
          lineRead = this->ReadLine(line);
          }
        }
      }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line
  lineRead = this->ReadLine(line);
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    vectors = vtkFloatArray::New();
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing with 1.
    output = this->GetOutput(partId);
    numCells = output->GetNumberOfCells();
    lineRead = this->ReadLine(line); // element type or "block"
    vectors->SetNumberOfTuples(numCells);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numCells*3);
    
    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") != 0)
      {
      while (lineRead && strncmp(line, "part", 4) != 0 &&
        strncmp(line, "END TIME STEP", 13) != 0)
        {
        elementType = this->GetElementType(line);
        if (elementType < 0)
          {
          vtkErrorMacro("invalid element type");
          delete this->IS;
          this->IS = NULL;
          return 0;
          }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        vectorsRead = new float[numCellsPerElement*3];
        this->ReadFloatArray(vectorsRead, numCellsPerElement*3);
        
        for (i = 0; i < numCellsPerElement; i++)
          {
          vector[0] = vectorsRead[3*i];
          vector[1] = vectorsRead[3*i+1];
          vector[2] = vectorsRead[3*i+2];
          vectors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(i),
                               vector);
          }
        delete [] vectorsRead;
        lineRead = this->ReadLine(line);
        } // end while
      }
    else
      {
      vectorsRead = new float[numCells*3];
      this->ReadFloatArray(vectorsRead, numCells*3);
      for (i = 0; i < numCells; i++)
        {
        vector[0] = vectorsRead[3*i];
        vector[1] = vectorsRead[3*i+1];
        vector[2] = vectorsRead[3*i+2];
        vectors->InsertTuple(i, vector);
        }
      delete [] vectorsRead;
      lineRead = this->ReadLine(line);
      }
    vectors->SetName(description);
    output->GetCellData()->AddArray(vectors);
    if (!output->GetCellData()->GetVectors())
      {
      output->GetCellData()->SetVectors(vectors);
      }
    vectors->Delete();
    }
  
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::ReadTensorsPerElement(char* fileName,
                                                   char* description,
                                                   int timeStep)
{
  char line[80];
  int partId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray *tensors;
  int elementType;
  float tensor[6];
  float *tensorsRead;
  int lineRead;
  vtkDataSet *output;
  
  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("NULL TensorPerElement variable file name");
    return 0;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    sfilename += fileName;
    vtkDebugMacro("full path to tensor per element file: " 
                  << sfilename.c_str());
    }
  else
    {
    sfilename = fileName;
    }
  
  if (this->OpenFile(sfilename.c_str()) == 0)
    {
    vtkErrorMacro("Unable to open file: " << sfilename.c_str());
    return 0;
    }

  if (this->UseTimeSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      this->ReadLine(line);
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
        {
        this->ReadLine(line);
        }
      this->ReadLine(line); // skip the description line
      lineRead = this->ReadLine(line);
      
      while (lineRead && strncmp(line, "part", 4) == 0)
        {
        sscanf(line, " part %d", &partId);
        partId--; // EnSight starts #ing with 1.
        numCells = this->GetOutput(partId)->GetNumberOfCells();
        lineRead = this->ReadLine(line); // element type or "block"
        
        // need to find out from CellIds how many cells we have of this element
        // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
        if (strcmp(line, "block") != 0)
          {
          while (lineRead && strncmp(line, "part", 4) != 0 &&
                 strncmp(line, "END TIME STEP", 13) != 0)
            {
            elementType = this->GetElementType(line);
            if (elementType < 0)
              {
              vtkErrorMacro("invalid element type");
              this->IFile->close();
              delete this->IFile;
              this->IFile = NULL;
              return 0;
              }
            idx = this->UnstructuredPartIds->IsId(partId);
            numCellsPerElement = this->GetCellIds(idx, elementType)->
              GetNumberOfIds();
            tensorsRead = new float[numCellsPerElement*6];
            this->ReadFloatArray(tensorsRead, numCellsPerElement*6);

            delete [] tensorsRead;
            lineRead = this->ReadLine(line);
            } // end while
          }
        else
          {
          tensorsRead = new float[numCells*6];
          this->ReadFloatArray(tensorsRead, numCells*6);

          delete [] tensorsRead;
          lineRead = this->ReadLine(line);
          }
        }
      }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line
  lineRead = this->ReadLine(line);
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    tensors = vtkFloatArray::New();
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing with 1.
    output = this->GetOutput(partId);
    numCells = output->GetNumberOfCells();
    lineRead = this->ReadLine(line); // element type or "block"
    tensors->SetNumberOfTuples(numCells);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numCells*6);
    
    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") != 0)
      {
      while (lineRead && strncmp(line, "part", 4) != 0 &&
             strncmp(line, "END TIME STEP", 13) != 0)
        {
        elementType = this->GetElementType(line);
        if (elementType < 0)
          {
          vtkErrorMacro("invalid element type");
          this->IFile->close();
          delete this->IFile;
          this->IFile = NULL;
          return 0;
          }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        tensorsRead = new float[numCellsPerElement*6];
        this->ReadFloatArray(tensorsRead, numCellsPerElement*6);
        
        for (i = 0; i < numCellsPerElement; i++)
          {
          tensor[0] = tensorsRead[6*i];
          tensor[1] = tensorsRead[6*i+1];
          tensor[2] = tensorsRead[6*i+2];
          tensor[3] = tensorsRead[6*i+3];
          tensor[4] = tensorsRead[6*i+4];
          tensor[5] = tensorsRead[6*i+5];
          
          tensors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(i),
                               tensor);
          }
        delete [] tensorsRead;
        lineRead = this->ReadLine(line);
        } // end while
      }
    else
      {
      tensorsRead = new float[numCells*6];
      this->ReadFloatArray(tensorsRead, numCells*6);
      
      for (i = 0; i < numCells; i++)
        {
        tensor[0] = tensorsRead[6*i];
        tensor[1] = tensorsRead[6*i+1];
        tensor[2] = tensorsRead[6*i+2];
        tensor[3] = tensorsRead[6*i+3];
        tensor[4] = tensorsRead[6*i+4];
        tensor[5] = tensorsRead[6*i+5];
        tensors->InsertTuple(i, tensor);
        }
      delete [] tensorsRead;
      lineRead = this->ReadLine(line);
      }
    tensors->SetName(description);
    output->GetCellData()->AddArray(tensors);
    tensors->Delete();
    }
  
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::CreateUnstructuredGridOutput(int partId,
                                                          char line[80],
                                                          const char* name)
{
  int lineRead = 1;
  int i, j;
  int *nodeIdList;
  vtkIdType *nodeIds;
  int numElements;
  int idx, cellType;
  vtkIdType cellId;

  this->NumberOfNewOutputs++;
  
  if (this->GetOutput(partId) == NULL)
    {
    vtkDebugMacro("creating new unstructured output");
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::New();
    this->SetNthOutput(partId, ugrid);
    ugrid->Delete();
    
    this->UnstructuredPartIds->InsertNextId(partId);
    }
  else if ( ! this->GetOutput(partId)->IsA("vtkUnstructuredGrid"))
    {
    vtkErrorMacro("Cannot change type of output");
    this->OutputsAreValid = 0;
    return 0;
    }
  
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
    this->GetOutput(partId));    

  vtkCharArray* nmArray =  vtkCharArray::New();
  nmArray->SetName("Name");
  size_t len = strlen(name);
  nmArray->SetNumberOfTuples(static_cast<vtkIdType>(len)+1);
  char* copy = nmArray->GetPointer(0);
  memcpy(copy, name, len);
  copy[len] = '\0';
  output->GetFieldData()->AddArray(nmArray);
  nmArray->Delete();

  // Clear all cell ids from the last execution, if any.
  idx = this->UnstructuredPartIds->IsId(partId);
  for (i = 0; i < 16; i++)
    {
    this->GetCellIds(idx, i)->Reset();
    }
  
  output->Allocate(1000);
  
  while(lineRead && strncmp(line, "part", 4) != 0)
    {
    if (strncmp(line, "point", 5) == 0)
      {
      vtkDebugMacro("point");
      
      nodeIds = new vtkIdType[1];
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        // There is probably a better way to advance the file pointer.
        int *tempArray = new int[numElements];
        this->ReadIntArray(tempArray, numElements);
        delete [] tempArray;
        }
      
      nodeIdList = new int[numElements];
      this->ReadIntArray(nodeIdList, numElements);
      
      for (i = 0; i < numElements; i++)
        {
        nodeIds[0] = nodeIdList[i] - 1;
        if (this->UnstructuredNodeIds)
          {
          nodeIds[0] = this->UnstructuredNodeIds->GetValue(nodeIds[0]);
          }
        cellId = output->InsertNextCell(VTK_VERTEX, 1, nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::POINT)->InsertNextId(cellId);
        }
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    else if (strncmp(line, "bar2", 4) == 0)
      {
      vtkDebugMacro("bar2");
      
      nodeIds = new vtkIdType[2];
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        // There is probably a better way to advance the file pointer.
        int *tempArray = new int[numElements];
        this->ReadIntArray(tempArray, numElements);
        delete [] tempArray;
        }
      
      nodeIdList = new int[numElements * 2];
      this->ReadIntArray(nodeIdList, numElements*2);
      
      for (i = 0; i < numElements; i++)
        {
        for (j = 0; j < 2; j++)
          {
          nodeIds[j] = nodeIdList[2*i+j] - 1;
          }

        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 2; j++)
            {
            nodeIds[j] = this->UnstructuredNodeIds->GetValue(nodeIds[j]);
            }
          }
        cellId = output->InsertNextCell(VTK_LINE, 2, nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::BAR2)->InsertNextId(cellId);
        }
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    else if (strncmp(line, "bar3", 4) == 0)
      {
      vtkDebugMacro("bar3");
      vtkWarningMacro("Only vertex nodes of this element will be read.");
      
      nodeIds = new vtkIdType[2];
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        // There is probably a better way to advance the file pointer.
        int *tempArray = new int[numElements];
        this->ReadIntArray(tempArray, numElements);
        delete [] tempArray;
        }
      
      nodeIdList = new int[numElements * 3];
      this->ReadIntArray(nodeIdList, numElements*3);
      
      for (i = 0; i < numElements; i++)
        {
        for (j = 0; j < 2; j++)
          {
          nodeIds[j] = nodeIdList[3*i+2*j] - 1;
          }

        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 2; j++)
            {
            nodeIds[j] = this->UnstructuredNodeIds->GetValue(nodeIds[j]);
            }
          }
        cellId = output->InsertNextCell(VTK_LINE, 2, nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::BAR3)->InsertNextId(cellId);
        }
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    else if (strncmp(line, "tria3", 5) == 0 ||
             strncmp(line, "tria6", 5) == 0)
      {
      if (strncmp(line, "tria3", 5) == 0)
        {
        vtkDebugMacro("tria3");
        cellType = vtkEnSightReader::TRIA3;
        }
      else
        {
        vtkDebugMacro("tria6");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::TRIA6;
        }
      
      nodeIds = new vtkIdType[3];
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        // There is probably a better way to advance the file pointer.
        int *tempArray = new int[numElements];
        this->ReadIntArray(tempArray, numElements);
        delete [] tempArray;
        }
      
      if (cellType == vtkEnSightReader::TRIA3)
        {
        nodeIdList = new int[numElements * 3];
        this->ReadIntArray(nodeIdList, numElements*3);
        }
      else
        {
        nodeIdList = new int[numElements * 6];
        this->ReadIntArray(nodeIdList, numElements*6);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::TRIA3)
          {
          for (j = 0; j < 3; j++)
            {
            nodeIds[j] = nodeIdList[3*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 3; j++)
            {
            nodeIds[j] = nodeIdList[6*i+j] - 1;
            }
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 3; j++)
            {
            nodeIds[j] = this->UnstructuredNodeIds->GetValue(nodeIds[j]);
            }
          }
        cellId = output->InsertNextCell(VTK_TRIANGLE, 3, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        }
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    else if (strncmp(line, "quad4", 5) == 0 ||
             strncmp(line, "quad8", 5) == 0)
      {
      if (strncmp(line, "quad8", 5) == 0)
        {
        vtkDebugMacro("quad8");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::QUAD8;
        }
      else
        {
        vtkDebugMacro("quad4");
        cellType = vtkEnSightReader::QUAD4;
        }
      
      nodeIds = new vtkIdType[4];
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        // There is probably a better way to advance the file pointer.
        int *tempArray = new int[numElements];
        this->ReadIntArray(tempArray, numElements);
        delete [] tempArray;
        }
      
      if (cellType == vtkEnSightReader::QUAD4)
        {
        nodeIdList = new int[numElements * 4];
        this->ReadIntArray(nodeIdList, numElements*4);
        }
      else
        {
        nodeIdList = new int[numElements * 8];
        this->ReadIntArray(nodeIdList, numElements*8);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::QUAD4)
          {
          for (j = 0; j < 4; j++)
            {
            nodeIds[j] = nodeIdList[4*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 4; j++)
            {
            nodeIds[j] = nodeIdList[8*i+j] - 1;
            }
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 4; j++)
            {
            nodeIds[j] = this->UnstructuredNodeIds->GetValue(nodeIds[j]);
            }
          }
        cellId = output->InsertNextCell(VTK_QUAD, 4, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        }
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    else if (strncmp(line, "tetra4", 6) == 0 ||
             strncmp(line, "tetra10", 7) == 0)
      {
      if (strncmp(line, "tetra10", 7) == 0)
        {
        vtkDebugMacro("tetra10");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::TETRA10;
        }
      else
        {
        vtkDebugMacro("tetra4");
        cellType = vtkEnSightReader::TETRA4;
        }
      
      nodeIds = new vtkIdType[4];
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        // There is probably a better way to advance the file pointer.
        int *tempArray = new int[numElements];
        this->ReadIntArray(tempArray, numElements);
        delete [] tempArray;
        }
      
      if (cellType == vtkEnSightReader::TETRA4)
        {
        nodeIdList = new int[numElements * 4];
        this->ReadIntArray(nodeIdList, numElements*4);
        }
      else
        {
        nodeIdList = new int[numElements * 10];
        this->ReadIntArray(nodeIdList, numElements*10);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::TETRA4)
          {
          for (j = 0; j < 4; j++)
            {
            nodeIds[j] = nodeIdList[4*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 4; j++)
            {
            nodeIds[j] = nodeIdList[10*i+j] - 1;
            }
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 3; j++)
            {
            nodeIds[j] = this->UnstructuredNodeIds->GetValue(nodeIds[j]);
            }
          }
        cellId = output->InsertNextCell(VTK_TETRA, 4, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        }
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    else if (strncmp(line, "pyramid5", 8) == 0 ||
             strncmp(line, "pyramid13", 9) == 0)
      {
      if (strncmp(line, "pyramid13", 9) == 0)
        {
        vtkDebugMacro("pyramid13");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::PYRAMID13;
        }
      else
        {
        vtkDebugMacro("pyramid5");
        cellType = vtkEnSightReader::PYRAMID5;
        }

      nodeIds = new vtkIdType[5];
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        // There is probably a better way to advance the file pointer.
        int *tempArray = new int[numElements];
        this->ReadIntArray(tempArray, numElements);
        delete [] tempArray;
        }
      
      if (cellType == vtkEnSightReader::PYRAMID5)
        {
        nodeIdList = new int[numElements * 5];
        this->ReadIntArray(nodeIdList, numElements*5);
        }
      else
        {
        nodeIdList = new int[numElements * 13];
        this->ReadIntArray(nodeIdList, numElements*13);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::PYRAMID5)
          {
          for (j = 0; j < 5; j++)
            {
            nodeIds[j] = nodeIdList[5*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 5; j++)
            {
            nodeIds[j] = nodeIdList[13*i+j] - 1;
            }
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 5; j++)
            {
            nodeIds[j] = this->UnstructuredNodeIds->GetValue(nodeIds[j]);
            }
          }
        cellId = output->InsertNextCell(VTK_PYRAMID, 5, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        }
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    else if (strncmp(line, "hexa8", 5) == 0 ||
             strncmp(line, "hexa20", 6) == 0)
      {
      if (strncmp(line, "hexa20", 6) == 0)
        {
        vtkDebugMacro("hexa20");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::HEXA20;
        }
      else
        {
        vtkDebugMacro("hexa8");
        cellType = vtkEnSightReader::HEXA8;
        }
      
      nodeIds = new vtkIdType[8];
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        // There is probably a better way to advance the file pointer.
        int *tempArray = new int[numElements];
        this->ReadIntArray(tempArray, numElements);
        delete [] tempArray;
        }
      
      if (cellType == vtkEnSightReader::HEXA8)
        {
        nodeIdList = new int[numElements * 8];
        this->ReadIntArray(nodeIdList, numElements*8);
        }
      else
        {
        nodeIdList = new int[numElements * 20];
        this->ReadIntArray(nodeIdList, numElements*20);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::HEXA8)
          {
          for (j = 0; j < 8; j++)
            {
            nodeIds[j] = nodeIdList[8*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 8; j++)
            {
            nodeIds[j] = nodeIdList[20*i+j] - 1;
            }
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 8; j++)
            {
            nodeIds[j] = this->UnstructuredNodeIds->GetValue(nodeIds[j]);
            }
          }
        cellId = output->InsertNextCell(VTK_HEXAHEDRON, 8, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        }
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    else if (strncmp(line, "penta6", 6) == 0 ||
             strncmp(line, "penta15", 7) == 0)
      {
      if (strncmp(line, "penta15", 7) == 0)
        {
        vtkDebugMacro("penta15");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::PENTA15;
        }
      else
        {
        vtkDebugMacro("penta6");
        cellType = vtkEnSightReader::PENTA6;
        }
      
      nodeIds = new vtkIdType[6];
      this->ReadIntNumber(&numElements);
      if (this->ElementIdsListed)
        {
        // There is probably a better way to advance the file pointer.
        int *tempArray = new int[numElements];
        this->ReadIntArray(tempArray, numElements);
        delete [] tempArray;
        }
      
      if (cellType == vtkEnSightReader::PENTA6)
        {
        nodeIdList = new int[numElements * 6];
        this->ReadIntArray(nodeIdList, numElements*6);
        }
      else
        {
        nodeIdList = new int[numElements * 15];
        this->ReadIntArray(nodeIdList, numElements*15);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::PENTA6)
          {
          for (j = 0; j < 6; j++)
            {
            nodeIds[j] = nodeIdList[6*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 6; j++)
            {
            nodeIds[j] = nodeIdList[15*i+j] - 1;
            }
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 6; j++)
            {
            nodeIds[j] = this->UnstructuredNodeIds->GetValue(nodeIds[j]);
            }
          }
        cellId = output->InsertNextCell(VTK_WEDGE, 6, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        }
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    lineRead = this->ReadLine(line);
    }

  ((vtkUnstructuredGrid*)this->GetOutput(partId))->
    SetPoints(this->UnstructuredPoints);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSight6BinaryReader::CreateStructuredGridOutput(int partId,
                                                        char line[80],
                                                        const char* name)
{
  char subLine[80];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkPoints *points = vtkPoints::New();
  int numPts;
  float *coordsRead;
  int *iblanks;
  
  this->NumberOfNewOutputs++;
  
  if (this->GetOutput(partId) == NULL)
    {
    vtkDebugMacro("creating new structured grid output");
    vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
    this->SetNthOutput(partId, sgrid);
    sgrid->Delete();
    }
  else if ( ! this->GetOutput(partId)->IsA("vtkStructuredGrid"))
    {
    vtkErrorMacro("Cannot change type of output");
    this->OutputsAreValid = 0;
    return 0;
    }

  vtkStructuredGrid* output = vtkStructuredGrid::SafeDownCast(
    this->GetOutput(partId));    

  vtkCharArray* nmArray =  vtkCharArray::New();
  nmArray->SetName("Name");
  size_t len = strlen(name);
  nmArray->SetNumberOfTuples(static_cast<vtkIdType>(len)+1);
  char* copy = nmArray->GetPointer(0);
  memcpy(copy, name, len);
  copy[len] = '\0';
  output->GetFieldData()->AddArray(nmArray);
  nmArray->Delete();
  
  if (sscanf(line, " %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "iblanked") == 0)
      {
      iblanked = 1;
      }
    }
  // Read these separately to get the byte order correct.  
  this->ReadIntNumber(dimensions);
  this->ReadIntNumber(dimensions+1);
  this->ReadIntNumber(dimensions+2);
  output->SetDimensions(dimensions);
  output->SetWholeExtent(
    0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  points->Allocate(numPts);
  
  coordsRead = new float[numPts*3];
  this->ReadFloatArray(coordsRead, numPts*3);
  
  for (i = 0; i < numPts; i++)
    {
    points->InsertNextPoint(coordsRead[i], coordsRead[numPts+i],
                            coordsRead[2*numPts+i]);
    }
  
  delete [] coordsRead;
  
  output->SetPoints(points);  
  if (iblanked)
    {
    iblanks = new int[numPts];
    this->ReadIntArray(iblanks, numPts);
    for (i = 0; i < numPts; i++)
      {
      if (!iblanks[i])
        {
        output->BlankPoint(i);
        }
      }
    delete [] iblanks;
    }
  
  points->Delete();
  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

// Internal function to read in a line up to 80 characters.
// Returns zero if there was an error.
int vtkEnSight6BinaryReader::ReadLine(char result[80])
{
  if ( ! this->IFile->read(result, sizeof(char)*80))
    {
    return 0;
    }
  
  return 1;
}

// Internal function to read a single integer.
// Returns zero if there was an error.
// This method is used to read the number of points ...
// It also tries to determine the byte order.
// It uses the byte order that produces the largest value that
// is smaller than the file.  Although this computation
// assumes only one int array is in the file,
// it should still work fine.
int vtkEnSight6BinaryReader::ReadIntNumber(int *result)
{
  if ( ! this->IFile->read((char*)result, sizeof(int)))
    {
    return 0;
    }
  if (this->ByteOrder == FILE_LITTLE_ENDIAN)
    {
    vtkByteSwap::Swap4LE(result);
    }
  else
    {
    vtkByteSwap::Swap4BE(result);
    }
  
  // Experimental byte swap.
  int tmp = *result;
  vtkByteSwap::SwapVoidRange(&tmp, 1, sizeof(int));
  // Use negative value as an indication of bad number.
  if ((tmp*(int)(sizeof(int))) > this->FileSize)
    {
    tmp = -1;
    }
  if ((*result*(int)(sizeof(int))) > this->FileSize)
    {
    *result = -1;
    }
  // Just a sanity check. (0, 0 occurs often).
  // This condition would only occur for some really large files.
  if (*result > 0 && tmp > 0)
    {
    vtkWarningMacro("Byte order is ambiguous.");
    }

  // If they are both valid, use the larger one.
  if (tmp > 0 && tmp > *result)
    { // Byte order must be wrong
    if (this->ByteOrder == FILE_LITTLE_ENDIAN)
      {
      this->ByteOrder = FILE_BIG_ENDIAN;
      }
    else
      {
      this->ByteOrder = FILE_LITTLE_ENDIAN;
      }
    *result = tmp;
    return 1;
    }

  if (*result < 0)
    { // both byte swaps are bad.
    vtkErrorMacro("Could not find a suitable byte order.");
    *result = 0;
    return 0;
    }

  return 1;
}

// Internal function to read an integer array.
// Returns zero if there was an error.
int vtkEnSight6BinaryReader::ReadIntArray(int *result,
                                          int numInts)
{
  if ( ! this->IFile->read((char*)result, sizeof(int)*numInts))
    {
    return 0;
    }
  if (this->ByteOrder == FILE_LITTLE_ENDIAN)
    {
    vtkByteSwap::Swap4LERange(result, numInts);
    }
  else
    {
    vtkByteSwap::Swap4BERange(result, numInts);
    }
  
  return 1;
}

// Internal function to read a float array.
// Returns zero if there was an error.
int vtkEnSight6BinaryReader::ReadFloatArray(float *result,
                                            int numFloats)
{
  if ( ! this->IFile->read((char*)result, sizeof(float)*numFloats))
    {
    return 0;
    }

  if (this->ByteOrder == FILE_LITTLE_ENDIAN)
    {
    vtkByteSwap::Swap4LERange(result, numFloats);
    }
  else
    {
    vtkByteSwap::Swap4BERange(result, numFloats);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkEnSight6BinaryReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
