/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightGoldReader.cxx
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
#include "vtkEnSightGoldReader.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include <ctype.h>

//----------------------------------------------------------------------------
vtkEnSightGoldReader* vtkEnSightGoldReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkEnSightGoldReader");
  if(ret)
    {
    return (vtkEnSightGoldReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkEnSightGoldReader;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadGeometryFile(char* fileName, int timeStep)
{
  char line[256], subLine[256];
  int partId, i;
  int lineRead;
  
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
  
  this->ReadNextDataLine(line);
  sscanf(line, " %*s %s", subLine);
  if (strcmp(subLine, "Binary") == 0)
    {
    vtkErrorMacro("This is a binary data set. Try "
                  <<"vtkEnSightGoldBinaryReader.");
    return 0;
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
    
    while(strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadNextDataLine(line);
      }
    this->ReadLine(line);
    }

  // Skip description lines.  Using ReadLine instead of
  // ReadNextDataLine because the description line could be blank.
  this->ReadNextDataLine(line);
  // Skip the node id and element id lines.
  this->ReadNextDataLine(line);
  this->ReadNextDataLine(line);
  
  lineRead = this->ReadNextDataLine(line); // "extents" or "part"
  if (strcmp(line, "extents") == 0)
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
    
    this->ReadNextDataLine(line); // part description line
    lineRead = this->ReadNextDataLine(line);
    
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
        delete this->IS;
        this->IS = NULL;
        return 0;
        }
      }
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadMeasuredGeometryFile(char* fileName,
						   int timeStep)
{
  char line[256], subLine[256];
  vtkPoints *newPoints;
  int i;
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

  // Skip the description line.  Using ReadLine instead of ReadNextDataLine
  // because the description line could be blank.
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
    
    while(strncmp(line, "BEGIN TIME STEP", 15) != 0)
      {
      this->ReadNextDataLine(line);
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
    
    this->MeasuredNodeIds->Allocate(this->NumberOfMeasuredPoints);
    }

  geom = ((vtkPolyData*)this->GetOutput(this->NumberOfGeometryParts));

  newPoints = vtkPoints::New();
  newPoints->Allocate(this->NumberOfMeasuredPoints);
  
  for (i = 0; i < this->NumberOfMeasuredPoints; i++)
    {
    this->ReadLine(line);
    sscanf(line, " %8d %12e %12e %12e", &id, &coords[0], &coords[1],
           &coords[2]);
    id--;
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
int vtkEnSightGoldReader::ReadScalarsPerNode(char* fileName, char* description,
					     int timeStep, int measured,
					     int numberOfComponents,
					     int component)
{
  char line[256], formatLine[256], tempLine[256];
  int partId, numPts, i, j, numLines, moreScalars;
  vtkFloatArray *scalars;
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
  
  this->ReadNextDataLine(line); // skip the description line
  
  if (measured)
    {
    numPts = this->GetOutput(this->NumberOfGeometryParts)->GetNumberOfPoints();
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
    this->GetOutput(this->NumberOfGeometryParts)->GetPointData()->
      AddArray(scalars);
    scalars->Delete();
    delete this->IS;
    this->IS = NULL;
    return 1;
    }
  
  while (this->ReadNextDataLine(line) &&
         strcmp(line, "part") == 0)
    {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    this->ReadNextDataLine(line); // "coordinates" or "block"
    numPts = this->GetOutput(partId)->GetNumberOfPoints();
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

    for (i = 0; i < numPts; i++)
      {
      this->ReadNextDataLine(line);
      scalars->InsertComponent(i, component, atof(line));
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
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadVectorsPerNode(char* fileName, char* description,
					     int timeStep, int measured)
{
  char line[256], formatLine[256], tempLine[256]; 
  int partId, numPts, i, j, numLines, moreVectors;
  vtkFloatArray *vectors;
  float vector1[3], vector2[3];
  
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

  this->ReadNextDataLine(line); // skip the description line

  if (measured)
    {
    this->ReadNextDataLine(line);
    numPts = this->GetOutput(this->NumberOfGeometryParts)->GetNumberOfPoints();
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
    vectors->SetName(description);
    this->GetOutput(this->NumberOfGeometryParts)->GetPointData()->
      AddArray(vectors);
    vectors->Delete();
    delete this->IS;
    this->IS = NULL;
    return 1;
    }
  
  while (this->ReadNextDataLine(line) &&
         strcmp(line, "part") == 0)
    {
    vectors = vtkFloatArray::New();
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    this->ReadNextDataLine(line); // "coordinates" or "block"
    numPts = this->GetOutput(partId)->GetNumberOfPoints();
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
    this->GetOutput(partId)->GetPointData()->AddArray(vectors);
    vectors->Delete();
    }

  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadTensorsPerNode(char* fileName, char* description,
					     int timeStep)
{
  char line[256];
  int partId, numPts, i, j;
  vtkFloatArray *tensors;
  
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

  this->ReadNextDataLine(line); // skip the description line

  while (this->ReadNextDataLine(line) &&
         strcmp(line, "part") == 0)
    {
    tensors = vtkFloatArray::New();
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    this->ReadNextDataLine(line); // "coordinates" or "block"
    numPts = this->GetOutput(partId)->GetNumberOfPoints();
    tensors->SetNumberOfTuples(numPts);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numPts*6);
    for (i = 0; i < 6; i++)
      {
      for (j = 0; j < numPts; j++)
        {
        this->ReadNextDataLine(line);
        tensors->InsertComponent(j, i, atof(line));
        }
      }
    tensors->SetName(description);
    this->GetOutput(partId)->GetPointData()->AddArray(tensors);
    tensors->Delete();
    }

  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadScalarsPerElement(char* fileName,
						char* description,
						int timeStep,
						int numberOfComponents,
						int component)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray *scalars;
  int lineRead, elementType;
  float scalar;
  
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

  this->ReadNextDataLine(line); // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"
  
  while (lineRead && strcmp(line, "part") == 0)
    {
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
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
        for (i = 0; i < numCellsPerElement; i++)
          {
          this->ReadNextDataLine(line);
          scalar = atof(line);
          scalars->InsertComponent(this->CellIds[idx][elementType]->GetId(i),
                                   component, scalar);
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
int vtkEnSightGoldReader::ReadVectorsPerElement(char* fileName,
						char* description,
						int timeStep)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray *vectors;
  int lineRead, elementType;
  float value;
  
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

  this->ReadNextDataLine(line); // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"
  
  while (lineRead && strcmp(line, "part") == 0)
    {
    vectors = vtkFloatArray::New();
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    numCells = this->GetOutput(partId)->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    vectors->SetNumberOfTuples(numCells);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numCells*3);
    
    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
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
        for (i = 0; i < 3; i++)
          {
          for (j = 0; j < numCellsPerElement; j++)
            {
            this->ReadNextDataLine(line);
            value = atof(line);
            vectors->InsertComponent(this->CellIds[idx][elementType]->GetId(j),
                                     i, value);
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
int vtkEnSightGoldReader::ReadTensorsPerElement(char* fileName,
                                                char* description,
						int timeStep)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray *tensors;
  int lineRead, elementType;
  float value;
  
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

  this->ReadNextDataLine(line); // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"
  
  while (lineRead && strcmp(line, "part") == 0)
    {
    tensors = vtkFloatArray::New();
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    numCells = this->GetOutput(partId)->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    tensors->SetNumberOfTuples(numCells);
    tensors->SetNumberOfComponents(6);
    tensors->Allocate(numCells*6);
    
    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
      {
      for (i = 0; i < 6; i++)
        {
        for (j = 0; j < numCells; j++)
          {
          this->ReadNextDataLine(line);
          value = atof(line);
          tensors->InsertComponent(j, i, value);
          }
        }
      lineRead = this->ReadNextDataLine(line);
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
        for (i = 0; i < 6; i++)
          {
          for (j = 0; j < numCellsPerElement; j++)
            {
            this->ReadNextDataLine(line);
            value = atof(line);
            tensors->InsertComponent(this->CellIds[idx][elementType]->GetId(j),
                                     i, value);
            }
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
int vtkEnSightGoldReader::CreateUnstructuredGridOutput(int partId,
                                                       char line[256])
{
  int lineRead = 1;
  char subLine[256];
  int i, j;
  vtkIdType *nodeIds;
  int numElements;
  int idx, cellType;
  vtkIdType cellId;
  
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
      float point[3];
      
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
      
      if (isdigit(subLine[0]))
        { // necessary if node ids were listed
        for (i = 0; i < numPts; i++)
          {
          points->GetPoint(i, point);
          points->SetPoint(i, point[1], point[2], atof(line));
          lineRead = this->ReadNextDataLine(line);
          }
        }
      ((vtkUnstructuredGrid*)this->GetOutput(partId))->SetPoints(points);
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
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_VERTEX, 1, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_POINT]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        }
      else
        {
        for (i = 0; i < numElements; i++)
          {
          nodeIds[0] = elementIds[i] - 1;
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_VERTEX, 1, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_POINT]->InsertNextId(cellId);
          }
        }
      
      delete [] nodeIds;
      delete [] elementIds;
      }
    else if (strncmp(line, "bar2", 4) == 0)
      {
      vtkDebugMacro("bar2");
      
      nodeIds = new vtkIdType[2];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d", &nodeIds[0], &nodeIds[1]) != 2)
        {
        for (i = 0; i < numElements; i++)
          {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
          }
        }
      for (i = 0; i < numElements; i++)
        {
        sscanf(line, " %d %d", &nodeIds[0], &nodeIds[1]);
        nodeIds[0]--;
        nodeIds[1]--;
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_LINE, 2, nodeIds);
        this->CellIds[idx][VTK_ENSIGHT_BAR2]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      
      delete [] nodeIds;
      }
    else if (strncmp(line, "bar3", 4) == 0)
      {
      vtkDebugMacro("bar3");
      vtkWarningMacro("Only vertex nodes of this element will be read.");
      nodeIds = new vtkIdType[2];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %*d %d", &nodeIds[0], &nodeIds[1]) != 2)
        {
        for (i = 0; i < numElements; i++)
          {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
          }
        }
      for (i = 0; i < numElements; i++)
        {
        sscanf(line, " %d %*d %d", &nodeIds[0], &nodeIds[1]);
        nodeIds[0]--;
        nodeIds[1]--;
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_LINE, 2, nodeIds);
        this->CellIds[idx][VTK_ENSIGHT_BAR3]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      
      delete [] nodeIds;
      }
    else if (strncmp(line, "nsided", 6) == 0)
      {
      int numNodes;
      char ** newLines;
      char formatLine[256], tempLine[256];
      
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      newLines = new char *[numElements*2];
      for (i = 0; i < numElements*2; i++)
        {
        newLines[i] = new char[256];
        this->ReadNextDataLine(newLines[i]);
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
          numNodes = atoi(newLines[numElements+i]);
          nodeIds = new vtkIdType[numNodes];
          strcpy(formatLine, "");
          strcpy(tempLine, "");
          for (j = 0; j < numNodes; j++)
            {
            strcat(formatLine, " %s");
            sscanf(line, formatLine, nodeIds[numNodes-j]);
            strcat(tempLine, " %*s");
            strcpy(formatLine, tempLine);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_POLYGON, numNodes, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_NSIDED]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          delete [] nodeIds;
          }
        }
      else
        {
        // We have already read in the lines with node ids into newLines.
        for (i = 0; i < numElements; i++)
          {
          numNodes = atoi(newLines[i]);
          nodeIds = new vtkIdType[numNodes];
          strcpy(formatLine, "");
          strcpy(tempLine, "");
          for (j = 0; j < numNodes; j++)
            {
            strcat(formatLine, " %d");
            sscanf(newLines[numElements+i], formatLine,
                   &nodeIds[numNodes-j-1]);
            nodeIds[numNodes-j-1]--;
            strcat(tempLine, " %*d");
            strcpy(formatLine, tempLine);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_POLYGON, numNodes, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_NSIDED]->InsertNextId(cellId);
          delete [] nodeIds;
          }
        }
      for (i = 0; i < numElements*2; i++)
        {
        delete [] newLines[i];
        newLines[i] = NULL;
        }
      delete [] newLines;
      newLines = NULL;
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
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d", &nodeIds[0], &nodeIds[1],
                 &nodeIds[2]) != 3)
        {
        for (i = 0; i < numElements; i++)
          {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
          }
        }
      for (i = 0; i < numElements; i++)
        {
        sscanf(line, " %d %d %d", &nodeIds[0], &nodeIds[1], &nodeIds[2]);
        nodeIds[0]--;
        nodeIds[1]--;
        nodeIds[2]--;
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_TRIANGLE, 3, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      
      delete [] nodeIds;
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
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &nodeIds[0], &nodeIds[1],
                 &nodeIds[2], &nodeIds[3]) != 4)
        {
        for (i = 0; i < numElements; i++)
          {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
          }
        }
      for (i = 0; i < numElements; i++)
        {
        sscanf(line, " %d %d %d %d", &nodeIds[0], &nodeIds[1], &nodeIds[2],
               &nodeIds[3]);
        nodeIds[0]--;
        nodeIds[1]--;
        nodeIds[2]--;
        nodeIds[3]--;
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_QUAD, 4, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      
      delete [] nodeIds;
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
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &nodeIds[0], &nodeIds[1],
                 &nodeIds[2], &nodeIds[3]) != 4)
        {
        for (i = 0; i < numElements; i++)
          {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
          }
        }
      for (i = 0; i < numElements; i++)
        {
        sscanf(line, " %d %d %d %d", &nodeIds[0], &nodeIds[1], &nodeIds[2],
               &nodeIds[3]);
        nodeIds[0]--;
        nodeIds[1]--;
        nodeIds[2]--;
        nodeIds[3]--;
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_TETRA, 4, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      
      delete [] nodeIds;
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
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d", &nodeIds[0], &nodeIds[1],
                 &nodeIds[2], &nodeIds[3], &nodeIds[4]) != 5)
        {
        for (i = 0; i < numElements; i++)
          {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
          }
        }
      for (i = 0; i < numElements; i++)
        {
        sscanf(line, " %d %d %d %d %d", &nodeIds[0], &nodeIds[1],
               &nodeIds[2], &nodeIds[3], &nodeIds[4]);
        nodeIds[0]--;
        nodeIds[1]--;
        nodeIds[2]--;
        nodeIds[3]--;
        nodeIds[4]--;
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_PYRAMID, 5, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
          
      delete [] nodeIds;
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
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d %d %d", &nodeIds[0],
                 &nodeIds[1], &nodeIds[2], &nodeIds[3], &nodeIds[4],
                 &nodeIds[5], &nodeIds[6], &nodeIds[7]) != 8)
        {
        for (i = 0; i < numElements; i++)
          {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
          }
        }
      for (i = 0; i < numElements; i++)
        {
        sscanf(line, " %d %d %d %d %d %d %d %d", &nodeIds[0], &nodeIds[1],
               &nodeIds[2], &nodeIds[3], &nodeIds[4], &nodeIds[5],
               &nodeIds[6], &nodeIds[7]);
        nodeIds[0]--;
        nodeIds[1]--;
        nodeIds[2]--;
        nodeIds[3]--;
        nodeIds[4]--;
        nodeIds[5]--;
        nodeIds[6]--;
        nodeIds[7]--;          
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_HEXAHEDRON, 8, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      
      delete [] nodeIds;
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
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d %d %d", &nodeIds[0], &nodeIds[1],
                 &nodeIds[2], &nodeIds[3], &nodeIds[4], &nodeIds[5]) != 6)
        {
        for (i = 0; i < numElements; i++)
          {
          // Skip the element ids since they are just labels.
          this->ReadNextDataLine(line);
          }
        }
      for (i = 0; i < numElements; i++)
        {
        sscanf(line, " %d %d %d %d %d %d", &nodeIds[0], &nodeIds[1],
               &nodeIds[2], &nodeIds[3], &nodeIds[4], &nodeIds[5]);
        nodeIds[0]--;
        nodeIds[1]--;
        nodeIds[2]--;
        nodeIds[3]--;
        nodeIds[4]--;
        nodeIds[5]--;
        cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
          InsertNextCell(VTK_WEDGE, 6, nodeIds);
        this->CellIds[idx][cellType]->InsertNextId(cellId);
        lineRead = this->ReadNextDataLine(line);
        }
      
      delete [] nodeIds;
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
                                                     char line[256])
{
  char subLine[256];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkPoints *points = vtkPoints::New();
  float point[3];
  int numPts;
  
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
  if (iblanked)
    {
    for (i = 0; i < numPts; i++)
      {
      this->ReadNextDataLine(line);
      if (!atoi(line))
        {
        ((vtkStructuredGrid*)this->GetOutput(partId))->BlankPoint(i);
        }
      }
    }
  
  ((vtkStructuredGrid*)this->GetOutput(partId))->SetPoints(points);
  points->Delete();
  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::CreateRectilinearGridOutput(int partId,
                                                      char line[256])
{
  char subLine[256];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int i;
  vtkFloatArray *xCoords = vtkFloatArray::New();
  vtkFloatArray *yCoords = vtkFloatArray::New();
  vtkFloatArray *zCoords = vtkFloatArray::New();
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

  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &dimensions[0], &dimensions[1], &dimensions[2]);
  ((vtkRectilinearGrid*)this->GetOutput(partId))->SetDimensions(dimensions);
  ((vtkRectilinearGrid*)this->GetOutput(partId))->
    SetWholeExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
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
    vtkWarningMacro("VTK does not handle blanking for rectilinear grids.");
    for (i = 0; i < numPts; i++)
      {
      this->ReadNextDataLine(line);
      }
    }
  
  ((vtkRectilinearGrid*)this->GetOutput(partId))->SetXCoordinates(xCoords);  
  ((vtkRectilinearGrid*)this->GetOutput(partId))->SetYCoordinates(yCoords);
  ((vtkRectilinearGrid*)this->GetOutput(partId))->SetZCoordinates(zCoords);
  
  // reading next line to check for EOF
  lineRead = this->ReadNextDataLine(line);
  return lineRead;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::CreateImageDataOutput(int partId, char line[256])
{
  char subLine[256];
  int lineRead = 1;
  int iblanked = 0;
  int dimensions[3];
  int i;
  float origin[3], delta[3];
  int numPts;
  
  if (this->GetOutput(partId) == NULL)
    {
    vtkDebugMacro("creating new image data output");
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

  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &dimensions[0], &dimensions[1], &dimensions[2]);
  ((vtkImageData*)this->GetOutput(partId))->SetDimensions(dimensions);
  ((vtkImageData*)this->GetOutput(partId))->
    SetWholeExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  this->ReadNextDataLine(line);
  sscanf(line, " %f %f %f", &origin[0], &origin[1], &origin[2]);
  ((vtkImageData*)this->GetOutput(partId))->SetOrigin(origin[0], origin[1],
                                                      origin[2]);
  this->ReadNextDataLine(line);
  sscanf(line, " %f %f %f", &delta[0], &delta[1], &delta[2]);
  ((vtkImageData*)this->GetOutput(partId))->SetSpacing(delta[0], delta[1],
                                                       delta[2]);
  
  if (iblanked)
    {
    vtkWarningMacro("VTK does not handle blanking for image data.");
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
