/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightGoldBinaryReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkEnSightGoldBinaryReader.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include "vtkByteSwap.h"
#include <ctype.h>

//----------------------------------------------------------------------------
vtkEnSightGoldBinaryReader* vtkEnSightGoldBinaryReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkEnSightGoldBinaryReader");
  if(ret)
    {
    return (vtkEnSightGoldBinaryReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkEnSightGoldBinaryReader;
}

//----------------------------------------------------------------------------
vtkEnSightGoldBinaryReader::vtkEnSightGoldBinaryReader()
{
  this->IFile = NULL;
}

//----------------------------------------------------------------------------
vtkEnSightGoldBinaryReader::~vtkEnSightGoldBinaryReader()
{
  if (this->IFile)
    {
    fclose(this->IFile);
    this->IFile = NULL;
    }
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
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, fileName);
    vtkDebugMacro("full path to geometry file: " << line);
    }
  else
    {
    strcpy(line, fileName);
    }
  
  this->IFile = fopen(line, "rb");
  if (this->IFile == NULL)
    {
    vtkErrorMacro("Unable to open file: " << line);
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
    // There should be a better way to advance the file pointer.
    float *tempArray = new float[6];
    this->ReadFloatArray(tempArray, 6);
    delete [] tempArray;
    lineRead = this->ReadLine(line); // "part"
    }
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    this->ReadInt(&partId);
    partId--; // EnSight starts #ing at 1.
    
    this->ReadLine(line); // part description line
    lineRead = this->ReadLine(line);
    
    if (strncmp(line, "block", 5) == 0)
      {
      if (sscanf(line, " %*s %s", subLine) == 1)
        {
        if (strcmp(subLine, "rectilinear") == 0)
          {
          // block rectilinear
          lineRead = this->CreateRectilinearGridOutput(partId, line);
          }
        else if (strcmp(subLine, "uniform") == 0)
          {
          // block uniform
          lineRead = this->CreateImageDataOutput(partId, line);
          }
        else
          {
          // block iblanked
          lineRead = this->CreateStructuredGridOutput(partId, line);
          }
        }
      else
        {
        // block
        lineRead = this->CreateStructuredGridOutput(partId, line);
        }
      }
    else
      {
      lineRead = this->CreateUnstructuredGridOutput(partId, line);
      if (lineRead < 0)
        {
        fclose(this->IFile);
        this->IFile = NULL;
        return 0;
        }
      }
    }
  
  fclose(this->IFile);
  this->IFile = NULL;
  return 1;
}

