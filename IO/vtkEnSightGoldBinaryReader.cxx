/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightGoldBinaryReader.cxx
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
#include "vtkEnSightGoldBinaryReader.h"

#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <sys/stat.h>
#include <ctype.h>
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkEnSightGoldBinaryReader, "1.38");
vtkStandardNewMacro(vtkEnSightGoldBinaryReader);

//----------------------------------------------------------------------------
vtkEnSightGoldBinaryReader::vtkEnSightGoldBinaryReader()
{
  this->IFile = NULL;
  this->FileSize = 0;
}

//----------------------------------------------------------------------------
vtkEnSightGoldBinaryReader::~vtkEnSightGoldBinaryReader()
{
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::OpenFile(const char* filename)
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
int vtkEnSightGoldBinaryReader::ReadGeometryFile(char* fileName, int timeStep)
{
  char line[80], subLine[80];
  int partId;
  int lineRead, i;
  
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
  
  this->ReadLine(line);
  sscanf(line, " %*s %s", subLine);
  if (strcmp(subLine, "Binary") != 0 &&
      strcmp(subLine, "binary") != 0)
    {
    vtkErrorMacro("This is not a binary data set. Try "
                  << "vtkEnSightGoldReader.");
    return 0;
    }

  if (this->UseFileSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      this->SkipTimeStep();
      }
    
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  // Skip the 2 description lines.
  this->ReadLine(line);
  this->ReadLine(line);
  
  // Read the node id and element id lines.
  this->ReadLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0)
    {
    this->NodeIdsListed = 1;
    }
  else if (strcmp(subLine, "ignore") == 0)
    {
    this->NodeIdsListed = 1;
    }
  else
    {
    this->NodeIdsListed = 0;
    }
  
  this->ReadLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0)
    {
    this->ElementIdsListed = 1;
    }
  else if (strcmp(subLine, "ignore") == 0)
    {
    this->ElementIdsListed = 1;
    }
  else
    {
    this->ElementIdsListed = 0;
    }
  
  lineRead = this->ReadLine(line); // "extents" or "part"
  if (strcmp(line, "extents") == 0)
    {
    // Skipping the extents.
    this->IFile->ignore(6*sizeof(float));
    lineRead = this->ReadLine(line); // "part"
    }

  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing at 1.
    
    this->ReadLine(line); // part description line
    char *name = strdup(line);
    lineRead = this->ReadLine(line);
    
    if (strncmp(line, "block", 5) == 0)
      {
      if (sscanf(line, " %*s %s", subLine) == 1)
        {
        if (strcmp(subLine, "rectilinear") == 0)
          {
          // block rectilinear
          lineRead = this->CreateRectilinearGridOutput(partId, line, name);
          }
        else if (strcmp(subLine, "uniform") == 0)
          {
          // block uniform
          lineRead = this->CreateImageDataOutput(partId, line, name);
          }
        else
          {
          // block iblanked
          lineRead = this->CreateStructuredGridOutput(partId, line, name);
          }
        }
      else
        {
        // block
        lineRead = this->CreateStructuredGridOutput(partId, line, name);
        }
      }
    else
      {
      lineRead = this->CreateUnstructuredGridOutput(partId, line, name);
      if (lineRead < 0)
        {
        free(name);
        if (this->IFile)
          {
          this->IFile->close();
          delete this->IFile;
          this->IFile = NULL;
          }
        return 0;
        }
      }
    free(name);
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
void vtkEnSightGoldBinaryReader::SkipTimeStep()
{
  char line[80], subLine[80];
  int lineRead;

  line[0] = '\0';
  while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
    {
    this->ReadLine(line);
    }
  
  // Skip the 2 description lines.
  this->ReadLine(line);
  this->ReadLine(line);
  
  // Read the node id and element id lines.
  this->ReadLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0 ||
      strcmp(subLine, "ignore") == 0)
    {
    this->NodeIdsListed = 1;
    }
  else
    {
    this->NodeIdsListed = 0;
    }
  
  this->ReadLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0)
    {
    this->ElementIdsListed = 1;
    }
  else if (strcmp(subLine, "ignore") == 0)
    {
    this->ElementIdsListed = 1;
    }
  else
    {
    this->ElementIdsListed = 0;
    }
  
  lineRead = this->ReadLine(line); // "extents" or "part"
  if (strcmp(line, "extents") == 0)
    {
    // Skipping the extents.
    this->IFile->ignore(6*sizeof(float));
    lineRead = this->ReadLine(line); // "part"
    }
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    int tmpInt;
    this->ReadInt(&tmpInt);
    this->ReadLine(line); // part description line
    lineRead = this->ReadLine(line);
    
    if (strncmp(line, "block", 5) == 0)
      {
      if (sscanf(line, " %*s %s", subLine) == 1)
        {
        if (strcmp(subLine, "rectilinear") == 0)
          {
          // block rectilinear
          lineRead = this->SkipRectilinearGrid(line);
          }
        else if (strcmp(subLine, "uniform") == 0)
          {
          // block uniform
          lineRead = this->SkipImageData(line);
          }
        else
          {
          // block iblanked
          lineRead = this->SkipStructuredGrid(line);
          }
        }
      else
        {
        // block
        lineRead = this->SkipStructuredGrid(line);
        }
      }
    else
      {
      lineRead = this->SkipUnstructuredGrid(line);
      if (lineRead < 0)
        {
        if (this->IFile)
          {
          this->IFile->close();
          delete this->IFile;
          this->IFile = NULL;
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::SkipStructuredGrid(char line[256])
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

  this->ReadIntArray(dimensions, 3);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];

  // Skip xCoords, yCoords and zCoords.
  this->IFile->ignore(sizeof(float)*numPts*3);
 
  if (iblanked)
    { // skip iblank array.
    this->IFile->ignore(numPts*sizeof(int));
    }
  
  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::SkipUnstructuredGrid(char line[256])
{
  int lineRead = 1;
  int i;
  int numElements;
  int cellType;
  
  while(lineRead && strncmp(line, "part", 4) != 0)
    {
    if (strncmp(line, "coordinates", 11) == 0)
      {
      vtkDebugMacro("coordinates");
      int numPts;
      
      this->ReadInt(&numPts);
      vtkDebugMacro("num. points: " << numPts);
      
      if (this->NodeIdsListed)
        { // skip node ids.
        this->IFile->ignore(sizeof(int)*numPts);
        }
      
      // Skip xCoords, yCoords and zCoords.
      this->IFile->ignore(sizeof(float)*3*numPts);
      }
    else if (strncmp(line, "point", 5) == 0)
      {
      vtkDebugMacro("point");
      
      this->ReadInt(&numElements);
      
      if (this->ElementIdsListed)
        { // skip element ids.
        this->IFile->ignore(sizeof(int)*numElements);
        }
      
      // Skip nodeIdList.
      this->IFile->ignore(sizeof(int)*numElements);
      }
    else if (strncmp(line, "bar2", 4) == 0)
      {
      vtkDebugMacro("bar2");
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      // Skip nodeIdList.
      this->IFile->ignore(sizeof(int)*2*numElements);
      }
    else if (strncmp(line, "bar3", 4) == 0)
      {
      vtkDebugMacro("bar3");
      vtkWarningMacro("Only vertex nodes of this element will be read.");

      this->ReadInt(&numElements);

      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }
      
      // Skip nodeIdList.
      this->IFile->ignore(sizeof(int)*2*numElements);
      }
    else if (strncmp(line, "nsided", 6) == 0)
      {
      vtkDebugMacro("nsided");
      int *numNodesPerElement;
      int numNodes = 0;
      
      cellType = vtkEnSightReader::NSIDED;
      this->ReadInt(&numElements);
      
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }
      
      numNodesPerElement = new int[numElements];
      this->ReadIntArray(numNodesPerElement, numElements);
      for (i = 0; i < numElements; i++)
        {
        numNodes += numNodesPerElement[i];
        }
      // Skip nodeIdList.
      this->IFile->ignore(sizeof(int)*numNodes);
      delete [] numNodesPerElement;
      }
    else if (strncmp(line, "tria3", 5) == 0 ||
             strncmp(line, "tria6", 5) == 0)
      {
      if (strncmp(line, "tria6", 5) == 0)
        {
        vtkDebugMacro("tria6");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::TRIA6;
        }
      else
        {
        vtkDebugMacro("tria3");
        cellType = vtkEnSightReader::TRIA3;
        }
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::TRIA6)
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*6*numElements);
        }
      else
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*3*numElements);
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
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::QUAD8)
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*8*numElements);
        }
      else
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*4*numElements);
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
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::TETRA10)
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*10*numElements);
        }
      else
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*4*numElements);
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
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::PYRAMID13)
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*13*numElements);
        }
      else
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*5*numElements);
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
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::HEXA20)
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*20*numElements);
        }
      else
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*8*numElements);
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
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::PENTA15)
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*15*numElements);
        }
      else
        {
        // Skip nodeIdList.
        this->IFile->ignore(sizeof(int)*6*numElements);
        }
      }
    else if (strncmp(line, "END TIME STEP", 13) == 0)
      {
      return 1;
      }
    else
      {
      vtkErrorMacro("undefined geometry file line");
      return -1;
      }
    lineRead = this->ReadLine(line);
    }
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::SkipRectilinearGrid(char line[256])
{
  char subLine[80];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int numPts;
  
  if (sscanf(line, " %*s %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "iblanked") == 0)
      {
      iblanked = 1;
      }
    }

  this->ReadIntArray(dimensions, 3);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  
  // Skip xCoords
  this->IFile->ignore(sizeof(float)*dimensions[0]);
  // Skip yCoords
  this->IFile->ignore(sizeof(float)*dimensions[1]);
  // Skip zCoords
  this->IFile->ignore(sizeof(float)*dimensions[2]);

  if (iblanked)
    {
    vtkWarningMacro("VTK does not handle blanking for rectilinear grids.");
    this->IFile->ignore(sizeof(int)*numPts);
    }
  
  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::SkipImageData(char line[256])
{
  char subLine[80];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  float origin[3], delta[3];
  int numPts;
  
  if (sscanf(line, " %*s %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "iblanked") == 0)
      {
      iblanked = 1;
      }
    }

  this->ReadIntArray(dimensions, 3);
  this->ReadFloatArray(origin, 3);
  this->ReadFloatArray(delta, 3);
  
  if (iblanked)
    {
    vtkWarningMacro("VTK does not handle blanking for image data.");
    numPts = dimensions[0] * dimensions[1] * dimensions[2];
    this->IFile->ignore(sizeof(int)*numPts);
    }
  
  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::ReadMeasuredGeometryFile(char* fileName,
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
      while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
        {
        this->ReadLine(line);
        }
      // Skip the description line.
      this->ReadLine(line);
      
      this->ReadLine(line); // "particle coordinates"
      
      this->ReadInt(&this->NumberOfMeasuredPoints);
      
      // Skip pointIds
      //this->IFile->ignore(sizeof(int)*this->NumberOfMeasuredPoints);
      // Skip xCoords
      //this->IFile->ignore(sizeof(float)*this->NumberOfMeasuredPoints);
      // Skip yCoords
      //this->IFile->ignore(sizeof(float)*this->NumberOfMeasuredPoints);
      // Skip zCoords
      //this->IFile->ignore(sizeof(float)*this->NumberOfMeasuredPoints);
      this->IFile->ignore((sizeof(float)*3 + sizeof(int))*this->NumberOfMeasuredPoints);
      
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
  
  this->ReadInt(&this->NumberOfMeasuredPoints);
  
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
int vtkEnSightGoldBinaryReader::ReadScalarsPerNode(char* fileName,
                                                   char* description,
                                                   int timeStep,
                                                   int measured,
                                                   int numberOfComponents,
                                                   int component)
{
  char line[80];
  int partId, numPts, i;
  vtkFloatArray *scalars;
  float* scalarsRead = NULL;
  vtkDataSet *output;
  
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
      
      if (measured)
        {
        this->ReadLine(line);
        output = this->GetOutput(this->NumberOfGeometryParts);
        numPts = output->GetNumberOfPoints();
        // Skip sclalars
        this->IFile->ignore(sizeof(float)*numPts);
        }
      
      while (this->ReadLine(line) &&
             strcmp(line, "part") == 0)
        {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        output = this->GetOutput(partId);
        this->ReadLine(line); // "coordinates" or "block"
        numPts = output->GetNumberOfPoints();
        // Skip sclalars
        this->IFile->ignore(sizeof(float)*numPts);
        }
      }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line
  
  if (measured)
    {
    this->ReadLine(line);
    output = this->GetOutput(this->NumberOfGeometryParts);
    numPts = output->GetNumberOfPoints();
    scalars = vtkFloatArray::New();
    scalars->SetNumberOfComponents(numberOfComponents);
    scalars->SetNumberOfTuples(numPts);
    scalarsRead = new float [numPts];
    this->ReadFloatArray(scalarsRead, numPts);
    // Why are we setting only one component here?
    for (i = 0; i < numPts; i++)
      {
      scalars->SetComponent(i, component, scalarsRead[i]);
      }
    scalars->SetName(description);
    output->GetPointData()->AddArray(scalars);
    if (!output->GetPointData()->GetScalars())
      {
      output->GetPointData()->SetScalars(scalars);
      }
    scalars->Delete();
    delete [] scalarsRead;
    if (this->IFile)
      {
      this->IFile->close();
      delete this->IFile;
      this->IFile = NULL;
      }
    return 1;
    }
  
  while (this->ReadLine(line) &&
         strcmp(line, "part") == 0)
    {
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    output = this->GetOutput(partId);
    this->ReadLine(line); // "coordinates" or "block"
    numPts = output->GetNumberOfPoints();
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
    
    scalarsRead = new float[numPts];
    this->ReadFloatArray(scalarsRead, numPts);
    
    for (i = 0; i < numPts; i++)
      {
      scalars->SetComponent(i, component, scalarsRead[i]);
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
int vtkEnSightGoldBinaryReader::ReadVectorsPerNode(char* fileName,
                                                   char* description,
                                                   int timeStep,
                                                   int measured)
{
  char line[80]; 
  int partId, numPts, i;
  vtkFloatArray *vectors;
  float tuple[3];
  float *comp1, *comp2, *comp3;
  float *vectorsRead;
  vtkDataSet *output;
  
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
      
      if (measured)
        {
        this->ReadLine(line);
        output = this->GetOutput(this->NumberOfGeometryParts);
        numPts = output->GetNumberOfPoints();
        // Skip vectors.
        this->IFile->ignore(sizeof(float)*3*numPts);
        }
      
      while (this->ReadLine(line) &&
             strcmp(line, "part") == 0)
        {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        this->ReadLine(line); // "coordinates" or "block"
        output = this->GetOutput(partId);
        numPts = output->GetNumberOfPoints();
        // Skip comp1, comp2 and comp3
        this->IFile->ignore(sizeof(float)*3*numPts);
        }
      }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line

  if (measured)
    {
    this->ReadLine(line);
    output = this->GetOutput(this->NumberOfGeometryParts);
    numPts = output->GetNumberOfPoints();
    vectors = vtkFloatArray::New();
    vectors->SetNumberOfComponents(3);
    vectors->SetNumberOfTuples(numPts);
    vectorsRead = vectors->GetPointer(0);
    this->ReadFloatArray(vectorsRead, numPts*3);
    vectors->SetName(description);
    output->GetPointData()->AddArray(vectors);
    if (!output->GetPointData()->GetVectors())
      {
      output->GetPointData()->SetVectors(vectors);
      }
    vectors->Delete();
    if (this->IFile)
      {
      this->IFile->close();
      delete this->IFile;
      this->IFile = NULL;
      }
    return 1;
    }
  
  while (this->ReadLine(line) &&
         strcmp(line, "part") == 0)
    {
    vectors = vtkFloatArray::New();
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    this->ReadLine(line); // "coordinates" or "block"
    output = this->GetOutput(partId);
    numPts = output->GetNumberOfPoints();
    vectors->SetNumberOfComponents(3);
    vectors->SetNumberOfTuples(numPts);
    comp1 = new float[numPts];
    comp2 = new float[numPts];
    comp3 = new float[numPts];
    this->ReadFloatArray(comp1, numPts);
    this->ReadFloatArray(comp2, numPts);
    this->ReadFloatArray(comp3, numPts);
    for (i = 0; i < numPts; i++)
      {
      tuple[0] = comp1[i];
      tuple[1] = comp2[i];
      tuple[2] = comp3[i];
      vectors->SetTuple(i, tuple);
      }
    vectors->SetName(description);
    output->GetPointData()->AddArray(vectors);
    if (!output->GetPointData()->GetVectors())
      {
      output->GetPointData()->SetVectors(vectors);
      }
    vectors->Delete();
    delete [] comp1;
    delete [] comp2;
    delete [] comp3;
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
int vtkEnSightGoldBinaryReader::ReadTensorsPerNode(char* fileName,
                                                   char* description,
                                                   int timeStep)
{
  char line[80];
  int partId, numPts, i;
  vtkFloatArray *tensors;
  float *comp1, *comp2, *comp3, *comp4, *comp5, *comp6;
  float tuple[6];
  vtkDataSet *output;
  
  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("NULL TensorPerNode variable file name");
    return 0;
    }
  vtkstd::string sfilename;
  if (this->FilePath)
    {
    sfilename = this->FilePath;
    sfilename += fileName;
    vtkDebugMacro("full path to tensor per node file: " << sfilename.c_str());
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
      
      while (this->ReadLine(line) &&
             strcmp(line, "part") == 0)
        {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        this->ReadLine(line); // "coordinates" or "block"
        output = this->GetOutput(partId);
        numPts = output->GetNumberOfPoints();
        // Skip over comp1, comp2, ... comp6
        this->IFile->ignore(sizeof(float)*6*numPts);
        }
      }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line

  while (this->ReadLine(line) &&
         strcmp(line, "part") == 0)
    {
    tensors = vtkFloatArray::New();
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    this->ReadLine(line); // "coordinates" or "block"
    output = this->GetOutput(partId);
    numPts = output->GetNumberOfPoints();
    tensors->SetNumberOfTuples(numPts);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numPts*6);
    comp1 = new float[numPts];
    comp2 = new float[numPts];
    comp3 = new float[numPts];
    comp4 = new float[numPts];
    comp5 = new float[numPts];
    comp6 = new float[numPts];
    this->ReadFloatArray(comp1, numPts);
    this->ReadFloatArray(comp2, numPts);
    this->ReadFloatArray(comp3, numPts);
    this->ReadFloatArray(comp4, numPts);
    this->ReadFloatArray(comp5, numPts);
    this->ReadFloatArray(comp6, numPts);
    for (i = 0; i < numPts; i++)
      {
      tuple[0] = comp1[i];
      tuple[1] = comp2[i];
      tuple[2] = comp3[i];
      tuple[3] = comp4[i];
      tuple[4] = comp5[i];
      tuple[5] = comp6[i];
      tensors->InsertTuple(i, tuple);
      }
    tensors->SetName(description);
    output->GetPointData()->AddArray(tensors);
    tensors->Delete();
    delete [] comp1;
    delete [] comp2;
    delete [] comp3;
    delete [] comp4;
    delete [] comp5;
    delete [] comp6;
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
int vtkEnSightGoldBinaryReader::ReadScalarsPerElement(char* fileName,
                                                      char* description,
                                                      int timeStep,
                                                      int numberOfComponents,
                                                      int component)
{
  char line[80];
  int partId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray *scalars;
  float *scalarsRead;
  int lineRead, elementType;
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
      lineRead = this->ReadLine(line); // "part"
      
      while (lineRead && strcmp(line, "part") == 0)
        {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        output = this->GetOutput(partId);
        numCells = output->GetNumberOfCells();
        this->ReadLine(line); // element type or "block"
        
        // need to find out from CellIds how many cells we have of this element
        // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
        if (strcmp(line, "block") == 0)
          {
          // Skip over float scalars.
          this->IFile->ignore(sizeof(float)*numCells);          
          lineRead = this->ReadLine(line);
          }
        else 
          {
          while (lineRead && strcmp(line, "part") != 0 &&
                 strncmp(line, "END TIME STEP", 13) != 0)
            {
            elementType = this->GetElementType(line);
            if (elementType == -1)
              {
              vtkErrorMacro("Unknown element type \"" << line << "\"");
              if (this->IFile)
                {
                this->IFile->close();
                delete this->IFile;
                this->IFile = NULL;
                }
              return 0;
              }
            idx = this->UnstructuredPartIds->IsId(partId);
            numCellsPerElement = this->GetCellIds(idx, elementType)->
              GetNumberOfIds();
            scalarsRead = new float[numCellsPerElement];
            this->ReadFloatArray(scalarsRead, numCellsPerElement);
            
            lineRead = this->ReadLine(line);
            delete [] scalarsRead;
            } // end while
          } // end else
        }
      }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line
  lineRead = this->ReadLine(line); // "part"
  
  while (lineRead && strcmp(line, "part") == 0)
    {
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    output = this->GetOutput(partId);
    numCells = output->GetNumberOfCells();
    this->ReadLine(line); // element type or "block"
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
    
    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
      {
      scalarsRead = new float[numCells];
      this->ReadFloatArray(scalarsRead, numCells);
      for (i = 0; i < numCells; i++)
        {
        scalars->SetComponent(i, component, scalarsRead[i]);
        }
      lineRead = this->ReadLine(line);
      delete [] scalarsRead;
      }
    else 
      {
      while (lineRead && strcmp(line, "part") != 0 &&
             strncmp(line, "END TIME STEP", 13) != 0)
        {
        elementType = this->GetElementType(line);
        if (elementType == -1)
          {
          vtkErrorMacro("Unknown element type \"" << line << "\"");
          if (this->IFile)
            {
            this->IFile->close();
            delete this->IFile;
            this->IFile = NULL;
            }
          if (component == 0)
            {
            scalars->Delete();
            }
          return 0;
          }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        scalarsRead = new float[numCellsPerElement];
        this->ReadFloatArray(scalarsRead, numCellsPerElement);
        for (i = 0; i < numCellsPerElement; i++)
          {
          scalars->SetComponent(this->GetCellIds(idx, elementType)->GetId(i),
                                component, scalarsRead[i]);
          }
        lineRead = this->ReadLine(line);
        delete [] scalarsRead;
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
  
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::ReadVectorsPerElement(char* fileName,
                                                      char* description,
                                                      int timeStep)
{
  char line[80];
  int partId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray *vectors;
  float *comp1, *comp2, *comp3;
  int lineRead, elementType;
  float tuple[3];
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
      lineRead = this->ReadLine(line); // "part"
      
      while (lineRead && strcmp(line, "part") == 0)
        {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        output = this->GetOutput(partId);
        numCells = output->GetNumberOfCells();
        this->ReadLine(line); // element type or "block"

        // need to find out from CellIds how many cells we have of this element
        // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
        if (strcmp(line, "block") == 0)
          {
          // Skip over comp1, comp2 and comp3
          this->IFile->ignore(sizeof(float)*3*numCells);
          lineRead = this->ReadLine(line);
          }
        else 
          {
          while (lineRead && strcmp(line, "part") != 0 &&
                 strncmp(line, "END TIME STEP", 13) != 0)
            {
            elementType = this->GetElementType(line);
            if (elementType == -1)
              {
              vtkErrorMacro("Unknown element type \"" << line << "\"");
              delete this->IS;
              this->IS = NULL;
              return 0;
              }
            idx = this->UnstructuredPartIds->IsId(partId);
            numCellsPerElement = this->GetCellIds(idx, elementType)->
              GetNumberOfIds();
            // Skip over comp1, comp2 and comp3
            this->IFile->ignore(sizeof(float)*3*numCellsPerElement);
            lineRead = this->ReadLine(line);
            } // end while
          } // end else
        }
      }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line
  lineRead = this->ReadLine(line); // "part"
  
  while (lineRead && strcmp(line, "part") == 0)
    {
    vectors = vtkFloatArray::New();
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    output = this->GetOutput(partId);
    numCells = output->GetNumberOfCells();
    this->ReadLine(line); // element type or "block"
    vectors->SetNumberOfComponents(3);
    vectors->SetNumberOfTuples(numCells);
    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
      {
      comp1 = new float[numCells];
      comp2 = new float[numCells];
      comp3 = new float[numCells];
      this->ReadFloatArray(comp1, numCells);
      this->ReadFloatArray(comp2, numCells);
      this->ReadFloatArray(comp3, numCells);
      for (i = 0; i < numCells; i++)
        {
        tuple[0] = comp1[i];
        tuple[1] = comp2[i];
        tuple[2] = comp3[i];
        vectors->SetTuple(i, tuple);
        }
      lineRead = this->ReadLine(line);
      delete [] comp1;
      delete [] comp2;
      delete [] comp3;
      }
    else 
      {
      while (lineRead && strcmp(line, "part") != 0 &&
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
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        comp1 = new float[numCellsPerElement];
        comp2 = new float[numCellsPerElement];
        comp3 = new float[numCellsPerElement];        
        this->ReadFloatArray(comp1, numCellsPerElement);
        this->ReadFloatArray(comp2, numCellsPerElement);
        this->ReadFloatArray(comp3, numCellsPerElement);
        for (i = 0; i < numCellsPerElement; i++)
          {
          tuple[0] = comp1[i];
          tuple[1] = comp2[i];
          tuple[2] = comp3[i];
          vectors->SetTuple(this->GetCellIds(idx, elementType)->GetId(i),
                            tuple);
          }
        lineRead = this->ReadLine(line);
        delete [] comp1;
        delete [] comp2;
        delete [] comp3;
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
  
  if (this->IFile)
    {
    this->IFile->close();
    delete this->IFile;
    this->IFile = NULL;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::ReadTensorsPerElement(char* fileName,
                                                      char* description,
                                                      int timeStep)
{
  char line[80];
  int partId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray *tensors;
  int lineRead, elementType;
  float *comp1, *comp2, *comp3, *comp4, *comp5, *comp6;
  float tuple[6];
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
    vtkDebugMacro("full path to  tensor per element file: " 
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
      lineRead = this->ReadLine(line); // "part"
      
      while (lineRead && strcmp(line, "part") == 0)
        {
        this->ReadPartId(&partId);
        partId--; // EnSight starts #ing with 1.
        output = this->GetOutput(partId);
        numCells = output->GetNumberOfCells();
        this->ReadLine(line); // element type or "block"
        
        // need to find out from CellIds how many cells we have of this element
        // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
        if (strcmp(line, "block") == 0)
          {
          // Skip comp1 - comp6
          this->IFile->ignore(sizeof(float)*6*numCells);
          lineRead = this->ReadLine(line);
          }
        else 
          {
          while (lineRead && strcmp(line, "part") != 0 &&
                 strncmp(line, "END TIME STEP", 13) != 0)
            {
            elementType = this->GetElementType(line);
            if (elementType == -1)
              {
              vtkErrorMacro("Unknown element type \"" << line << "\"");
              delete this->IS;
              this->IS = NULL;
              return 0;
              }
            idx = this->UnstructuredPartIds->IsId(partId);
            numCellsPerElement = this->GetCellIds(idx, elementType)->
              GetNumberOfIds();
            // Skip over comp1->comp6
            this->IFile->ignore(sizeof(float)*6*numCellsPerElement);
            lineRead = this->ReadLine(line);
            } // end while
          } // end else
        }
      }
    this->ReadLine(line);
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    }
  
  this->ReadLine(line); // skip the description line
  lineRead = this->ReadLine(line); // "part"
  
  while (lineRead && strcmp(line, "part") == 0)
    {
    tensors = vtkFloatArray::New();
    this->ReadPartId(&partId);
    partId--; // EnSight starts #ing with 1.
    output = this->GetOutput(partId);
    numCells = output->GetNumberOfCells();
    this->ReadLine(line); // element type or "block"
    tensors->SetNumberOfTuples(numCells);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numCells*6);
    
    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
      {
      comp1 = new float[numCells];
      comp2 = new float[numCells];
      comp3 = new float[numCells];
      comp4 = new float[numCells];
      comp5 = new float[numCells];
      comp6 = new float[numCells];
      this->ReadFloatArray(comp1, numCells);
      this->ReadFloatArray(comp2, numCells);
      this->ReadFloatArray(comp3, numCells);
      this->ReadFloatArray(comp4, numCells);
      this->ReadFloatArray(comp5, numCells);
      this->ReadFloatArray(comp6, numCells);
      for (i = 0; i < numCells; i++)
        {
        tuple[0] = comp1[i];
        tuple[1] = comp2[i];
        tuple[2] = comp3[i];
        tuple[3] = comp4[i];
        tuple[4] = comp5[i];
        tuple[5] = comp6[i];
        tensors->InsertTuple(i, tuple);
        }
      lineRead = this->ReadLine(line);
      delete [] comp1;
      delete [] comp2;
      delete [] comp3;
      delete [] comp4;
      delete [] comp5;
      delete [] comp6;
      }
    else 
      {
      while (lineRead && strcmp(line, "part") != 0 &&
             strncmp(line, "END TIME STEP", 13) != 0)
        {
        elementType = this->GetElementType(line);
        if (elementType == -1)
          {
          vtkErrorMacro("Unknown element type \"" << line << "\"");
          delete this->IS;
          this->IS = NULL;
          tensors->Delete();
          return 0;
          }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->GetCellIds(idx, elementType)->GetNumberOfIds();
        comp1 = new float[numCellsPerElement];
        comp2 = new float[numCellsPerElement];
        comp3 = new float[numCellsPerElement];
        comp4 = new float[numCellsPerElement];
        comp5 = new float[numCellsPerElement];
        comp6 = new float[numCellsPerElement];
        this->ReadFloatArray(comp1, numCellsPerElement);
        this->ReadFloatArray(comp2, numCellsPerElement);
        this->ReadFloatArray(comp3, numCellsPerElement);
        this->ReadFloatArray(comp4, numCellsPerElement);
        this->ReadFloatArray(comp5, numCellsPerElement);
        this->ReadFloatArray(comp6, numCellsPerElement);
        for (i = 0; i < numCellsPerElement; i++)
          {
          tuple[0] = comp1[i];
          tuple[1] = comp2[i];
          tuple[2] = comp3[i];
          tuple[3] = comp4[i];
          tuple[4] = comp5[i];
          tuple[5] = comp6[i];
          tensors->InsertTuple(this->GetCellIds(idx, elementType)->GetId(i),
                               tuple);
          }
        lineRead = this->ReadLine(line);
        delete [] comp1;
        delete [] comp2;
        delete [] comp3;
        delete [] comp4;
        delete [] comp5;
        delete [] comp6;
        } // end while
      } // end else
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
int vtkEnSightGoldBinaryReader::CreateUnstructuredGridOutput(int partId,
                                                             char line[80],
                                                             const char* name)
{
  int lineRead = 1;
  int i, j;
  vtkIdType *nodeIds;
  int *nodeIdList;
  int numElements;
  int idx, cellId, cellType;
  float *xCoords, *yCoords, *zCoords;
  
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
  nmArray->SetNumberOfTuples(len+1);
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
    if (strncmp(line, "coordinates", 11) == 0)
      {
      vtkDebugMacro("coordinates");
      int numPts;
      vtkPoints *points = vtkPoints::New();
      
      this->ReadInt(&numPts);
      vtkDebugMacro("num. points: " << numPts);
      
      points->Allocate(numPts);

      if (this->NodeIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numPts);
        }
      
      xCoords = new float[numPts];
      yCoords = new float[numPts];
      zCoords = new float[numPts];
      this->ReadFloatArray(xCoords, numPts);
      this->ReadFloatArray(yCoords, numPts);
      this->ReadFloatArray(zCoords, numPts);
      
      for (i = 0; i < numPts; i++)
        {
        points->InsertNextPoint(xCoords[i], yCoords[i], zCoords[i]);
        }
      
      output->SetPoints(points);
      points->Delete();
      delete [] xCoords;
      delete [] yCoords;
      delete [] zCoords;
      }
    else if (strncmp(line, "point", 5) == 0)
      {
      vtkDebugMacro("point");
      
      nodeIds = new vtkIdType[1];
      
      this->ReadInt(&numElements);
      
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }
      
      nodeIdList = new int[numElements];
      this->ReadIntArray(nodeIdList, numElements);

      for (i = 0; i < numElements; i++)
        {
        nodeIds[0] = nodeIdList[i] - 1;
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
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      nodeIdList = new int[numElements * 2];
      this->ReadIntArray(nodeIdList, numElements * 2);
      
      for (i = 0; i < numElements; i++)
        {
        for (j = 0; j < 2; j++)
          {
          nodeIds[j] = nodeIdList[2*i+j] - 1;
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

      this->ReadInt(&numElements);

      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }
      
      nodeIdList = new int[numElements*3];
      this->ReadIntArray(nodeIdList, numElements*3);
      
      for (i = 0; i < numElements; i++)
        {
        for (j = 0; j < 2; j++)
          {
          nodeIds[j] = nodeIdList[3*i+2*j] - 1;
          }
        cellId = output->InsertNextCell(VTK_LINE, 2, nodeIds);
        this->GetCellIds(idx, vtkEnSightReader::BAR3)->InsertNextId(cellId);
        }
      
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    else if (strncmp(line, "nsided", 6) == 0)
      {
      vtkDebugMacro("nsided");
      int *numNodesPerElement;
      int numNodes = 0;
      int nodeCount = 0;
      
      cellType = vtkEnSightReader::NSIDED;
      this->ReadInt(&numElements);
      
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }
      
      numNodesPerElement = new int[numElements];
      this->ReadIntArray(numNodesPerElement, numElements);
      for (i = 0; i < numElements; i++)
        {
        numNodes += numNodesPerElement[i];
        }
      nodeIdList = new int[numNodes];
      this->ReadIntArray(nodeIdList, numNodes);
      
      for (i = 0; i < numElements; i++)
        {
        nodeIds = new vtkIdType[numNodesPerElement[i]];
        for (j = 0; j < numNodesPerElement[i]; j++)
          {
          nodeIds[j] = nodeIdList[nodeCount] - 1;
          nodeCount++;
          }
        cellId = output->InsertNextCell(VTK_POLYGON, 
                                        numNodesPerElement[i], 
                                        nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        
        delete [] nodeIds;
        }
      
      delete [] nodeIdList;
      delete [] numNodesPerElement;
      }
    else if (strncmp(line, "tria3", 5) == 0 ||
             strncmp(line, "tria6", 5) == 0)
      {
      if (strncmp(line, "tria6", 5) == 0)
        {
        vtkDebugMacro("tria6");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = vtkEnSightReader::TRIA6;
        }
      else
        {
        vtkDebugMacro("tria3");
        cellType = vtkEnSightReader::TRIA3;
        }
      
      nodeIds = new vtkIdType[3];
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::TRIA6)
        {
        nodeIdList = new int[numElements*6];
        this->ReadIntArray(nodeIdList, numElements*6);
        }
      else
        {
        nodeIdList = new int[numElements*3];
        this->ReadIntArray(nodeIdList, numElements*3);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::TRIA6)
          {
          for (j = 0; j < 3; j++)
            {
            nodeIds[j] = nodeIdList[6*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 3; j++)
            {
            nodeIds[j] = nodeIdList[3*i+j] - 1;
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
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::QUAD8)
        {
        nodeIdList = new int[numElements*8];
        this->ReadIntArray(nodeIdList, numElements*8);
        }
      else
        {
        nodeIdList = new int[numElements*4];
        this->ReadIntArray(nodeIdList, numElements*4);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::QUAD8)
          {
          for (j = 0; j < 4; j++)
            {
            nodeIds[j] = nodeIdList[8*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 4; j++)
            {
            nodeIds[j] = nodeIdList[4*i+j] - 1;
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
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::TETRA10)
        {
        nodeIdList = new int[numElements*10];
        this->ReadIntArray(nodeIdList, numElements*10);
        }
      else
        {
        nodeIdList = new int[numElements*4];
        this->ReadIntArray(nodeIdList, numElements*4);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::TETRA10)
          {
          for (j = 0; j < 4; j++)
            {
            nodeIds[j] = nodeIdList[10*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 4; j++)
            {
            nodeIds[j] = nodeIdList[4*i+j] - 1;
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
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::PYRAMID13)
        {
        nodeIdList = new int[numElements*13];
        this->ReadIntArray(nodeIdList, numElements*13);
        }
      else
        {
        nodeIdList = new int[numElements*5];
        this->ReadIntArray(nodeIdList, numElements*5);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::PYRAMID13)
          {
          for (j = 0; j < 5; j++)
            {
            nodeIds[j] = nodeIdList[13*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 5; j++)
            {
            nodeIds[j] = nodeIdList[5*i+j] - 1;
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
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::HEXA20)
        {
        nodeIdList = new int[numElements*20];
        this->ReadIntArray(nodeIdList, numElements*20);
        }
      else
        {
        nodeIdList = new int[numElements*8];
        this->ReadIntArray(nodeIdList, numElements*8);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::HEXA20)
          {
          for (j = 0; j < 8; j++)
            {
            nodeIds[j] = nodeIdList[20*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 8; j++)
            {
            nodeIds[j] = nodeIdList[8*i+j] - 1;
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
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        this->IFile->ignore(sizeof(int)*numElements);
        }

      if (cellType == vtkEnSightReader::PENTA15)
        {
        nodeIdList = new int[numElements*15];
        this->ReadIntArray(nodeIdList, numElements*15);
        }
      else
        {
        nodeIdList = new int[numElements*6];
        this->ReadIntArray(nodeIdList, numElements*6);
        }
      
      for (i = 0; i < numElements; i++)
        {
        if (cellType == vtkEnSightReader::PENTA15)
          {
          for (j = 0; j < 6; j++)
            {
            nodeIds[j] = nodeIdList[15*i+j] - 1;
            }
          }
        else
          {
          for (j = 0; j < 6; j++)
            {
            nodeIds[j] = nodeIdList[6*i+j] - 1;
            }
          }
        cellId = output->InsertNextCell(VTK_WEDGE, 6, nodeIds);
        this->GetCellIds(idx, cellType)->InsertNextId(cellId);
        }
      
      delete [] nodeIds;
      delete [] nodeIdList;
      }
    else if (strncmp(line, "END TIME STEP", 13) == 0)
      {
      return 1;
      }
    else
      {
      vtkErrorMacro("undefined geometry file line");
      return -1;
      }
    lineRead = this->ReadLine(line);
    }
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::CreateStructuredGridOutput(int partId,
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
  float *xCoords, *yCoords, *zCoords;
  
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
  nmArray->SetNumberOfTuples(len+1);
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

  this->ReadIntArray(dimensions, 3);
  output->SetDimensions(dimensions);
  output->SetWholeExtent(
    0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  points->Allocate(numPts);

  xCoords = new float[numPts];
  yCoords = new float[numPts];
  zCoords = new float[numPts]; 
  this->ReadFloatArray(xCoords, numPts);
  this->ReadFloatArray(yCoords, numPts);
  this->ReadFloatArray(zCoords, numPts);
 
  for (i = 0; i < numPts; i++)
    {
    points->InsertNextPoint(xCoords[i], yCoords[i], zCoords[i]);
    }
  output->SetPoints(points);
  if (iblanked)
    {
    ((vtkStructuredGrid*)this->GetOutput(partId))->BlankingOn();
    int *iblanks = new int[numPts];
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
  delete [] xCoords;
  delete [] yCoords;
  delete [] zCoords;
  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  
  if (strcmp(line, "node_ids") == 0)
    {
    int *nodeIds = new int[numPts];
    this->ReadIntArray(nodeIds, numPts);
    lineRead = this->ReadLine(line);
    delete [] nodeIds;
    }
  if (strcmp(line, "element_ids") == 0)
    {
    int numElements = (dimensions[0] - 1) * (dimensions[1] - 1) *
      (dimensions[2] - 1);
    int *elementIds = new int[numElements];
    this->ReadIntArray(elementIds, numElements);
    lineRead = this->ReadLine(line);
    delete [] elementIds;
    }
  
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::CreateRectilinearGridOutput(int partId,
                                                            char line[80],
                                                            const char* name)
{
  char subLine[80];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkFloatArray *xCoords = vtkFloatArray::New();
  vtkFloatArray *yCoords = vtkFloatArray::New();
  vtkFloatArray *zCoords = vtkFloatArray::New();
  float *tempCoords;
  int numPts;
  
  this->NumberOfNewOutputs++;
  
  if (this->GetOutput(partId) == NULL)
    {
    vtkDebugMacro("creating new rectilinear grid output");
    vtkRectilinearGrid* rgrid = vtkRectilinearGrid::New();
    this->SetNthOutput(partId, rgrid);
    rgrid->Delete();
    }
  else if ( ! this->GetOutput(partId)->IsA("vtkRectilinearGrid"))
    {
    vtkErrorMacro("Cannot change type of output");
    this->OutputsAreValid = 0;
    return 0;
    }

  vtkRectilinearGrid* output = vtkRectilinearGrid::SafeDownCast(
    this->GetOutput(partId));    

  vtkCharArray* nmArray =  vtkCharArray::New();
  nmArray->SetName("Name");
  size_t len = strlen(name);
  nmArray->SetNumberOfTuples(len+1);
  char* copy = nmArray->GetPointer(0);
  memcpy(copy, name, len);
  copy[len] = '\0';
  output->GetFieldData()->AddArray(nmArray);
  nmArray->Delete();
  
  if (sscanf(line, " %*s %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "iblanked") == 0)
      {
      iblanked = 1;
      }
    }

  this->ReadIntArray(dimensions, 3);
  output->SetDimensions(dimensions);
  output->SetWholeExtent(
    0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  xCoords->Allocate(dimensions[0]);
  yCoords->Allocate(dimensions[1]);
  zCoords->Allocate(dimensions[2]);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  
  tempCoords = new float[dimensions[0]];
  this->ReadFloatArray(tempCoords, dimensions[0]);
  for (i = 0; i < dimensions[0]; i++)
    {
    xCoords->InsertNextTuple(&tempCoords[i]);
    }
  delete [] tempCoords;
  tempCoords = new float[dimensions[1]];
  this->ReadFloatArray(tempCoords, dimensions[1]);
  for (i = 0; i < dimensions[1]; i++)
    {
    yCoords->InsertNextTuple(&tempCoords[i]);
    }
  delete [] tempCoords;
  tempCoords = new float[dimensions[2]];
  this->ReadFloatArray(tempCoords, dimensions[2]);
  for (i = 0; i < dimensions[2]; i++)
    {
    zCoords->InsertNextTuple(&tempCoords[i]);
    }
  delete [] tempCoords;
  if (iblanked)
    {
    vtkWarningMacro("VTK does not handle blanking for rectilinear grids.");
    int *tempArray = new int[numPts];
    this->ReadIntArray(tempArray, numPts);
    delete [] tempArray;
    }
  
  output->SetXCoordinates(xCoords);  
  output->SetYCoordinates(yCoords);
  output->SetZCoordinates(zCoords);

  xCoords->Delete();
  yCoords->Delete();
  zCoords->Delete();
  
  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::CreateImageDataOutput(int partId,
                                                      char line[80],
                                                      const char* name)
{
  char subLine[80];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  float origin[3], delta[3];
  int numPts;
  
  this->NumberOfNewOutputs++;
  
  if (this->GetOutput(partId) == NULL)
    {
    vtkDebugMacro("creating new image data output");
    vtkImageData* idata = vtkImageData::New();
    this->SetNthOutput(partId, idata);
    idata->Delete();
    }
  else if ( ! this->GetOutput(partId)->IsA("vtkImageData"))
    {
    vtkErrorMacro("Cannot change output type");
    this->OutputsAreValid = 0;
    return 0;
    }

  vtkImageData* output = vtkImageData::SafeDownCast(
    this->GetOutput(partId));    

  vtkCharArray* nmArray =  vtkCharArray::New();
  nmArray->SetName("Name");
  size_t len = strlen(name);
  nmArray->SetNumberOfTuples(len+1);
  char* copy = nmArray->GetPointer(0);
  memcpy(copy, name, len);
  copy[len] = '\0';
  output->GetFieldData()->AddArray(nmArray);
  nmArray->Delete();
  
  if (sscanf(line, " %*s %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "iblanked") == 0)
      {
      iblanked = 1;
      }
    }

  this->ReadIntArray(dimensions, 3);
  output->SetDimensions(dimensions);
  output->SetWholeExtent(
    0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  this->ReadFloatArray(origin, 3);
  output->SetOrigin(origin[0], origin[1], origin[2]);
  this->ReadFloatArray(delta, 3);
  output->SetSpacing(delta[0], delta[1], delta[2]);
  
  if (iblanked)
    {
    vtkWarningMacro("VTK does not handle blanking for image data.");
    numPts = dimensions[0] * dimensions[1] * dimensions[2];
    int *tempArray = new int[numPts];
    this->ReadIntArray(tempArray, numPts);
    delete [] tempArray;
    }
  
  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

// Internal function to read in a line up to 80 characters.
// Returns zero if there was an error.
int vtkEnSightGoldBinaryReader::ReadLine(char result[80])
{
  if ( this->IFile->read(result, 80) == 0)
    {
    return 0;
    }
  
  return 1;
}

// This is half the precision of an int.
#define MAXIMUM_PART_ID 65536

// Internal function to read a single integer.
// Returns zero if there was an error.
// Sets byte order so that part id is reasonable.
int vtkEnSightGoldBinaryReader::ReadPartId(int *result)
{
  // first swap like normal.
  if (this->ReadInt(result) == 0)
    {
    return 0;
    }
  
  // second: try an experimental byte swap.
  int tmp = *result;
  vtkByteSwap::SwapVoidRange(&tmp, 1, 4);
  // Assume we do not have negative part ids or part ids over 65536.
  if (tmp >= 0 && tmp < MAXIMUM_PART_ID)
    {
    if (*result < 0 || *result >= MAXIMUM_PART_ID)
      {
      if (this->ByteOrder == FILE_LITTLE_ENDIAN)
        {
        //vtkWarningMacro("Changing EnSight reader byte order to big endian");
        this->ByteOrder = FILE_BIG_ENDIAN;
        }
      else
        {
        //vtkWarningMacro("Changing EnSight byte order to little endian");
        this->ByteOrder = FILE_LITTLE_ENDIAN;
        }
      *result = tmp;
      }
    }
    
  return 1;
}

// Internal function to read a single integer.
// Returns zero if there was an error.
int vtkEnSightGoldBinaryReader::ReadInt(int *result)
{
  if ( this->IFile->read((char*)result, sizeof(int)) == 0)
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

  return 1;
}

// Internal function to read an integer array.
// Returns zero if there was an error.
int vtkEnSightGoldBinaryReader::ReadIntArray(int *result,
                                             int numInts)
{
  if (this->IFile->read((char*)result, sizeof(int)*numInts) == 0)
    {
    vtkErrorMacro("Read failed.");
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
int vtkEnSightGoldBinaryReader::ReadFloatArray(float *result,
                                               int numFloats)
{
  if (this->IFile->read((char*)result, sizeof(float)*numFloats) == 0)
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
void vtkEnSightGoldBinaryReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
