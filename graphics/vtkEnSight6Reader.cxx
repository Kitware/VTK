/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSight6Reader.cxx
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
#include "vtkEnSight6Reader.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include <ctype.h>

//----------------------------------------------------------------------------
vtkEnSight6Reader* vtkEnSight6Reader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkEnSight6Reader");
  if(ret)
    {
    return (vtkEnSight6Reader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkEnSight6Reader;
}

//----------------------------------------------------------------------------
vtkEnSight6Reader::vtkEnSight6Reader()
{
  this->NumberOfUnstructuredPoints = 0;
  this->UnstructuredPoints = vtkPoints::New();
  this->UnstructuredNodeIds = NULL;
}

//----------------------------------------------------------------------------
vtkEnSight6Reader::~vtkEnSight6Reader()
{
  if (this->UnstructuredNodeIds)
    {
    this->UnstructuredNodeIds->Delete();
    this->UnstructuredNodeIds = NULL;
    }
  this->UnstructuredPoints->Delete();
  this->UnstructuredPoints = NULL;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadGeometryFile(char* fileName, int timeStep)
{
  char line[256], subLine[256];
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
  
  this->IS = new ifstream(line, ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << line);
    delete this->IS;
    this->IS = NULL;
    return 0;
    }
  
  this->ReadLine(line);
  
  if (sscanf(line, " %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "Binary") == 0)
      {
      vtkErrorMacro("This is a binary data set. Try "
		    << "vtkEnSight6BinaryReader.");
      return 0;
      }
    }
  
  if (this->UseFileSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      while (strncmp(line, "END TIME STEP", 13) != 0)
	{
	this->ReadLine(line);
	}
      this->ReadLine(line);
      }
    
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadNextDataLine(line);
      }
    this->ReadLine(line);
    }
  
  // Skip description line.  Using ReadLine instead of
  // ReadNextDataLine because the description line could be blank.
  this->ReadLine(line);
  
  // Read the node id and element id lines.
  this->ReadLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0)
    {
    this->UnstructuredNodeIds = vtkIdList::New();
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
  
  this->ReadNextDataLine(line);
  
  this->ReadNextDataLine(line); // "coordinates"
  this->ReadNextDataLine(line);
  this->NumberOfUnstructuredPoints = atoi(line);
  this->UnstructuredPoints->Allocate(this->NumberOfUnstructuredPoints);
  if (this->UnstructuredNodeIds)
    {
    this->UnstructuredNodeIds->Allocate(this->NumberOfUnstructuredPoints);
    }
  
  for (j = 0; j < this->NumberOfUnstructuredPoints; j++)
    {
    this->ReadNextDataLine(line);
    if (pointIdsListed)
      {
      // point ids listed
      sscanf(line, " %8d %12e %12e %12e", &pointId, &point[0],
	     &point[1], &point[2]);
      
      if (this->UnstructuredNodeIds)
	{
	this->UnstructuredNodeIds->InsertNextId(pointId-1);
	}
      this->UnstructuredPoints->InsertNextPoint(point);
      }
    else
      {
      sscanf(line, " %12e %12e %12e", &point[0], &point[1], &point[2]);
      this->UnstructuredPoints->InsertNextPoint(point);
      }
    }
  
  lineRead = this->ReadNextDataLine(line); // "part"
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    this->NumberOfGeometryParts++;
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing at 1.
    
    this->ReadLine(line); // part description line
    lineRead = this->ReadNextDataLine(line);
    
    if (strncmp(line, "block", 5) == 0)
      {
      lineRead = this->CreateStructuredGridOutput(partId, line);
      }
    else
      {
      lineRead = this->CreateUnstructuredGridOutput(partId, line);
      }
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadMeasuredGeometryFile(char* fileName, int timeStep)
{
  char line[256], subLine[256];
  vtkPoints *newPoints = NULL;
  int i;
  vtkIdType id;
  int tempId;
  float coords[3];
  vtkPolyData *geom;
  
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
  
  this->IS = new ifstream(line, ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << line);
    delete this->IS;
    this->IS = NULL;
    return 0;
    }

  this->ReadLine(line);
  
  if (sscanf(line, " %*s %s", subLine) == 1)
    {
    if (strcmp(subLine, "Binary") == 0)
      {
      vtkErrorMacro("This is a binary data set. Try "
		    << "vtkEnSight6BinaryReader.");
      return 0;
      }
    }
  
  if (this->UseFileSets)
    {
    for (i = 0; i < timeStep - 1; i++)
      {
      while (strncmp(line, "END TIME STEP", 13) != 0)
	{
	this->ReadLine(line);
	}
      this->ReadLine(line);
      }
    
    while (strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadLine(line);
      }
    this->ReadLine(line);
    }
  
  this->ReadLine(line); // "particle coordinates"
  this->ReadLine(line);
  this->NumberOfMeasuredPoints = atoi(line);
  
  if (this->GetOutput(this->NumberOfGeometryParts) == NULL)
    {
    vtkDebugMacro("creating new measured geometry output");
    vtkPolyData* pd = vtkPolyData::New();
    pd->Allocate(this->NumberOfMeasuredPoints);
    this->SetNthOutput(this->NumberOfGeometryParts, pd);
    pd->Delete();
    }
  
  this->MeasuredNodeIds->Allocate(this->NumberOfMeasuredPoints);
        
  geom = ((vtkPolyData*)this->GetOutput(this->NumberOfGeometryParts));
    
  newPoints = vtkPoints::New();
  newPoints->Allocate(this->NumberOfMeasuredPoints);
    
  for (i = 0; i < this->NumberOfMeasuredPoints; i++)
    {
    this->ReadLine(line);
    sscanf(line, " %8d %12e %12e %12e", &tempId, &coords[0], &coords[1],
	   &coords[2]);
    tempId--;
    id = tempId;
    this->MeasuredNodeIds->InsertNextId(id);
    newPoints->InsertNextPoint(coords);
    geom->InsertNextCell(VTK_VERTEX, 1, &id);
    }
  
  ((vtkPolyData*)this->GetOutput(this->NumberOfGeometryParts))->
    SetPoints(newPoints);
  
  newPoints->Delete();
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadScalarsPerNode(char* fileName, char* description,
					  int timeStep, int measured,
					  int numberOfComponents,
                                          int component)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numPts, i, j;
  vtkFloatArray *scalars;
  int numLines, moreScalars;
  float scalarsRead[6];
  
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
  
  this->IS = new ifstream(line, ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << line);
    delete this->IS;
    this->IS = NULL;
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
  
  this->ReadLine(line); // skip the description line

  this->ReadNextDataLine(line); // 1st data line or part #
  if (strncmp(line, "part", 4) != 0)
    {
    // There are 6 values per line, and one scalar per point.
    if (!measured)
      {
      numPts = this->UnstructuredPoints->GetNumberOfPoints();
      }
    else
      {
      numPts = this->GetOutput(this->NumberOfGeometryParts)->
        GetNumberOfPoints();
      }
    numLines = numPts / 6;
    moreScalars = numPts % 6;
    if (component == 0)
      {
      scalars = vtkFloatArray::New();
      scalars->SetNumberOfTuples(numPts);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numPts * numberOfComponents);
      }
    else
      {
      // It does not matter which unstructured part we get the point data from
      // because it is the same for all of them.
      partId = this->UnstructuredPartIds->GetId(0);
      scalars = (vtkFloatArray*)(this->GetOutput(partId)->GetPointData()->
	GetArray(description));
      }
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
    if (moreScalars != 0)
      {
      this->ReadLine(line);
      }
    if (!measured)
      {
      for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
        {
        partId = this->UnstructuredPartIds->GetId(i);
        if (component == 0)
          {
          scalars->SetName(description);
          this->GetOutput(partId)->GetPointData()->AddArray(scalars);
          scalars->Delete();
          }
        else
          {
          this->GetOutput(partId)->GetPointData()->AddArray(scalars);
          }
        }
      }
    else
      {
      scalars->SetName(description);
      this->GetOutput(this->NumberOfGeometryParts)->GetPointData()->
        AddArray(scalars);
      scalars->Delete();
      }
    }  

  // scalars for structured parts
  while (strncmp(line, "part", 4) == 0)
    {
    sscanf(line, " part %d", &partId);
    partId--;
    this->ReadNextDataLine(line); // block
    numPts = this->GetOutput(partId)->GetNumberOfPoints();
    numLines = numPts / 6;
    moreScalars = numPts % 6;
    if (component == 0)
      {
      scalars = vtkFloatArray::New();
      scalars->SetNumberOfTuples(numPts);
      scalars->SetNumberOfComponents(numberOfComponents);
      scalars->Allocate(numPts * numberOfComponents);
      }
    else
      {
      scalars = (vtkFloatArray*)(this->GetOutput(partId)->GetPointData()->
				 GetArray(description)); 
      }
    for (i = 0; i < numLines; i++)
      {
      this->ReadNextDataLine(line);
      sscanf(line, " %12e %12e %12e %12e %12e %12e", &scalarsRead[0],
             &scalarsRead[1], &scalarsRead[2], &scalarsRead[3],
             &scalarsRead[4], &scalarsRead[5]);
      for (j = 0; j < 6; j++)
        {
        scalars->InsertComponent(i*6 + j, component, scalarsRead[j]);        
        }
      }
    this->ReadNextDataLine(line);
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
    if (component == 0)
      {
      scalars->SetName(description);
      this->GetOutput(partId)->GetPointData()->AddArray(scalars);
      scalars->Delete();
      }
    else
      {
      this->GetOutput(partId)->GetPointData()->AddArray(scalars);
      }
    this->ReadNextDataLine(line);
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadVectorsPerNode(char* fileName, char* description,
					  int timeStep, int measured)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numPts, i, j, k;
  vtkFloatArray *vectors;
  int numLines, moreVectors;
  float vector1[3], vector2[3], values[6];
  
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
  
  this->IS = new ifstream(line, ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << line);
    delete this->IS;
    this->IS = NULL;
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

  this->ReadLine(line); // skip the description line

  this->ReadNextDataLine(line); // 1st data line or part #
  if (strncmp(line, "part", 4) != 0)
    {
    // There are 6 values per line, and 3 values (or 1 vector) per point.
    if (!measured)
      {
      numPts = this->UnstructuredPoints->GetNumberOfPoints();
      }
    else
      {
      numPts = this->GetOutput(this->NumberOfGeometryParts)->
        GetNumberOfPoints();
      }
    numLines = numPts / 2;
    moreVectors = ((numPts * 3) % 6) / 3;
    vectors = vtkFloatArray::New();
    vectors->SetNumberOfTuples(numPts);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numPts*3);
    for (i = 0; i < numLines; i++)
      {
      sscanf(line, " %12e %12e %12e %12e %12e %12e", &vector1[0], &vector1[1],
             &vector1[2], &vector2[0], &vector2[1], &vector2[2]);
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
    if (moreVectors != 0)
      {
      this->ReadLine(line);
      }
    if (!measured)
      {
      for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
        {
        partId = this->UnstructuredPartIds->GetId(i);
        vectors->SetName(description);
        this->GetOutput(partId)->GetPointData()->AddArray(vectors);
        }
      }
    else
      {
      vectors->SetName(description);
      this->GetOutput(this->NumberOfGeometryParts)->GetPointData()->
        AddArray(vectors);
      }
    vectors->Delete();
    }

  // vectors for structured parts
  while (strncmp(line, "part", 4) == 0)
    {
    sscanf(line, " part %d", &partId);
    partId--;
    this->ReadNextDataLine(line); // block
    numPts = this->GetOutput(partId)->GetNumberOfPoints();
    numLines = numPts / 6;
    moreVectors = numPts % 6;
    vectors = vtkFloatArray::New();
    vectors->SetNumberOfTuples(numPts);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numPts*3);

    for (k = 0; k < 3; k++)
      {
      for (i = 0; i < numLines; i++)
        {
        this->ReadNextDataLine(line);
        sscanf(line, " %12e %12e %12e %12e %12e %12e", &values[0], &values[1],
               &values[2], &values[3], &values[4], &values[5]);
        for (j = 0; j < 6; j++)
          {
          vectors->InsertComponent(i*6 + j, k, values[j]);
          }
        }
      
      if (moreVectors)
        {
        this->ReadNextDataLine(line);
        strcpy(formatLine, "");
        strcpy(tempLine, "");
        for (j = 0; j < moreVectors; j++)
          {
          strcat(formatLine, " %12e");
          sscanf(line, formatLine, &values[j]);
          vectors->InsertComponent(i*6 + j, k, values[j]);
          strcat(tempLine, " %*12e");
          strcpy(formatLine, tempLine);
          }
        }
      }
    vectors->SetName(description);
    this->GetOutput(partId)->GetPointData()->AddArray(vectors);
    vectors->Delete();
    
    this->ReadNextDataLine(line);
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadTensorsPerNode(char* fileName, char* description,
					  int timeStep)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numPts, i, j, k;
  vtkFloatArray *tensors;
  int numLines, moreTensors;
  float tensor[6], values[6];
  int lineRead;
  
  // Initialize
  //
  if (!fileName)
    {
    vtkErrorMacro("NULL TensorSymmPerNode variable file name");
    return 0;
    }
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, fileName);
    vtkDebugMacro("full path to tensor symm per node file: " << line);
    }
  else
    {
    strcpy(line, fileName);
    }
  
  this->IS = new ifstream(line, ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << line);
    delete this->IS;
    this->IS = NULL;
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

  this->ReadLine(line); // skip the description line

  lineRead = this->ReadNextDataLine(line); // 1st data line or part #
  if (strncmp(line, "part", 4) != 0)
    {
    // There are 6 values per line, and 6 values (or 1 tensor) per point.
    numPts = this->UnstructuredPoints->GetNumberOfPoints();
    numLines = numPts;
    tensors = vtkFloatArray::New();
    tensors->SetNumberOfTuples(numPts);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numPts*6);
    for (i = 0; i < numLines; i++)
      {
      sscanf(line, " %12e %12e %12e %12e %12e %12e", &tensor[0], &tensor[1],
             &tensor[2], &tensor[3], &tensor[4], &tensor[5]);
      tensors->InsertTuple(i, tensor);
      lineRead = this->ReadNextDataLine(line);
      }

    for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
      {
      partId = this->UnstructuredPartIds->GetId(i);
      tensors->SetName(description);
      this->GetOutput(partId)->GetPointData()->AddArray(tensors);
      }
    tensors->Delete();
    }

  // vectors for structured parts
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    sscanf(line, " part %d", &partId);
    partId--;
    this->ReadNextDataLine(line); // block
    numPts = this->GetOutput(partId)->GetNumberOfPoints();
    numLines = numPts / 6;
    moreTensors = numPts % 6;
    tensors = vtkFloatArray::New();
    tensors->SetNumberOfTuples(numPts);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numPts*6);

    for (k = 0; k < 6; k++)
      {
      for (i = 0; i < numLines; i++)
        {
        lineRead = this->ReadNextDataLine(line);
        sscanf(line, " %12e %12e %12e %12e %12e %12e", &values[0], &values[1],
               &values[2], &values[3], &values[4], &values[5]);
        for (j = 0; j < 6; j++)
          {
          tensors->InsertComponent(i*6 + j, k, values[j]);
          }
        }
      
      if (moreTensors)
        {
        lineRead = this->ReadNextDataLine(line);
        strcpy(formatLine, "");
        strcpy(tempLine, "");
        for (j = 0; j < moreTensors; j++)
          {
          strcat(formatLine, " %12e");
          sscanf(line, formatLine, &values[j]);
          tensors->InsertComponent(i*6 + j, k, values[j]);
          strcat(tempLine, " %*12e");
          strcpy(formatLine, tempLine);
          }
        }
      }
    tensors->SetName(description);
    this->GetOutput(partId)->GetPointData()->AddArray(tensors);
    tensors->Delete();
    lineRead = this->ReadNextDataLine(line);
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadScalarsPerElement(char* fileName, char* description,
					     int timeStep,
					     int numberOfComponents,
					     int component)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray *scalars;
  int lineRead, elementType;
  float scalarsRead[6];
  int numLines, moreScalars;
  
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
  
  this->IS = new ifstream(line, ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << line);
    delete this->IS;
    this->IS = NULL;
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

  this->ReadLine(line); // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing with 1.
    numCells = this->GetOutput(partId)->GetNumberOfCells();
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
      scalars = (vtkFloatArray*)(this->GetOutput(partId)->GetCellData()->
				 GetArray(description));
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
        sscanf(line, " %12e %12e %12e %12e %12e %12e", &scalarsRead[0],
	       &scalarsRead[1], &scalarsRead[2], &scalarsRead[3],
	       &scalarsRead[4], &scalarsRead[5]);
        for (j = 0; j < 6; j++)
          {
          scalars->InsertComponent(i*6 + j, component, scalarsRead[j]);
          }
        }
      lineRead = this->ReadNextDataLine(line);
      
      if (moreScalars)
        {
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
        }
      }
    else 
      {
      while (lineRead && strncmp(line, "part", 4) != 0)
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
        numCellsPerElement = this->CellIds[idx][elementType]->GetNumberOfIds();
        numLines = numCellsPerElement / 6;
        moreScalars = numCellsPerElement % 6;
        for (i = 0; i < numLines; i++)
          {
          this->ReadNextDataLine(line);
          sscanf(line, " %12e %12e %12e %12e %12e %12e", &scalarsRead[0],
		 &scalarsRead[1], &scalarsRead[2], &scalarsRead[3],
		 &scalarsRead[4], &scalarsRead[5]);
          for (j = 0; j < 6; j++)
            {
            scalars->InsertComponent(this->CellIds[idx][elementType]->
				     GetId(i*6 + j), component,
				     scalarsRead[j]);
            }
          }
        if (moreScalars)
          {
          lineRead = this->ReadNextDataLine(line);
          strcpy(formatLine, "");
          strcpy(tempLine, "");
          for (j = 0; j < moreScalars; j++)
            {
            strcat(formatLine, " %12e");
            sscanf(line, formatLine, &scalarsRead[j]);
            scalars->InsertComponent(this->CellIds[idx][elementType]->
				     GetId(i*6 + j), component,
				     scalarsRead[j]);
            strcat(tempLine, " %*12e");
            strcpy(formatLine, tempLine);
            }
          }
        lineRead = this->ReadNextDataLine(line);
        } // end while
      } // end else
    if (component == 0)
      {
      scalars->SetName(description);
      this->GetOutput(partId)->GetCellData()->AddArray(scalars);
      scalars->Delete();
      }
    else
      {
      this->GetOutput(partId)->GetCellData()->AddArray(scalars);
      }
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadVectorsPerElement(char* fileName, char* description,
					     int timeStep)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numCells, numCellsPerElement, i, j, k, idx;
  vtkFloatArray *vectors;
  int lineRead, elementType;
  float values[6], vector1[3], vector2[3];
  int numLines, moreVectors;
  
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
  
  this->IS = new ifstream(line, ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << line);
    delete this->IS;
    this->IS = NULL;
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

  this->ReadLine(line); // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    vectors = vtkFloatArray::New();
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing with 1.
    numCells = this->GetOutput(partId)->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    vectors->SetNumberOfTuples(numCells);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numCells*3);
    
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
          sscanf(line, " %12e %12e %12e %12e %12e %12e", &values[0],
		 &values[1], &values[2], &values[3], &values[4], &values[5]);
          for (j = 0; j < 6; j++)
            {
            vectors->InsertComponent(i*6 + j, k, values[j]);
            }
          }
        if (moreVectors)
          {
          this->ReadNextDataLine(line);
          strcpy(formatLine, "");
          strcpy(tempLine, "");
          for (j = 0; j < moreVectors; j++)
            {
            strcat(formatLine, " %12e");
            sscanf(line, formatLine, &values[j]);
            vectors->InsertComponent(i*6 + j, k, values[j]);
            strcat(tempLine, " %*12e");
            strcpy(formatLine, tempLine);
            }
          }
        }
      lineRead = this->ReadNextDataLine(line);
      }
    else 
      {
      while (lineRead && strncmp(line, "part", 4) != 0)
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
        numCellsPerElement = this->CellIds[idx][elementType]->GetNumberOfIds();
        numLines = numCellsPerElement / 2;
        moreVectors = ((numCellsPerElement*3) % 6) / 3;
        
        for (i = 0; i < numLines; i++)
          {
          this->ReadNextDataLine(line);
          sscanf(line, " %12e %12e %12e %12e %12e %12e", &vector1[0],
		 &vector1[1], &vector1[2], &vector2[0], &vector2[1],
		 &vector2[2]);
          vectors->InsertTuple(this->CellIds[idx][elementType]->GetId(2*i),
                               vector1);
          vectors->InsertTuple(this->CellIds[idx][elementType]->GetId(2*i + 1),
                               vector2);
          }
        if (moreVectors)
          {
          lineRead = this->ReadNextDataLine(line);
          strcpy(formatLine, "");
          strcpy(tempLine, "");
          for (j = 0; j < moreVectors; j++)
            {
            strcat(formatLine, " %12e %12e %12e");
            sscanf(line, formatLine, &vector1[0], &vector1[1], &vector1[2]);
            vectors->InsertTuple(this->CellIds[idx][elementType]->
				 GetId(2*i + j), vector1);
            strcat(tempLine, " %*12e %*12e %*12e");
            strcpy(formatLine, tempLine);
            }
          }
        lineRead = this->ReadNextDataLine(line);
        } // end while
      } // end else
    vectors->SetName(description);
    this->GetOutput(partId)->GetCellData()->AddArray(vectors);
    vectors->Delete();
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadTensorsPerElement(char* fileName, char* description,
					     int timeStep)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numCells, numCellsPerElement, i, j, k, idx;
  vtkFloatArray *tensors;
  int lineRead, elementType;
  float values[6], tensor[6];
  int numLines, moreTensors;
  
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
  
  this->IS = new ifstream(line, ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << line);
    delete this->IS;
    this->IS = NULL;
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

  this->ReadLine(line); // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    tensors = vtkFloatArray::New();
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing with 1.
    numCells = this->GetOutput(partId)->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    tensors->SetNumberOfTuples(numCells);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numCells*6);
    
    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
      {
      numLines = numCells / 6;
      moreTensors = numCells % 6;
      
      for (k = 0; k < 6; k++)
        {
        for (i = 0; i < numLines; i++)
          {
          this->ReadNextDataLine(line);
          sscanf(line, " %12e %12e %12e %12e %12e %12e", &values[0],
		 &values[1], &values[2], &values[3], &values[4], &values[5]);
          for (j = 0; j < 6; j++)
            {
            tensors->InsertComponent(i*6 + j, k, values[j]);
            }
          }
        if (moreTensors)
          {
          this->ReadNextDataLine(line);
          strcpy(formatLine, "");
          strcpy(tempLine, "");
          for (j = 0; j < moreTensors; j++)
            {
            strcat(formatLine, " %12e");
            sscanf(line, formatLine, &values[j]);
            tensors->InsertComponent(i*6 + j, k, values[j]);
            strcat(tempLine, " %*12e");
            strcpy(formatLine, tempLine);
            }
          }
        }
      lineRead = this->ReadNextDataLine(line);
      }
    else 
      {
      while (lineRead && strncmp(line, "part", 4) != 0)
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
        numCellsPerElement = this->CellIds[idx][elementType]->GetNumberOfIds();
        numLines = numCellsPerElement;
        
        for (i = 0; i < numLines; i++)
          {
          this->ReadNextDataLine(line);
          sscanf(line, " %12e %12e %12e %12e %12e %12e", &tensor[0],
		 &tensor[1], &tensor[2], &tensor[3], &tensor[4], &tensor[5]);
          tensors->InsertTuple(this->CellIds[idx][elementType]->GetId(i),
                               tensor);
          }
        lineRead = this->ReadNextDataLine(line);
        } // end while
      } // end else
    tensors->SetName(description);
    this->GetOutput(partId)->GetCellData()->AddArray(tensors);
    tensors->Delete();
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::CreateUnstructuredGridOutput(int partId,
                                                    char line[256])
{
  int lineRead = 1;
  char subLine[256];
  int i, j;
  vtkIdType *nodeIds;
  int *intIds;
  int numElements;
  int idx, cellId, cellType, testId;
  
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
    if (strncmp(line, "point", 5) == 0)
      {
      vtkDebugMacro("point");
      
      nodeIds = new vtkIdType[1];        
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*s %s", subLine) == 1)
          {
          // element ids listed
          // EnSight ids start at 1
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(atoi(subLine) - 1);
            }
          else
            {
            nodeIds[0] = atoi(subLine) - 1;
            }
          }
        else
          {
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(atoi(line) - 1);
            }
          else
            {
            nodeIds[0] = atoi(line) - 1;
            }
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_VERTEX, 1, nodeIds);
        this->CellIds[idx][VTK_ENSIGHT_POINT]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      delete [] nodeIds;
      }
    else if (strncmp(line, "bar2", 4) == 0)
      {
      vtkDebugMacro("bar2");
      
      nodeIds = new vtkIdType[2];
      intIds = new int[2];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*d %d %d", &intIds[0], &intIds[1]) != 2)
          {
          sscanf(line, " %d %d", &intIds[0], &intIds[1]);
          }
        for (j = 0; j < 2; j++)
          {
          intIds[j]--;
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 2; j++)
            {
            intIds[j] = this->UnstructuredNodeIds->IsId(intIds[j]);
            }
          }
        for (j = 0; j < 2; j++)
          {
          nodeIds[j] = intIds[j];
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_LINE, 2, nodeIds);
        this->CellIds[idx][VTK_ENSIGHT_BAR2]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      delete [] nodeIds;
      delete [] intIds;
      }
    else if (strncmp(line, "bar3", 4) == 0)
      {
      vtkDebugMacro("bar3");
      vtkWarningMacro("Only vertex nodes of this element will be read.");
      nodeIds = new vtkIdType[2];
      intIds = new int[2];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*d %d %*d %d", &intIds[0], &intIds[1]) != 2)
          {
          sscanf(line, " %d %*d %d", &intIds[0], &intIds[1]);
          }
        for (j = 0; j < 2; j++)
          {
          intIds[j]--;
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 2; j++)
            {
            intIds[j] = this->UnstructuredNodeIds->IsId(intIds[j]);
            }
          }
        for (j = 0; j < 2; j++)
          {
          nodeIds[j] = intIds[j];
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_LINE, 2, nodeIds);
        this->CellIds[idx][VTK_ENSIGHT_BAR3]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      delete [] nodeIds;
      delete [] intIds;
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
      intIds = new int[3];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (!(sscanf(line, " %*d %d %d %d", &intIds[0], &intIds[1],
                     &intIds[2]) == 3 &&
              cellType == VTK_ENSIGHT_TRIA3) &&
            !(sscanf(line, " %*d %d %d %d %*d %*d %d", &intIds[0], &intIds[1],
                     &intIds[2], &testId) == 4 &&
              cellType == VTK_ENSIGHT_TRIA6))
          {
          sscanf(line, " %d %d %d", &intIds[0], &intIds[1], &intIds[2]);
          }
        for (j = 0; j < 3; j++)
          {
          intIds[j]--;
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 3; j++)
            {
            intIds[j] = this->UnstructuredNodeIds->IsId(intIds[j]);
            }
          }
        for (j = 0; j < 3; j++)
          {
          nodeIds[j] = intIds[j];
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_TRIANGLE, 3, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      delete [] nodeIds;
      delete [] intIds;
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
      intIds = new int[4];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (!(sscanf(line, " %*d %d %d %d %d", &intIds[0], &intIds[1],
                     &intIds[2], &intIds[3]) == 4 &&
              cellType == VTK_ENSIGHT_QUAD4) &&
            !(sscanf(line, " %*d %d %d %d %d %*d %*d %*d %d", &intIds[0],
                     &intIds[1], &intIds[2], &intIds[3], &testId) == 5 &&
              cellType == VTK_ENSIGHT_QUAD8))
          {
          sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
                 &intIds[3]);
          }
        for (j = 0; j < 4; j++)
          {
          intIds[j]--;
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 4; j++)
            {
            intIds[j] = this->UnstructuredNodeIds->IsId(intIds[j]);
            }
          }
        for (j = 0; j < 4; j++)
          {
          nodeIds[j] = intIds[j];
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_QUAD, 4, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      delete [] nodeIds;
      delete [] intIds;
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
      intIds = new int[4];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (!(sscanf(line, " %*d %d %d %d %d", &intIds[0], &intIds[1],
                     &intIds[2], &intIds[3]) == 4 &&
              cellType == VTK_ENSIGHT_TETRA4) &&
            !(sscanf(line, " %*d %d %d %d %d %*d %*d %*d %*d %*d %d",
                     &intIds[0], &intIds[1], &intIds[2], &intIds[3],
                     &testId) == 5 &&
              cellType == VTK_ENSIGHT_TETRA10))
          {
          sscanf(line, " %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
                 &intIds[3]);
          }
        for (j = 0; j < 4; j++)
          {
          intIds[j]--;
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 4; j++)
            {
            intIds[j] = this->UnstructuredNodeIds->IsId(intIds[j]);
            }
          }
        for (j = 0; j < 4; j++)
          {
          nodeIds[j] = intIds[j];
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_TETRA, 4, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      delete [] nodeIds;
      delete [] intIds;
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
      intIds = new int[5];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (!(sscanf(line, " %*d %d %d %d %d %d", &intIds[0], &intIds[1],
                     &intIds[2], &intIds[3], &intIds[4]) == 5 &&
              cellType == VTK_ENSIGHT_PYRAMID5) &&
            !(sscanf(line, " %*d %d %d %d %d %d %*d %*d %*d %*d %*d %*d %*d %d",
                     &intIds[0], &intIds[1], &intIds[2], &intIds[3],
                     &intIds[4], &testId) == 6 &&
              cellType == VTK_ENSIGHT_PYRAMID13))
          {
          sscanf(line, " %d %d %d %d %d", &intIds[0], &intIds[1], &intIds[2],
                 &intIds[3], &intIds[4]);
          }
        for (j = 0; j < 5; j++)
          {
          intIds[j]--;
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 5; j++)
            {
            intIds[j] = this->UnstructuredNodeIds->IsId(intIds[j]);
            }
          }
        for (j = 0; j < 5; j++)
          {
          nodeIds[j] = intIds[j];
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_PYRAMID, 5, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      delete [] nodeIds;
      delete [] intIds;
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
      intIds = new int[8];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (!(sscanf(line, " %*d %d %d %d %d %d %d %d %d", &intIds[0],
                     &intIds[1], &intIds[2], &intIds[3], &intIds[4],
                     &intIds[5], &intIds[6], &intIds[7]) == 8 &&
              cellType == VTK_ENSIGHT_HEXA8) &&
            !(sscanf(line, " %*d %d %d %d %d %d %d %d %d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %d",
                     &intIds[0], &intIds[1], &intIds[2], &intIds[3],
                     &intIds[4], &intIds[5], &intIds[6], &intIds[7],
                     &testId) == 9 &&
              cellType == VTK_ENSIGHT_HEXA20))
          {
          sscanf(line, " %d %d %d %d %d %d %d %d", &intIds[0], &intIds[1],
                 &intIds[2], &intIds[3], &intIds[4], &intIds[5],
                 &intIds[6], &intIds[7]);
          }
        for (j = 0; j < 8; j++)
          {
          intIds[j]--;
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 8; j++)
            {
            intIds[j] = this->UnstructuredNodeIds->IsId(intIds[j]);
            }
          }
        for (j = 0; j < 8; j++)
          {
          nodeIds[j] = intIds[j];
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_HEXAHEDRON, 8, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      delete [] nodeIds;
      delete [] intIds;
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
      intIds = new int[6];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (!(sscanf(line, " %*d %d %d %d %d %d %d", &intIds[0],
                     &intIds[1], &intIds[2], &intIds[3], &intIds[4],
                     &intIds[5]) == 6 &&
              cellType == VTK_ENSIGHT_PENTA6) &&
            !(sscanf(line, " %*d %d %d %d %d %d %d %*d %*d %*d %*d %*d %*d %*d %*d %d",
                     &intIds[0], &intIds[1], &intIds[2], &intIds[3],
                     &intIds[4], &intIds[5], &testId) == 7 &&
              cellType == VTK_ENSIGHT_PENTA15))
          {
          sscanf(line, " %d %d %d %d %d %d", &intIds[0], &intIds[1],
                 &intIds[2], &intIds[3], &intIds[4], &intIds[5]);
          }
        for (j = 0; j < 6; j++)
          {
          intIds[j]--;
          }
        if (this->UnstructuredNodeIds)
          {
          for (j = 0; j < 6; j++)
            {
            intIds[j] = this->UnstructuredNodeIds->IsId(intIds[j]);
            }
          }
        for (j = 0; j < 6; j++)
          {
          nodeIds[j] = intIds[j];
          }
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_WEDGE, 6, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      delete [] nodeIds;
      delete [] intIds;
      }
    }

  ((vtkUnstructuredGrid*)this->GetOutput(partId))->
    SetPoints(this->UnstructuredPoints);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::CreateStructuredGridOutput(int partId,
                                                  char line[256])
{
  char subLine[256];
  char formatLine[256], tempLine[256];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int i, j;
  vtkPoints *points = vtkPoints::New();
  float point[3];
  int numPts, numLines, moreCoords;
  float coords[6];
  int iblanks[10];
  
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
      ((vtkStructuredGrid*)this->GetOutput(partId))->BlankingOn();
      }
    }

  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &dimensions[0], &dimensions[1], &dimensions[2]);
  ((vtkStructuredGrid*)this->GetOutput(partId))->SetDimensions(dimensions);
  ((vtkStructuredGrid*)this->GetOutput(partId))->
    SetWholeExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  numPts = dimensions[0] * dimensions[1] * dimensions[2];
  points->Allocate(numPts);
  
  numLines = numPts / 6; // integer division
  moreCoords = numPts % 6;
  
  for (i = 0; i < numLines; i++)
    {
    this->ReadNextDataLine(line);
    sscanf(line, " %12e %12e %12e %12e %12e %12e", &coords[0], &coords[1],
	   &coords[2], &coords[3], &coords[4], &coords[5]);
    for (j = 0; j < 6; j++)
      {
      points->InsertNextPoint(coords[j], 0.0, 0.0);
      }
    }
  if (moreCoords != 0)
    {
    this->ReadNextDataLine(line);
    strcpy(formatLine, "");
    strcpy(tempLine, "");
    for (j = 0; j < moreCoords; j++)
      {
      strcat(formatLine, " %12e");
      sscanf(line, formatLine, &coords[j]);
      points->InsertNextPoint(coords[j], 0.0, 0.0);
      strcat(tempLine, " %*12e");
      strcpy(formatLine, tempLine);
      }
    }
  for (i = 0; i < numLines; i++)
    {
    this->ReadNextDataLine(line);
    sscanf(line, " %12e %12e %12e %12e %12e %12e", &coords[0], &coords[1],
	   &coords[2], &coords[3], &coords[4], &coords[5]);
    for (j = 0; j < 6; j++)
      {
      points->GetPoint(i*6+j, point);
      points->SetPoint(i*6+j, point[0], coords[j], point[2]);
      }
    }
  if (moreCoords != 0)
    {
    this->ReadNextDataLine(line);
    strcpy(formatLine, "");
    strcpy(tempLine, "");
    for (j = 0; j < moreCoords; j++)
      {
      strcat(formatLine, " %12e");
      sscanf(line, formatLine, &coords[j]);
      points->GetPoint(i*6+j, point);
      points->SetPoint(i*6+j, point[0], coords[j], point[2]);
      strcat(tempLine, " %*12e");
      strcpy(formatLine, tempLine);
      }
    }
  for (i = 0; i < numLines; i++)
    {
    this->ReadNextDataLine(line);
    sscanf(line, " %12e %12e %12e %12e %12e %12e", &coords[0], &coords[1],
	   &coords[2], &coords[3], &coords[4], &coords[5]);
    for (j = 0; j < 6; j++)
      {
      points->GetPoint(i*6+j, point);
      points->SetPoint(i*6+j, point[0], point[1], coords[j]);
      }
    }
  if (moreCoords != 0)
    {
    this->ReadNextDataLine(line);
    strcpy(formatLine, "");
    strcpy(tempLine, "");
    for (j = 0; j < moreCoords; j++)
      {
      strcat(formatLine, " %12e");
      sscanf(line, formatLine, &coords[j]);
      points->GetPoint(i*6+j, point);
      points->SetPoint(i*6+j, point[0], point[1], coords[j]);
      strcat(tempLine, " %*12e");
      strcpy(formatLine, tempLine);
      }
    }
  
  numLines = numPts / 10;
  if (iblanked)
    {
    for (i = 0; i < numLines; i++)
      {
      this->ReadNextDataLine(line);
      sscanf(line, " %d %d %d %d %d %d %d %d %d %d", &iblanks[0], &iblanks[1],
             &iblanks[2], &iblanks[3], &iblanks[4], &iblanks[5], &iblanks[6],
             &iblanks[7], &iblanks[8], &iblanks[9]);
      for (j = 0; j < 10; j++)
        {
        if (!iblanks[j])
          {
          ((vtkStructuredGrid*)this->GetOutput(partId))->BlankPoint(i*numLines+j);
          }
        }
      }
    }
  
  ((vtkStructuredGrid*)this->GetOutput(partId))->SetPoints(points);
  points->Delete();
  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}
