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
int vtkEnSight6Reader::ReadGeometryFile()
{
  char line[256], subLine[256];
  char tempLine1[256], tempLine2[256], tempLine3[256], tempLine4[256];
  int partId;
  int lineRead;
  int useListedIds = 0;
  int pointId;
  float point[3];
  int i;
  
  // Initialize
  //
  if (!this->GeometryFileName)
    {
    vtkErrorMacro("A GeometryFileName must be specified in the case file.");
    return 0;
    }
  if (strrchr(this->GeometryFileName, '*') != NULL)
    {
    vtkErrorMacro("VTK does not currently handle time.");
    return 0;
    }
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, this->GeometryFileName);
    vtkDebugMacro("full path to geometry file: " << line);
    }
  else
    {
    strcpy(line, this->GeometryFileName);
    }
  
  this->IS = new ifstream(line, ios::in);
  if (this->IS->fail())
    {
    vtkErrorMacro("Unable to open file: " << line);
    delete this->IS;
    this->IS = NULL;
    return 0;
    }
  
  // Skip the 2 description lines.  Using ReadLine instead of ReadNextDataLine
  // because the description line could be blank.
  //this->ReadNextDataLine(line);
  this->ReadLine(line);
  sscanf(line, " %*s %s", subLine);
  if (strcmp(subLine, "binary") == 0)
    {
    vtkErrorMacro("Reading binary files is not implemented yet.");
    return 0;
    }
  //this->ReadNextDataLine(line);
  this->ReadLine(line);
  
  // Read the node id and element id lines.
  //this->ReadNextDataLine(line);
  this->ReadLine(line);
  sscanf(line, " %*s %*s %s", subLine);
  if (strcmp(subLine, "given") == 0)
    {
    this->UnstructuredNodeIds = vtkIdList::New();
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
  
  for (i = 0; i < this->NumberOfUnstructuredPoints; i++)
    {
    this->ReadNextDataLine(line);
//    if (sscanf(line, " %d %f %f %f", &pointId, &point[0], &point[1],
//               &point[2]) == 4)
    if (sscanf(line, " %s %s %s %s", tempLine1, tempLine2, tempLine3, tempLine4) == 4)
      {
      pointId = atoi(tempLine1);
      point[0] = atof(tempLine2);
      point[1] = atof(tempLine3);
      point[2] = atof(tempLine4);
      // point ids listed
      if (this->UnstructuredNodeIds)
        {
        this->UnstructuredNodeIds->InsertNextId(pointId-1);
        }
      this->UnstructuredPoints->InsertNextPoint(point);
      }
    else
      {
      sscanf(line, " %f %f %f", &point[0], &point[1], &point[2]);
      this->UnstructuredPoints->InsertNextPoint(point);
      }
    }
  
  lineRead = this->ReadNextDataLine(line); // "part"
  
  while (lineRead && strncmp(line, "part", 4) == 0)
    {
    sscanf(line, " part %d", &partId);
    partId--; // EnSight starts #ing at 1.
    
    //this->ReadNextDataLine(line);
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
int vtkEnSight6Reader::ReadScalarsPerNode(char* fileName, char* description,
					  int numberOfComponents, int component)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numPts, i, j;
  vtkFloatArray *scalars;
  vtkFieldData *fieldData;
  int numLines, moreScalars;
  float scalarsRead[6];
  int lineRead, arrayNum;
  
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

  this->ReadLine(line); // skip the description line

  lineRead = this->ReadNextDataLine(line); // 1st data line or part #
  if (strncmp(line, "part", 4) != 0)
    {
    // There are 6 values per line, and one scalar per point.
    numPts = this->UnstructuredPoints->GetNumberOfPoints();
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
      // It does not matter which unstructured part we get the point data from because
      // it is the same for all of them.
      partId = this->UnstructuredPartIds->GetId(0);
      scalars = (vtkFloatArray*)(this->GetOutput(partId)->GetPointData()->GetFieldData()->
	GetArray(description, arrayNum));
      }
    for (i = 0; i < numLines; i++)
      {
      sscanf(line, " %f %f %f %f %f %f", &scalarsRead[0], &scalarsRead[1],
             &scalarsRead[2], &scalarsRead[3], &scalarsRead[4],
             &scalarsRead[5]);
      for (j = 0; j < 6; j++)
        {
        scalars->InsertComponent(i*6 + j, component, scalarsRead[j]);        
        }
      lineRead = this->ReadNextDataLine(line);
      }
    strcpy(formatLine, "");
    strcpy(tempLine, "");
    for (j = 0; j < moreScalars; j++)
      {
      strcat(formatLine, " %f");
      sscanf(line, formatLine, &scalarsRead[j]);
      scalars->InsertComponent(i*6 + j, component, scalarsRead[j]);
      strcat(tempLine, " %*f");
      strcpy(formatLine, tempLine);
      }
    for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
      {
      partId = this->UnstructuredPartIds->GetId(i);
      if (this->GetOutput(partId)->GetPointData()->GetFieldData() == NULL)
        {
        fieldData = vtkFieldData::New();
        fieldData->Allocate(1000);
        this->GetOutput(partId)->GetPointData()->SetFieldData(fieldData);
        fieldData->Delete();
        }
      if (component == 0)
	{
	this->GetOutput(partId)->GetPointData()->GetFieldData()->
	  AddArray(scalars, description);
	scalars->Delete();
	}
      else
	{
	this->GetOutput(partId)->GetPointData()->GetFieldData()->SetArray(arrayNum, scalars);
	}
      }
    }

  // scalars for structured parts
  while (lineRead = this->ReadNextDataLine(line) &&
         strncmp(line, "part", 4) == 0)
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
      scalars = (vtkFloatArray*)(this->GetOutput(partId)->GetPointData()->GetFieldData()->
	GetArray(description, arrayNum)); 
      }
    for (i = 0; i < numLines; i++)
      {
      lineRead = this->ReadNextDataLine(line);
      sscanf(line, " %f %f %f %f %f %f", &scalarsRead[0], &scalarsRead[1],
             &scalarsRead[2], &scalarsRead[3], &scalarsRead[4],
             &scalarsRead[5]);
      for (j = 0; j < 6; j++)
        {
        scalars->InsertComponent(i*6 + j, component, scalarsRead[j]);        
        }
      }
    lineRead = this->ReadNextDataLine(line);
    strcpy(formatLine, "");
    strcpy(tempLine, "");
    for (j = 0; j < moreScalars; j++)
      {
      strcat(formatLine, " %f");
      sscanf(line, formatLine, &scalarsRead[j]);
      strcat(tempLine, " %*f");
      strcpy(formatLine, tempLine);
      }
    if (this->GetOutput(partId)->GetPointData()->GetFieldData() == NULL)
      {
      fieldData = vtkFieldData::New();
      fieldData->Allocate(1000);
      this->GetOutput(partId)->GetPointData()->SetFieldData(fieldData);
      fieldData->Delete();
      }
    if (component == 0)
      {
      this->GetOutput(partId)->GetPointData()->GetFieldData()->
	AddArray(scalars, description);
      scalars->Delete();
      }
    else
      {
      this->GetOutput(partId)->GetPointData()->GetFieldData()->SetArray(arrayNum, scalars);
      }
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadVectorsPerNode(char* fileName, char* description)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numPts, i, j, k;
  vtkFloatArray *vectors;
  vtkFieldData *fieldData;
  int numLines, moreVectors;
  float vector1[3], vector2[3], values[6];
  int lineRead;
  
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

  //this->ReadNextDataLine(line);
  this->ReadLine(line); // skip the description line

  lineRead = this->ReadNextDataLine(line); // 1st data line or part #
  if (strncmp(line, "part", 4) != 0)
    {
    // There are 6 values per line, and 3 values (or 1 vector) per point.
    numPts = this->UnstructuredPoints->GetNumberOfPoints();
    numLines = numPts / 2;
    moreVectors = ((numPts * 3) % 6) / 3;
    vectors = vtkFloatArray::New();
    vectors->SetNumberOfTuples(numPts);
    vectors->SetNumberOfComponents(3);
    vectors->Allocate(numPts*3);
    for (i = 0; i < numLines; i++)
      {
      sscanf(line, " %f %f %f %f %f %f", &vector1[0], &vector1[1],
             &vector1[2], &vector2[0], &vector2[1], &vector2[2]);
      vectors->InsertTuple(i*2, vector1);
      vectors->InsertTuple(i*2 + 1, vector2);
      lineRead = this->ReadNextDataLine(line);
      }
    strcpy(formatLine, "");
    strcpy(tempLine, "");
    for (j = 0; j < moreVectors; j++)
      {
      strcat(formatLine, " %f %f %f");
      sscanf(line, formatLine, &vector1[0], &vector1[1], &vector1[2]);
      vectors->InsertTuple(i*2 + j, vector1);
      strcat(tempLine, " %*f %*f %*f");
      strcpy(formatLine, tempLine);
      }
    for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
      {
      partId = this->UnstructuredPartIds->GetId(i);
      if (this->GetOutput(partId)->GetPointData()->GetFieldData() == NULL)
        {
        fieldData = vtkFieldData::New();
        fieldData->Allocate(1000);
        this->GetOutput(partId)->GetPointData()->SetFieldData(fieldData);
        fieldData->Delete();
        }
      this->GetOutput(partId)->GetPointData()->GetFieldData()->
        AddArray(vectors, description);
      }
    vectors->Delete();
    }

  // vectors for structured parts
  while (lineRead = this->ReadNextDataLine(line) &&
         strncmp(line, "part", 4) == 0)
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
        lineRead = this->ReadNextDataLine(line);
        sscanf(line, " %f %f %f %f %f %f", &values[0], &values[1],
               &values[2], &values[3], &values[4], &values[5]);
        for (j = 0; j < 6; j++)
          {
          vectors->InsertComponent(i*6 + j, k, values[j]);
          }
        }
      
      if (moreVectors)
        {
        lineRead = this->ReadNextDataLine(line);
        strcpy(formatLine, "");
        strcpy(tempLine, "");
        for (j = 0; j < moreVectors; j++)
          {
          strcat(formatLine, " %f");
          sscanf(line, formatLine, &values[j]);
          vectors->InsertComponent(i*6 + j, k, values[j]);
          strcat(tempLine, " %*f");
          strcpy(formatLine, tempLine);
          }
        }
      }
    
    if (this->GetOutput(partId)->GetPointData()->GetFieldData() == NULL)
      {
      fieldData = vtkFieldData::New();
      fieldData->Allocate(1000);
      this->GetOutput(partId)->GetPointData()->SetFieldData(fieldData);
      fieldData->Delete();
      }
    this->GetOutput(partId)->GetPointData()->GetFieldData()->
      AddArray(vectors, description);
    vectors->Delete();
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadTensorsPerNode(char* fileName, char* description)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numPts, i, j, k;
  vtkFloatArray *tensors;
  vtkFieldData *fieldData;
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

  //this->ReadNextDataLine(line);
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
      sscanf(line, " %f %f %f %f %f %f", &tensor[0], &tensor[1],
             &tensor[2], &tensor[3], &tensor[4], &tensor[5]);
      tensors->InsertTuple(i, tensor);
      lineRead = this->ReadNextDataLine(line);
      }

    for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
      {
      partId = this->UnstructuredPartIds->GetId(i);
      if (this->GetOutput(partId)->GetPointData()->GetFieldData() == NULL)
        {
        fieldData = vtkFieldData::New();
        fieldData->Allocate(1000);
        this->GetOutput(partId)->GetPointData()->SetFieldData(fieldData);
        fieldData->Delete();
        }
      this->GetOutput(partId)->GetPointData()->GetFieldData()->
        AddArray(tensors, description);
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
        sscanf(line, " %f %f %f %f %f %f", &values[0], &values[1],
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
          strcat(formatLine, " %f");
          sscanf(line, formatLine, &values[j]);
          tensors->InsertComponent(i*6 + j, k, values[j]);
          strcat(tempLine, " %*f");
          strcpy(formatLine, tempLine);
          }
        }
      }
    
    if (this->GetOutput(partId)->GetPointData()->GetFieldData() == NULL)
      {
      fieldData = vtkFieldData::New();
      fieldData->Allocate(1000);
      this->GetOutput(partId)->GetPointData()->SetFieldData(fieldData);
      fieldData->Delete();
      }
    this->GetOutput(partId)->GetPointData()->GetFieldData()->
      AddArray(tensors, description);
    tensors->Delete();
    lineRead = this->ReadNextDataLine(line);
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadScalarsPerElement(char* fileName, char* description,
					     int numberOfComponents, int component)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray *scalars;
  vtkFieldData *fieldData;
  int lineRead, elementType;
  float scalarsRead[6];
  int numLines, moreScalars;
  int arrayNum;
  
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
      scalars = (vtkFloatArray*)(this->GetOutput(partId)->GetCellData()->GetFieldData()->
	GetArray(description, arrayNum));
      }
    numLines = numCells / 6;
    moreScalars = numCells % 6;
    
    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
      {
      for (i = 0; i < numLines; i++)
        {
        this->ReadNextDataLine(line);
        sscanf(line, " %f %f %f %f %f %f", &scalarsRead[0], &scalarsRead[1],
               &scalarsRead[2], &scalarsRead[3], &scalarsRead[4],
               &scalarsRead[5]);
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
          strcat(formatLine, " %f");
          sscanf(line, formatLine, &scalarsRead[j]);
          scalars->InsertComponent(i*6 + j, component, scalarsRead[j]);
          strcat(tempLine, " %*f");
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
          sscanf(line, " %f %f %f %f %f %f", &scalarsRead[0], &scalarsRead[1],
                 &scalarsRead[2], &scalarsRead[3], &scalarsRead[4],
                 &scalarsRead[5]);
          for (j = 0; j < 6; j++)
            {
            scalars->InsertComponent(this->CellIds[idx][elementType]->GetId(i*6 + j),
                                     component, scalarsRead[j]);
            }
          }
        if (moreScalars)
          {
          lineRead = this->ReadNextDataLine(line);
          strcpy(formatLine, "");
          strcpy(tempLine, "");
          for (j = 0; j < moreScalars; j++)
            {
            strcat(formatLine, " %f");
            sscanf(line, formatLine, &scalarsRead[j]);
            scalars->InsertComponent(this->CellIds[idx][elementType]->GetId(i*6 + j),
                                     component, scalarsRead[j]);
            strcat(tempLine, " %*f");
            strcpy(formatLine, tempLine);
            }
          }
        lineRead = this->ReadNextDataLine(line);
        } // end while
      } // end else
    if (this->GetOutput(partId)->GetCellData()->GetFieldData() == NULL)
      {
      fieldData = vtkFieldData::New();
      fieldData->Allocate(1000);
      this->GetOutput(partId)->GetCellData()->SetFieldData(fieldData);
      fieldData->Delete();
      }
    if (component == 0)
      {
      this->GetOutput(partId)->GetCellData()->GetFieldData()->
	AddArray(scalars, description);
      scalars->Delete();
      }
    else
      {
      this->GetOutput(partId)->GetCellData()->GetFieldData()->SetArray(arrayNum, scalars);
      }
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadVectorsPerElement(char* fileName,
                                             char* description)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numCells, numCellsPerElement, i, j, k, idx;
  vtkFloatArray *vectors;
  vtkFieldData *fieldData;
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

  //this->ReadNextDataLine(line);
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
          sscanf(line, " %f %f %f %f %f %f", &values[0], &values[1],
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
            strcat(formatLine, " %f");
            sscanf(line, formatLine, &values[j]);
            vectors->InsertComponent(i*6 + j, k, values[j]);
            strcat(tempLine, " %*f");
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
          sscanf(line, " %f %f %f %f %f %f", &vector1[0], &vector1[1],
                 &vector1[2], &vector2[0], &vector2[1], &vector2[2]);
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
            strcat(formatLine, " %f %f %f");
            sscanf(line, formatLine, &vector1[0], &vector1[1], &vector1[2]);
            vectors->InsertTuple(this->CellIds[idx][elementType]->GetId(2*i + j),
                                 vector1);
            strcat(tempLine, " %*f %*f %*f");
            strcpy(formatLine, tempLine);
            }
          }
        lineRead = this->ReadNextDataLine(line);
        } // end while
      } // end else
    if (this->GetOutput(partId)->GetCellData()->GetFieldData() == NULL)
      {
      fieldData = vtkFieldData::New();
      fieldData->Allocate(1000);
      this->GetOutput(partId)->GetCellData()->SetFieldData(fieldData);
      fieldData->Delete();
      }
    this->GetOutput(partId)->GetCellData()->GetFieldData()->
      AddArray(vectors, description);
    vectors->Delete();
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSight6Reader::ReadTensorsPerElement(char* fileName,
                                             char* description)
{
  char line[256];
  char formatLine[256], tempLine[256];
  int partId, numCells, numCellsPerElement, i, j, k, idx;
  vtkFloatArray *tensors;
  vtkFieldData *fieldData;
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

  //this->ReadNextDataLine(line);
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
          sscanf(line, " %f %f %f %f %f %f", &values[0], &values[1],
                 &values[2], &values[3], &values[4], &values[5]);
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
            strcat(formatLine, " %f");
            sscanf(line, formatLine, &values[j]);
            tensors->InsertComponent(i*6 + j, k, values[j]);
            strcat(tempLine, " %*f");
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
          sscanf(line, " %f %f %f %f %f %f", &tensor[0], &tensor[1],
                 &tensor[2], &tensor[3], &tensor[4], &tensor[5]);
          tensors->InsertTuple(this->CellIds[idx][elementType]->GetId(i),
                               tensor);
          }
        lineRead = this->ReadNextDataLine(line);
        } // end while
      } // end else
    if (this->GetOutput(partId)->GetCellData()->GetFieldData() == NULL)
      {
      fieldData = vtkFieldData::New();
      fieldData->Allocate(1000);
      this->GetOutput(partId)->GetCellData()->SetFieldData(fieldData);
      fieldData->Delete();
      }
    this->GetOutput(partId)->GetCellData()->GetFieldData()->
      AddArray(tensors, description);
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
  int i;
  int *nodeIds;
  int numElements;
  int idx, cellId, cellType;
  
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
      
      nodeIds = new int[1];        
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
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_VERTEX, 1, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_POINT]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
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
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_VERTEX, 1, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_POINT]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        }
      delete [] nodeIds;
      }
    else if (strncmp(line, "bar2", 4) == 0)
      {
      vtkDebugMacro("bar2");
      
      nodeIds = new int[2];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*d %d %d", &nodeIds[0], &nodeIds[1]) == 2)
          {
          // element ids listed
          nodeIds[0]--;
          nodeIds[1]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_LINE, 2, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_BAR2]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        else
          {
          sscanf(line, " %d %d", &nodeIds[0], &nodeIds[1]);
          nodeIds[0]--;
          nodeIds[1]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_LINE, 2, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_BAR2]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        }
      delete [] nodeIds;
      }
    else if (strncmp(line, "bar3", 4) == 0)
      {
      vtkDebugMacro("bar3");
      vtkWarningMacro("Only vertex nodes of this element will be read.");
      nodeIds = new int[2];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*d %d %*d %d", &nodeIds[0], &nodeIds[1]) == 2)
          {
          // element ids listed
          nodeIds[0]--;
          nodeIds[1]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_LINE, 2, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_BAR3]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        else
          {
          sscanf(line, " %d %*d %d", &nodeIds[0], &nodeIds[1]);
          nodeIds[0]--;
          nodeIds[1]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_LINE, 2, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_BAR3]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        }
      delete [] nodeIds;
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
      
      nodeIds = new int[3];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*d %d %d %d", &nodeIds[0], &nodeIds[1],
                   &nodeIds[2]) == 3)
          {
          // element ids listed
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_TRIANGLE, 3, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        else
          {
          sscanf(line, " %d %d %d", &nodeIds[0], &nodeIds[1], &nodeIds[2]);
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_TRIANGLE, 3, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
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
      
      nodeIds = new int[4];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*d %d %d %d %d", &nodeIds[0], &nodeIds[1],
                   &nodeIds[2], &nodeIds[3]) == 4)
          {
          // element ids listed
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          nodeIds[3]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            nodeIds[3] = this->UnstructuredNodeIds->IsId(nodeIds[3]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_QUAD, 4, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        else
          {
          sscanf(line, " %d %d %d %d", &nodeIds[0], &nodeIds[1], &nodeIds[2],
                 &nodeIds[3]);
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          nodeIds[3]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            nodeIds[3] = this->UnstructuredNodeIds->IsId(nodeIds[3]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_QUAD, 4, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
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
      
      nodeIds = new int[4];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*d %d %d %d %d", &nodeIds[0], &nodeIds[1],
                   &nodeIds[2], &nodeIds[3]) == 4)
          {
          // element ids listed
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          nodeIds[3]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            nodeIds[3] = this->UnstructuredNodeIds->IsId(nodeIds[3]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_TETRA, 4, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        else
          {
          sscanf(line, " %d %d %d %d", &nodeIds[0], &nodeIds[1], &nodeIds[2],
                 &nodeIds[3]);
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          nodeIds[3]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            nodeIds[3] = this->UnstructuredNodeIds->IsId(nodeIds[3]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_TETRA, 4, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
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

      nodeIds = new int[5];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*d %d %d %d %d %d", &nodeIds[0], &nodeIds[1],
                   &nodeIds[2], &nodeIds[3], &nodeIds[4]) == 5)
          {
          // element ids listed
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          nodeIds[3]--;
          nodeIds[4]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            nodeIds[3] = this->UnstructuredNodeIds->IsId(nodeIds[3]);
            nodeIds[4] = this->UnstructuredNodeIds->IsId(nodeIds[4]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_PYRAMID, 5, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        else
          {
          sscanf(line, " %d %d %d %d %d", &nodeIds[0], &nodeIds[1],
                 &nodeIds[2], &nodeIds[3], &nodeIds[4]);
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          nodeIds[3]--;
          nodeIds[4]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            nodeIds[3] = this->UnstructuredNodeIds->IsId(nodeIds[3]);
            nodeIds[4] = this->UnstructuredNodeIds->IsId(nodeIds[4]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_PYRAMID, 5, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
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
      
      nodeIds = new int[8];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*d %d %d %d %d %d %d %d %d", &nodeIds[0],
                   &nodeIds[1], &nodeIds[2], &nodeIds[3], &nodeIds[4],
                   &nodeIds[5], &nodeIds[6], &nodeIds[7]) == 8)
          {
          // element ids listed
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          nodeIds[3]--;
          nodeIds[4]--;
          nodeIds[5]--;
          nodeIds[6]--;
          nodeIds[7]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            nodeIds[3] = this->UnstructuredNodeIds->IsId(nodeIds[3]);
            nodeIds[4] = this->UnstructuredNodeIds->IsId(nodeIds[4]);
            nodeIds[5] = this->UnstructuredNodeIds->IsId(nodeIds[5]);
            nodeIds[6] = this->UnstructuredNodeIds->IsId(nodeIds[6]);
            nodeIds[7] = this->UnstructuredNodeIds->IsId(nodeIds[7]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_HEXAHEDRON, 8, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        else
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
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            nodeIds[3] = this->UnstructuredNodeIds->IsId(nodeIds[3]);
            nodeIds[4] = this->UnstructuredNodeIds->IsId(nodeIds[4]);
            nodeIds[5] = this->UnstructuredNodeIds->IsId(nodeIds[5]);
            nodeIds[6] = this->UnstructuredNodeIds->IsId(nodeIds[6]);
            nodeIds[7] = this->UnstructuredNodeIds->IsId(nodeIds[7]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_HEXAHEDRON, 8, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
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
      
      nodeIds = new int[6];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      lineRead = this->ReadNextDataLine(line);
      
      for (i = 0; i < numElements; i++)
        {
        if (sscanf(line, " %*d %d %d %d %d %d %d", &nodeIds[0],
                   &nodeIds[1], &nodeIds[2], &nodeIds[3], &nodeIds[4],
                   &nodeIds[5]) == 6)
          {
          // element ids listed
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          nodeIds[3]--;
          nodeIds[4]--;
          nodeIds[5]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            nodeIds[3] = this->UnstructuredNodeIds->IsId(nodeIds[3]);
            nodeIds[4] = this->UnstructuredNodeIds->IsId(nodeIds[4]);
            nodeIds[5] = this->UnstructuredNodeIds->IsId(nodeIds[5]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_WEDGE, 5, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        else
          {
          sscanf(line, " %d %d %d %d %d %d", &nodeIds[0], &nodeIds[1],
                 &nodeIds[2], &nodeIds[3], &nodeIds[4], &nodeIds[5]);
          nodeIds[0]--;
          nodeIds[1]--;
          nodeIds[2]--;
          nodeIds[3]--;
          nodeIds[4]--;
          nodeIds[5]--;
          if (this->UnstructuredNodeIds)
            {
            nodeIds[0] = this->UnstructuredNodeIds->IsId(nodeIds[0]);
            nodeIds[1] = this->UnstructuredNodeIds->IsId(nodeIds[1]);
            nodeIds[2] = this->UnstructuredNodeIds->IsId(nodeIds[2]);
            nodeIds[3] = this->UnstructuredNodeIds->IsId(nodeIds[3]);
            nodeIds[4] = this->UnstructuredNodeIds->IsId(nodeIds[4]);
            nodeIds[5] = this->UnstructuredNodeIds->IsId(nodeIds[5]);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_WEDGE, 6, nodeIds);
          this->CellIds[idx][cellType]->InsertNextId(cellId);
          lineRead = this->ReadNextDataLine(line);
          }
        }
      delete [] nodeIds;
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
    sscanf(line, " %f %f %f %f %f %f", &coords[0], &coords[1], &coords[2],
           &coords[3], &coords[4], &coords[5]);
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
      strcat(formatLine, " %f");
      sscanf(line, formatLine, &coords[j]);
      points->InsertNextPoint(coords[j], 0.0, 0.0);
      strcat(tempLine, " %*s");
      strcpy(formatLine, tempLine);
      }
    }
  for (i = 0; i < numLines; i++)
    {
    this->ReadNextDataLine(line);
    sscanf(line, " %f %f %f %f %f %f", &coords[0], &coords[1], &coords[2],
           &coords[3], &coords[4], &coords[5]);
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
      strcat(formatLine, " %f");
      sscanf(line, formatLine, &coords[j]);
      points->GetPoint(i*6+j, point);
      points->SetPoint(i*6+j, point[0], coords[j], point[2]);
      strcat(tempLine, " %*s");
      strcpy(formatLine, tempLine);
      }
    }
  for (i = 0; i < numLines; i++)
    {
    this->ReadNextDataLine(line);
    sscanf(line, " %f %f %f %f %f %f", &coords[0], &coords[1], &coords[2],
           &coords[3], &coords[4], &coords[5]);
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
      strcat(formatLine, " %f");
      sscanf(line, formatLine, &coords[j]);
      points->GetPoint(i*6+j, point);
      points->SetPoint(i*6+j, point[0], point[1], coords[j]);
      strcat(tempLine, " %*s");
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
          ((vtkStructuredGrid*)this->GetOutput(partId))->BlankPoint(i);
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