//----------------------------------------------------------------------------
void vtkEnSightGoldBinaryReader::SkipTimeStep()
{
  char line[80], subLine[80];
  int lineRead;

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
    float *tempArray = new float[6];
    this->ReadFloatArray(tempArray, 6);
    delete [] tempArray;
    lineRead = this->ReadLine(line); // "part"
    }
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
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
        fclose(this->IFile);
        this->IFile = NULL;
        }
      }
    }
  this->ReadLine(line); // END TIME STEP
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::SkipStructuredGrid(char line[256])
{  
  char subLine[80];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int numPts;
  float *xCoords, *yCoords, *zCoords;
  
  if (sscanf(line, " %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "iblanked") == 0)
      {
      iblanked = 1;
      }
    }

  this->ReadIntArray(dimensions, 3);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];

  xCoords = new float[numPts];
  yCoords = new float[numPts];
  zCoords = new float[numPts]; 
  this->ReadFloatArray(xCoords, numPts);
  this->ReadFloatArray(yCoords, numPts);
  this->ReadFloatArray(zCoords, numPts);
 
  if (iblanked)
    {
    int *iblanks = new int[numPts];
    this->ReadIntArray(iblanks, numPts);
    delete [] iblanks;
    }
  
  delete [] xCoords;
  delete [] yCoords;
  delete [] zCoords;

  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::SkipUnstructuredGrid(char line[256])
{
  int lineRead = 1;
  int i;
  int *elementIdList, *nodeIdList;
  int numElements;
  int cellType;
  float *xCoords, *yCoords, *zCoords;
  
  while(lineRead && strncmp(line, "part", 4) != 0)
    {
    if (strncmp(line, "coordinates", 11) == 0)
      {
      vtkDebugMacro("coordinates");
      int numPts;
      
      this->ReadInt(&numPts);
      vtkDebugMacro("num. points: " << numPts);
      
      if (this->NodeIdsListed)
        {
        int *tempArray = new int[numPts];
        this->ReadIntArray(tempArray, numPts);
        delete [] tempArray;
        }
      
      xCoords = new float[numPts];
      yCoords = new float[numPts];
      zCoords = new float[numPts];
      this->ReadFloatArray(xCoords, numPts);
      this->ReadFloatArray(yCoords, numPts);
      this->ReadFloatArray(zCoords, numPts);
      
      delete [] xCoords;
      delete [] yCoords;
      delete [] zCoords;
      }
    else if (strncmp(line, "point", 5) == 0)
      {
      vtkDebugMacro("point");
      
      this->ReadInt(&numElements);
      
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }
      
      nodeIdList = new int[numElements];
      this->ReadIntArray(nodeIdList, numElements);

      delete [] nodeIdList;
      }
    else if (strncmp(line, "bar2", 4) == 0)
      {
      vtkDebugMacro("bar2");
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      nodeIdList = new int[numElements * 2];
      this->ReadIntArray(nodeIdList, numElements * 2);

      delete [] nodeIdList;
      }
    else if (strncmp(line, "bar3", 4) == 0)
      {
      vtkDebugMacro("bar3");
      vtkWarningMacro("Only vertex nodes of this element will be read.");

      this->ReadInt(&numElements);

      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }
      
      nodeIdList = new int[numElements*3];
      this->ReadIntArray(nodeIdList, numElements*3);
      
      delete [] nodeIdList;
      }
    else if (strncmp(line, "nsided", 6) == 0)
      {
      vtkDebugMacro("nsided");
      int *numNodesPerElement;
      int numNodes = 0;
      
      cellType = VTK_ENSIGHT_NSIDED;
      this->ReadInt(&numElements);
      
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }
      
      numNodesPerElement = new int[numElements];
      this->ReadIntArray(numNodesPerElement, numElements);
      for (i = 0; i < numElements; i++)
        {
        numNodes += numNodesPerElement[i];
        }
      nodeIdList = new int[numNodes];
      this->ReadIntArray(nodeIdList, numNodes);
      
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
        cellType = VTK_ENSIGHT_TRIA6;
        }
      else
        {
        vtkDebugMacro("tria3");
        cellType = VTK_ENSIGHT_TRIA3;
        }
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_TRIA6)
        {
        nodeIdList = new int[numElements*6];
        this->ReadIntArray(nodeIdList, numElements*6);
        }
      else
        {
        nodeIdList = new int[numElements*3];
        this->ReadIntArray(nodeIdList, numElements*3);
        }
      
      delete [] nodeIdList;
      }
    else if (strncmp(line, "quad4", 5) == 0 ||
             strncmp(line, "quad8", 5) == 0)
      {
      if (strncmp(line, "quad8", 5) == 0)
        {
        vtkDebugMacro("quad8");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = VTK_ENSIGHT_QUAD8;
        }
      else
        {
        vtkDebugMacro("quad4");
        cellType = VTK_ENSIGHT_QUAD4;
        }
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_QUAD8)
        {
        nodeIdList = new int[numElements*8];
        this->ReadIntArray(nodeIdList, numElements*8);
        }
      else
        {
        nodeIdList = new int[numElements*4];
        this->ReadIntArray(nodeIdList, numElements*4);
        }
      
      delete [] nodeIdList;
      }
    else if (strncmp(line, "tetra4", 6) == 0 ||
             strncmp(line, "tetra10", 7) == 0)
      {
      if (strncmp(line, "tetra10", 7) == 0)
        {
        vtkDebugMacro("tetra10");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = VTK_ENSIGHT_TETRA10;
        }
      else
        {
        vtkDebugMacro("tetra4");
        cellType = VTK_ENSIGHT_TETRA4;
        }
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_TETRA10)
        {
        nodeIdList = new int[numElements*10];
        this->ReadIntArray(nodeIdList, numElements*10);
        }
      else
        {
        nodeIdList = new int[numElements*4];
        this->ReadIntArray(nodeIdList, numElements*4);
        }
      
      delete [] nodeIdList;
      }
    else if (strncmp(line, "pyramid5", 8) == 0 ||
             strncmp(line, "pyramid13", 9) == 0)
      {
      if (strncmp(line, "pyramid13", 9) == 0)
        {
        vtkDebugMacro("pyramid13");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = VTK_ENSIGHT_PYRAMID13;
        }
      else
        {
        vtkDebugMacro("pyramid5");
        cellType = VTK_ENSIGHT_PYRAMID5;
        }
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_PYRAMID13)
        {
        nodeIdList = new int[numElements*13];
        this->ReadIntArray(nodeIdList, numElements*13);
        }
      else
        {
        nodeIdList = new int[numElements*5];
        this->ReadIntArray(nodeIdList, numElements*5);
        }
      
      delete [] nodeIdList;
      }
    else if (strncmp(line, "hexa8", 5) == 0 ||
             strncmp(line, "hexa20", 6) == 0)
      {
      if (strncmp(line, "hexa20", 6) == 0)
        {
        vtkDebugMacro("hexa20");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = VTK_ENSIGHT_HEXA20;
        }
      else
        {
        vtkDebugMacro("hexa8");
        cellType = VTK_ENSIGHT_HEXA8;
        }
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_HEXA20)
        {
        nodeIdList = new int[numElements*20];
        this->ReadIntArray(nodeIdList, numElements*20);
        }
      else
        {
        nodeIdList = new int[numElements*8];
        this->ReadIntArray(nodeIdList, numElements*8);
        }
      
      delete [] nodeIdList;
      }
    else if (strncmp(line, "penta6", 6) == 0 ||
             strncmp(line, "penta15", 7) == 0)
      {
      if (strncmp(line, "penta15", 7) == 0)
        {
        vtkDebugMacro("penta15");
        vtkWarningMacro("Only vertex nodes of this element will be read.");
        cellType = VTK_ENSIGHT_PENTA15;
        }
      else
        {
        vtkDebugMacro("penta6");
        cellType = VTK_ENSIGHT_PENTA6;
        }
      
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_PENTA15)
        {
        nodeIdList = new int[numElements*15];
        this->ReadIntArray(nodeIdList, numElements*15);
        }
      else
        {
        nodeIdList = new int[numElements*6];
        this->ReadIntArray(nodeIdList, numElements*6);
        }
      
      delete [] nodeIdList;
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
  float *tempCoords;
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
  
  tempCoords = new float[dimensions[0]];
  this->ReadFloatArray(tempCoords, dimensions[0]);
  delete [] tempCoords;

  tempCoords = new float[dimensions[1]];
  this->ReadFloatArray(tempCoords, dimensions[1]);
  delete [] tempCoords;

  tempCoords = new float[dimensions[2]];
  this->ReadFloatArray(tempCoords, dimensions[2]);
  delete [] tempCoords;

  if (iblanked)
    {
    vtkWarningMacro("VTK does not handle blanking for rectilinear grids.");
    int *tempArray = new int[numPts];
    this->ReadIntArray(tempArray, numPts);
    delete [] tempArray;
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
    int *tempArray = new int[numPts];
    this->ReadIntArray(tempArray, numPts);
    delete [] tempArray;
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
  
  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("A MeasuredFileName must be specified in the case file.");
    return 0;
    }
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, fileName);
    vtkDebugMacro("full path to measured geometry file: " << line);
    }
  else
    {
    strcpy(line, fileName);
    }
  
  this->IFile = fopen(line, "rb");
  if (this->IFile == NULL)
    {
    vtkErrorMacro("Unable to open file: " << line);
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
  
  fclose(this->IFile);
  this->IFile = NULL;
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
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, fileName);
    vtkDebugMacro("full path to scalar per node file: " << line);
    }
  else
    {
    strcpy(line, fileName);
    }
  
  this->IFile = fopen(line, "rb");
  if (this->IFile == NULL)
    {
    vtkErrorMacro("Unable to open file: " << line);
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
	scalarsRead = new float[numPts];
	this->ReadFloatArray(scalarsRead, numPts);
	delete [] scalarsRead;
	}
      
      while (this->ReadLine(line) &&
	     strcmp(line, "part") == 0)
	{
	this->ReadInt(&partId);
	partId--; // EnSight starts #ing with 1.
        output = this->GetOutput(partId);
	this->ReadLine(line); // "coordinates" or "block"
	numPts = output->GetNumberOfPoints();
	scalarsRead = new float[numPts];
	this->ReadFloatArray(scalarsRead, numPts);
	
	delete [] scalarsRead;
	}
      
      this->ReadLine(line); // END TIME STEP
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
    scalars->SetNumberOfTuples(numPts);
    scalars->SetNumberOfComponents(numberOfComponents);
    scalars->Allocate(numPts * numberOfComponents);
    scalarsRead = new float [numPts];
    this->ReadFloatArray(scalarsRead, numPts);
    for (i = 0; i < numPts; i++)
      {
      scalars->InsertComponent(i, component, scalarsRead[i]);
      }
    scalars->SetName(description);
    output->GetPointData()->AddArray(scalars);
    if (!output->GetPointData()->GetScalars())
      {
      output->GetPointData()->SetScalars(scalars);
      }
    scalars->Delete();
    delete [] scalarsRead;
    fclose(this->IFile);
    this->IFile = NULL;
    return 1;
    }
  
  while (this->ReadLine(line) &&
         strcmp(line, "part") == 0)
    {
    this->ReadInt(&partId);
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
    }
  
  fclose(this->IFile);
  this->IFile = NULL;
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
  float tuple[3], vector[3];
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
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, fileName);
    vtkDebugMacro("full path to vector per node file: " << line);
    }
  else
    {
    strcpy(line, fileName);
    }
  
  this->IFile = fopen(line, "rb");
  if (this->IFile == NULL)
    {
    vtkErrorMacro("Unable to open file: " << line);
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
	vectorsRead = new float[numPts*3];
	this->ReadFloatArray(vectorsRead, numPts*3);
	delete [] vectorsRead;
	}
      
      while (this->ReadLine(line) &&
	     strcmp(line, "part") == 0)
	{
	this->ReadInt(&partId);
	partId--; // EnSight starts #ing with 1.
	this->ReadLine(line); // "coordinates" or "block"
	output = this->GetOutput(partId);
	numPts = output->GetNumberOfPoints();
	comp1 = new float[numPts];
	comp2 = new float[numPts];
	comp3 = new float[numPts];
	this->ReadFloatArray(comp1, numPts);
	this->ReadFloatArray(comp2, numPts);
	this->ReadFloatArray(comp3, numPts);
	}
      
      this->ReadLine(line); // END TIME STEP
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
    fclose(this->IFile);
    this->IFile = NULL;
    return 1;
    }
  
  while (this->ReadLine(line) &&
         strcmp(line, "part") == 0)
    {
    vectors = vtkFloatArray::New();
    this->ReadInt(&partId);
    partId--; // EnSight starts #ing with 1.
    this->ReadLine(line); // "coordinates" or "block"
    output = this->GetOutput(partId);
    numPts = output->GetNumberOfPoints();
    vectors->SetNumberOfTuples(numPts);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numPts*3);
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
      vectors->InsertTuple(i, tuple);
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

  fclose(this->IFile);
  this->IFile = NULL;

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
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, fileName);
    vtkDebugMacro("full path to tensor per node file: " << line);
    }
  else
    {
    strcpy(line, fileName);
    }
  
  this->IFile = fopen(line, "rb");
  if (this->IFile)
    {
    vtkErrorMacro("Unable to open file: " << line);
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
	this->ReadInt(&partId);
	partId--; // EnSight starts #ing with 1.
	this->ReadLine(line); // "coordinates" or "block"
        output = this->GetOutput(partId);
	numPts = output->GetNumberOfPoints();
	comp1 = new float[numPts];
	comp2 = new float[numPts];
	comp3 = new float[numPts];
	comp4 = new float[numPts];
	comp5 = new float[numPts];
	comp6 = new float[numPts];
	this->ReadFloatArray(comp1, numPts);
	this->ReadFloatArray(comp1, numPts);
	this->ReadFloatArray(comp1, numPts);
	this->ReadFloatArray(comp1, numPts);
	this->ReadFloatArray(comp1, numPts);
	this->ReadFloatArray(comp1, numPts);
	delete [] comp1;
	delete [] comp2;
	delete [] comp3;
	delete [] comp4;
	delete [] comp5;
	delete [] comp6;
	}
      
      this->ReadLine(line); // END TIME STEP
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
    this->ReadInt(&partId);
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

  fclose(this->IFile);
  this->IFile = NULL;

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
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, fileName);
    vtkDebugMacro("full path to scalar per element file: " << line);
    }
  else
    {
    strcpy(line, fileName);
    }
  
  this->IFile = fopen(line, "rb");
  if (this->IFile == NULL)
    {
    vtkErrorMacro("Unable to open file: " << line);
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
	this->ReadInt(&partId);
	partId--; // EnSight starts #ing with 1.
        output = this->GetOutput(partId);
	numCells = output->GetNumberOfCells();
	this->ReadLine(line); // element type or "block"
	
	// need to find out from CellIds how many cells we have of this element
	// type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
	if (strcmp(line, "block") == 0)
	  {
	  scalarsRead = new float[numCells];
	  this->ReadFloatArray(scalarsRead, numCells);
	  
	  lineRead = this->ReadLine(line);
	  delete [] scalarsRead;
	  }
	else 
	  {
	  while (lineRead && strcmp(line, "part") != 0)
	    {
	    elementType = this->GetElementType(line);
	    if (elementType == -1)
	      {
	      vtkErrorMacro("Unknown element type");
	      fclose(this->IFile);
	      this->IFile = NULL;
	      return 0;
	      }
	    idx = this->UnstructuredPartIds->IsId(partId);
	    numCellsPerElement = this->CellIds[idx][elementType]->
	      GetNumberOfIds();
	    scalarsRead = new float[numCellsPerElement];
	    this->ReadFloatArray(scalarsRead, numCellsPerElement);
	    
	    lineRead = this->ReadLine(line);
	    delete [] scalarsRead;
	    } // end while
	  } // end else
	}
      this->ReadLine(line); // END TIME STEP
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
    this->ReadInt(&partId);
    partId--; // EnSight starts #ing with 1.
    output = this->GetOutput(partId);
    numCells = output->GetNumberOfCells();
    this->ReadLine(line); // element type or "block"
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
      scalarsRead = new float[numCells];
      this->ReadFloatArray(scalarsRead, numCells);
      for (i = 0; i < numCells; i++)
        {
        scalars->InsertComponent(i, component, scalarsRead[i]);
        }
      lineRead = this->ReadLine(line);
      delete [] scalarsRead;
      }
    else 
      {
      while (lineRead && strcmp(line, "part") != 0)
        {
        elementType = this->GetElementType(line);
        if (elementType == -1)
          {
          vtkErrorMacro("Unknown element type");
          fclose(this->IFile);
          this->IFile = NULL;
          return 0;
          }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->CellIds[idx][elementType]->GetNumberOfIds();
        scalarsRead = new float[numCellsPerElement];
        this->ReadFloatArray(scalarsRead, numCellsPerElement);
        for (i = 0; i < numCellsPerElement; i++)
          {
          scalars->InsertComponent(this->CellIds[idx][elementType]->GetId(i),
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
  
  fclose(this->IFile);
  this->IFile = NULL;
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
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, fileName);
    vtkDebugMacro("full path to vector per element file: " << line);
    }
  else
    {
    strcpy(line, fileName);
    }
  
  this->IFile = fopen(line, "rb");
  if (this->IFile == NULL)
    {
    vtkErrorMacro("Unable to open file: " << line);
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
	this->ReadInt(&partId);
	partId--; // EnSight starts #ing with 1.
        output = this->GetOutput(partId);
	numCells = output->GetNumberOfCells();
	this->ReadLine(line); // element type or "block"

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

	  lineRead = this->ReadLine(line);
	  delete [] comp1;
	  delete [] comp2;
	  delete [] comp3;
	  }
	else 
	  {
	  while (lineRead && strcmp(line, "part") != 0)
	    {
	    elementType = this->GetElementType(line);
	    if (elementType == -1)
	      {
	      vtkErrorMacro("Unknown element type");
	      delete this->IS;
	      this->IS = NULL;
	      return 0;
	      }
	    idx = this->UnstructuredPartIds->IsId(partId);
	    numCellsPerElement = this->CellIds[idx][elementType]->
	      GetNumberOfIds();
	    comp1 = new float[numCellsPerElement];
	    comp2 = new float[numCellsPerElement];
	    comp3 = new float[numCellsPerElement];        
	    this->ReadFloatArray(comp1, numCellsPerElement);
	    this->ReadFloatArray(comp2, numCellsPerElement);
	    this->ReadFloatArray(comp3, numCellsPerElement);

	    lineRead = this->ReadLine(line);
	    delete [] comp1;
	    delete [] comp2;
	    delete [] comp3;
	    } // end while
	  } // end else
	}
      
      this->ReadLine(line); // END TIME STEP
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
    this->ReadInt(&partId);
    partId--; // EnSight starts #ing with 1.
    output = this->GetOutput(partId);
    numCells = output->GetNumberOfCells();
    this->ReadLine(line); // element type or "block"
    vectors->SetNumberOfTuples(numCells);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numCells*3);
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
        vectors->InsertTuple(i, tuple);
        }
      lineRead = this->ReadLine(line);
      delete [] comp1;
      delete [] comp2;
      delete [] comp3;
      }
    else 
      {
      while (lineRead && strcmp(line, "part") != 0)
        {
        elementType = this->GetElementType(line);
        if (elementType == -1)
          {
          vtkErrorMacro("Unknown element type");
          delete this->IS;
          this->IS = NULL;
          return 0;
          }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->CellIds[idx][elementType]->GetNumberOfIds();
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
          vectors->InsertTuple(this->CellIds[idx][elementType]->GetId(i),
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
  
  fclose(this->IFile);
  this->IFile = NULL;
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
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, fileName);
    vtkDebugMacro("full path to tensor per element file: " << line);
    }
  else
    {
    strcpy(line, fileName);
    }
  
  this->IFile = fopen(line, "rb");
  if (this->IFile)
    {
    vtkErrorMacro("Unable to open file: " << line);
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
	this->ReadInt(&partId);
	partId--; // EnSight starts #ing with 1.
        output = this->GetOutput(partId);
	numCells = output->GetNumberOfCells();
	this->ReadLine(line); // element type or "block"
	
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
	  while (lineRead && strcmp(line, "part") != 0)
	    {
	    elementType = this->GetElementType(line);
	    if (elementType == -1)
	      {
	      vtkErrorMacro("Unknown element type");
	      delete [] this->IS;
	      this->IS = NULL;
	      return 0;
	      }
	    idx = this->UnstructuredPartIds->IsId(partId);
	    numCellsPerElement = this->CellIds[idx][elementType]->
	      GetNumberOfIds();
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

	    lineRead = this->ReadLine(line);
	    delete [] comp1;
	    delete [] comp2;
	    delete [] comp3;
	    delete [] comp4;
	    delete [] comp5;
	    delete [] comp6;
	    } // end while
	  } // end else
	}
      
      this->ReadLine(line); // END TIME STEP
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
    this->ReadInt(&partId);
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
      while (lineRead && strcmp(line, "part") != 0)
        {
        elementType = this->GetElementType(line);
        if (elementType == -1)
          {
          vtkErrorMacro("Unknown element type");
          delete [] this->IS;
          this->IS = NULL;
          return 0;
          }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->CellIds[idx][elementType]->GetNumberOfIds();
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
          tensors->InsertTuple(this->CellIds[idx][elementType]->GetId(i),
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
  
  fclose(this->IFile);
  this->IFile = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::CreateUnstructuredGridOutput(int partId,
                                                             char line[80])
{
  int lineRead = 1;
  int i, j;
  vtkIdType *nodeIds;
  int *elementIdList, *nodeIdList;
  int numElements;
  int idx, cellId, cellType;
  float *xCoords, *yCoords, *zCoords;
  
  if (this->GetOutput(partId) == NULL)
    {
    vtkDebugMacro("creating new unstructured output");
    vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::New();
    this->SetNthOutput(partId, ugrid);
    ugrid->Delete();
    
    this->UnstructuredPartIds->InsertNextId(partId);
    }
  ((vtkUnstructuredGrid *)this->GetOutput(partId))->Allocate(1000);
  
  idx = this->UnstructuredPartIds->IsId(partId);

  if (this->CellIds == NULL)
    {
    this->CellIds = new vtkIdList **[16];
    }
  
  this->CellIds[idx] = new vtkIdList *[16];
  for (i = 0; i < 16; i++)
    {
    this->CellIds[idx][i] = vtkIdList::New();
    }
  
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
        int *tempArray = new int[numPts];
        this->ReadIntArray(tempArray, numPts);
        delete [] tempArray;
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
      
      ((vtkUnstructuredGrid*)this->GetOutput(partId))->SetPoints(points);
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
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }
      
      nodeIdList = new int[numElements];
      this->ReadIntArray(nodeIdList, numElements);

      for (i = 0; i < numElements; i++)
        {
        nodeIds[0] = nodeIdList[i] - 1;
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_VERTEX, 1, nodeIds);
        this->CellIds[idx][VTK_ENSIGHT_POINT]->InsertNextId(cellId);
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
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      nodeIdList = new int[numElements * 2];
      this->ReadIntArray(nodeIdList, numElements * 2);
      
      for (i = 0; i < numElements; i++)
        {
        for (j = 0; j < 2; j++)
          {
          nodeIds[j] = nodeIdList[2*i+j] - 1;
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_LINE, 2, nodeIds);
        this->CellIds[idx][VTK_ENSIGHT_BAR2]->InsertNextId(cellId);
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
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }
      
      nodeIdList = new int[numElements*3];
      this->ReadIntArray(nodeIdList, numElements*3);
      
      for (i = 0; i < numElements; i++)
        {
        for (j = 0; j < 2; j++)
          {
          nodeIds[j] = nodeIdList[3*i+2*j] - 1;
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_LINE, 2, nodeIds);
        this->CellIds[idx][VTK_ENSIGHT_BAR3]->InsertNextId(cellId);
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
      
      cellType = VTK_ENSIGHT_NSIDED;
      this->ReadInt(&numElements);
      
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
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
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_POLYGON, numNodesPerElement[i], nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        
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
        cellType = VTK_ENSIGHT_TRIA6;
        }
      else
        {
        vtkDebugMacro("tria3");
        cellType = VTK_ENSIGHT_TRIA3;
        }
      
      nodeIds = new vtkIdType[3];
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_TRIA6)
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
        if (cellType == VTK_ENSIGHT_TRIA6)
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
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_TRIANGLE, 3, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
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
        cellType = VTK_ENSIGHT_QUAD8;
        }
      else
        {
        vtkDebugMacro("quad4");
        cellType = VTK_ENSIGHT_QUAD4;
        }
      
      nodeIds = new vtkIdType[4];
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_QUAD8)
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
        if (cellType == VTK_ENSIGHT_QUAD8)
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
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_QUAD, 4, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
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
        cellType = VTK_ENSIGHT_TETRA10;
        }
      else
        {
        vtkDebugMacro("tetra4");
        cellType = VTK_ENSIGHT_TETRA4;
        }
      
      nodeIds = new vtkIdType[4];
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_TETRA10)
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
        if (cellType == VTK_ENSIGHT_TETRA10)
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
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_TETRA, 4, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
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
        cellType = VTK_ENSIGHT_PYRAMID13;
        }
      else
        {
        vtkDebugMacro("pyramid5");
        cellType = VTK_ENSIGHT_PYRAMID5;
        }
      
      nodeIds = new vtkIdType[5];
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_PYRAMID13)
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
        if (cellType == VTK_ENSIGHT_PYRAMID13)
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
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_PYRAMID, 5, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
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
        cellType = VTK_ENSIGHT_HEXA20;
        }
      else
        {
        vtkDebugMacro("hexa8");
        cellType = VTK_ENSIGHT_HEXA8;
        }
      
      nodeIds = new vtkIdType[8];
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_HEXA20)
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
        if (cellType == VTK_ENSIGHT_HEXA20)
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
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_HEXAHEDRON, 8, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
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
        cellType = VTK_ENSIGHT_PENTA15;
        }
      else
        {
        vtkDebugMacro("penta6");
        cellType = VTK_ENSIGHT_PENTA6;
        }
      
      nodeIds = new vtkIdType[6];
      this->ReadInt(&numElements);
      if (this->ElementIdsListed)
        {
        elementIdList = new int[numElements];
        this->ReadIntArray(elementIdList, numElements);
        delete [] elementIdList;
        }

      if (cellType == VTK_ENSIGHT_PENTA15)
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
        if (cellType == VTK_ENSIGHT_PENTA15)
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
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_WEDGE, 6, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        }
      
      delete [] nodeIds;
      delete [] nodeIdList;
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
                                                           char line[80])
{
  char subLine[80];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkPoints *points = vtkPoints::New();
  int numPts;
  float *xCoords, *yCoords, *zCoords;
  
  if (this->GetOutput(partId) == NULL)
    {
    vtkDebugMacro("creating new structured grid output");
    vtkStructuredGrid* sgrid = vtkStructuredGrid::New();
    this->SetNthOutput(partId, sgrid);
    sgrid->Delete();
    }
  
  if (sscanf(line, " %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "iblanked") == 0)
      {
      iblanked = 1;
      }
    }

  this->ReadIntArray(dimensions, 3);
  ((vtkStructuredGrid*)this->GetOutput(partId))->SetDimensions(dimensions);
  ((vtkStructuredGrid*)this->GetOutput(partId))->
    SetWholeExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
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
  ((vtkStructuredGrid*)this->GetOutput(partId))->SetPoints(points);
  if (iblanked)
    {
    ((vtkStructuredGrid*)this->GetOutput(partId))->BlankingOn();
    int *iblanks = new int[numPts];
    this->ReadIntArray(iblanks, numPts);
    
    for (i = 0; i < numPts; i++)
      {
      if (!iblanks[i])
        {
        ((vtkStructuredGrid*)this->GetOutput(partId))->BlankPoint(i);
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
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::CreateRectilinearGridOutput(int partId,
                                                            char line[80])
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
  
  if (this->GetOutput(partId) == NULL)
    {
    vtkDebugMacro("creating new structured grid output");
    vtkRectilinearGrid* rgrid = vtkRectilinearGrid::New();
    this->SetNthOutput(partId, rgrid);
    rgrid->Delete();
    }
  
  if (sscanf(line, " %*s %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "iblanked") == 0)
      {
      iblanked = 1;
      }
    }

  this->ReadIntArray(dimensions, 3);
  ((vtkRectilinearGrid*)this->GetOutput(partId))->SetDimensions(dimensions);
  ((vtkRectilinearGrid*)this->GetOutput(partId))->
    SetWholeExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
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
  
  ((vtkRectilinearGrid*)this->GetOutput(partId))->SetXCoordinates(xCoords);  
  ((vtkRectilinearGrid*)this->GetOutput(partId))->SetYCoordinates(yCoords);
  ((vtkRectilinearGrid*)this->GetOutput(partId))->SetZCoordinates(zCoords);
  
  // reading next line to check for EOF
  lineRead = this->ReadLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldBinaryReader::CreateImageDataOutput(int partId,
						      char line[80])
{
  char subLine[80];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  float origin[3], delta[3];
  int numPts;
  
  if (this->GetOutput(partId) == NULL)
    {
    vtkDebugMacro("creating new structured grid output");
    vtkImageData* idata = vtkImageData::New();
    this->SetNthOutput(partId, idata);
    idata->Delete();
    }
  
  if (sscanf(line, " %*s %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "iblanked") == 0)
      {
      iblanked = 1;
      }
    }

  this->ReadIntArray(dimensions, 3);
  ((vtkImageData*)this->GetOutput(partId))->SetDimensions(dimensions);
  ((vtkImageData*)this->GetOutput(partId))->
    SetWholeExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  this->ReadFloatArray(origin, 3);
  ((vtkImageData*)this->GetOutput(partId))->SetOrigin(origin[0], origin[1],
                                                      origin[2]);
  this->ReadFloatArray(delta, 3);
  ((vtkImageData*)this->GetOutput(partId))->SetSpacing(delta[0], delta[1],
                                                       delta[2]);
  
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
  fread(result, sizeof(char), 80, this->IFile);

  if (feof(this->IFile) || ferror(this->IFile))
    {
    return 0;
    }
  
  return 1;
}

// Internal function to read a single integer.
// Returns zero if there was an error.
int vtkEnSightGoldBinaryReader::ReadInt(int *result)
{
  fread(result, sizeof(int), 1, this->IFile);
  if (feof(this->IFile) || ferror(this->IFile))
    {
    return 0;
    }
  vtkByteSwap::Swap4BE(result);
  
  return 1;
}

// Internal function to read an integer array.
// Returns zero if there was an error.
int vtkEnSightGoldBinaryReader::ReadIntArray(int *result,
                                             int numInts)
{
  int i;
  
  fread(result, sizeof(int), numInts, this->IFile);
  if (feof(this->IFile) || ferror(this->IFile))
    {
    return 0;
    }
  for (i = 0; i < numInts; i++)
    {
    vtkByteSwap::Swap4BE(&result[i]);
    }
  
  return 1;
}

// Internal function to read a float array.
// Returns zero if there was an error.
int vtkEnSightGoldBinaryReader::ReadFloatArray(float *result,
                                               int numFloats)
{
  int i;
  
  fread(result, sizeof(float), numFloats, this->IFile);
  if (feof(this->IFile) || ferror(this->IFile))
    {
    return 0;
    }
  for (i = 0; i < numFloats; i++)
    {
    vtkByteSwap::Swap4BE(&result[i]);
    }
  
  return 1;
}
