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
#include "vtkStructuredPoints.h"
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
vtkEnSightGoldReader::vtkEnSightGoldReader()
{
  this->FilePath = NULL;
  
  this->CaseFileName = NULL;
  this->GeometryFileName = NULL;
  this->MeasuredFileName = NULL;
  this->MatchFileName = NULL;

  this->IS = NULL;
  
  this->VariableMode = -1;
  
  this->NumberOfVariables = 0;
  this->NumberOfComplexVariables = 0;

  this->UnstructuredPartIds = vtkIdList::New();
  this->CellIds = NULL;
  
  this->VariableTypes = NULL;
  this->ComplexVariableTypes = NULL;
  
  this->VariableFileNames = NULL;
  this->ComplexVariableFileNames = NULL;
  
  this->VariableDescriptions = NULL;
  this->ComplexVariableDescriptions = NULL;

  this->NumberOfScalarsPerNode = 0;
  this->NumberOfVectorsPerNode = 0;
  this->NumberOfTensorsSymmPerNode = 0;
  this->NumberOfScalarsPerElement = 0;
  this->NumberOfVectorsPerElement = 0;
  this->NumberOfTensorsSymmPerElement = 0;
  this->NumberOfScalarsPerMeasuredNode = 0;
  this->NumberOfVectorsPerMeasuredNode = 0;
  this->NumberOfComplexScalarsPerNode = 0;
  this->NumberOfComplexVectorsPerNode = 0;
  this->NumberOfComplexScalarsPerElement = 0;
  this->NumberOfComplexVectorsPerElement = 0;
}

//----------------------------------------------------------------------------
vtkEnSightGoldReader::~vtkEnSightGoldReader()
{
  int i, j;

  if (this->FilePath)
    {
    delete [] this->FilePath;
    this->FilePath = NULL;
    }
  
  if (this->CaseFileName)
    {
    delete [] this->CaseFileName;
    this->CaseFileName = NULL;
    }
  if (this->GeometryFileName)
    {
    delete [] this->GeometryFileName;
    this->GeometryFileName = NULL;
    }
  if (this->MeasuredFileName)
    {
    delete [] this->MeasuredFileName;
    this->MeasuredFileName = NULL;
    }
  if (this->MatchFileName)
    {
    delete [] this->MatchFileName;
    this->MatchFileName = NULL;
    }

  if (this->IS)
    {
    delete this->IS;
    this->IS = NULL;
    }

  if (this->NumberOfVariables > 0)
    {
    for (i = 0; i < this->NumberOfVariables; i++)
      {
      delete [] this->VariableFileNames[i];
      delete [] this->VariableDescriptions[i];
      }
    delete [] this->VariableFileNames;
    this->VariableFileNames = NULL;
    delete [] this->VariableDescriptions;
    this->VariableDescriptions = NULL;
    delete [] this->VariableTypes;
    this->VariableTypes = NULL;
    }

  if (this->NumberOfComplexVariables > 0)
    {
    for (i = 0; i < this->NumberOfComplexVariables*2; i++)
      {
      delete [] this->ComplexVariableFileNames[i];
      if (i < this->NumberOfComplexVariables)
        {
        delete [] this->ComplexVariableDescriptions[i];
        }
      }
    delete [] this->ComplexVariableFileNames;
    this->ComplexVariableFileNames = NULL;
    delete [] this->ComplexVariableDescriptions;
    this->ComplexVariableDescriptions = NULL;
    delete [] this->ComplexVariableTypes;
    this->ComplexVariableTypes = NULL;
    }
  
  if (this->CellIds)
    {
    for (i = 0; i < this->UnstructuredPartIds->GetNumberOfIds(); i++)
      {
      for (j = 0; j < 16; j++)
        {
        this->CellIds[i][j]->Delete();
        this->CellIds[i][j] = NULL;
        }
      delete [] this->CellIds[i];
      this->CellIds[i] = NULL;
      }
    delete [] this->CellIds;
    this->CellIds = NULL;
    }
  
  this->UnstructuredPartIds->Delete();
  this->UnstructuredPartIds = NULL;
}

//----------------------------------------------------------------------------
void vtkEnSightGoldReader::Execute()
{
  if (!this->ReadCaseFile())
    {
    vtkErrorMacro("error reading case file");
    return;
    }
  if (!this->ReadGeometryFile())
    {
    vtkErrorMacro("error reading geometry file");
    return;
    }
  if ((this->NumberOfVariables + this->NumberOfComplexVariables) > 0)
    {
    if (!this->ReadVariableFiles())
      {
      vtkErrorMacro("error reading variable files");
      return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkEnSightGoldReader::Update()
{
  int i;
  
  this->Execute();
  
  for (i = 0; i < this->GetNumberOfOutputs(); i++)
    {
    if ( this->GetOutput(i) )
      {
      this->GetOutput(i)->DataHasBeenGenerated();
      }
    }
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadCaseFile()
{
  char line[256];
  char subLine[256], subLine2[256];
  
  // Initialize
  //
  if (!this->CaseFileName)
    {
    vtkErrorMacro("A CaseFileName must be specified.");
    return 0;
    }
  if (this->FilePath)
    {
    strcpy(line, this->FilePath);
    strcat(line, this->CaseFileName);
    vtkDebugMacro("full path to case file: " << line);
    }
  else
    {
    strcpy(line, this->CaseFileName);
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
  
  if (strncmp(line, "FORMAT", 6) == 0)
    {
    // found the FORMAT section
    vtkDebugMacro("*** FORMAT section");
    this->ReadNextDataLine(line);
    
    sscanf(line, " %*s %*s %s", subLine);
    if (strcmp(subLine, "gold") != 0)
      {
      vtkErrorMacro("This is not an EnSight Gold file.");
      delete this->IS;
      this->IS = NULL;
      return 0;
      }
    }
  
  // We know how many lines to read in the FORMAT section, so we haven't read
  // the "GEOMETRY" line yet.
  this->ReadNextDataLine(line);
  if (strncmp(line, "GEOMETRY", 8) == 0)
    {
    // found the GEOMETRY section
    vtkDebugMacro("*** GEOMETRY section");
    
    // There will definitely be a "model" line.  There may also be "measured"
    // and "match" lines.
    while(this->ReadNextDataLine(line) != 0 &&
          strncmp(line, "m", 1) == 0)
      {
      if (strncmp(line, "model:", 6) == 0)
        {
        if (sscanf(line, " %*s %*d %*d %s", subLine) == 1)
          {
          this->SetGeometryFileName(subLine);
          vtkDebugMacro(<<this->GetGeometryFileName());
          }
        else if (sscanf(line, " %*s %*d %s", subLine) == 1)
          {
          this->SetGeometryFileName(subLine);
          vtkDebugMacro(<<this->GetGeometryFileName());
          }
        else if (sscanf(line, " %*s %s", subLine) == 1)
          {
          this->SetGeometryFileName(subLine);
          vtkDebugMacro(<<this->GetGeometryFileName());
          }
        }
      else if (strncmp(subLine, "measured:", 9) == 0)
        {
        if (sscanf(line, " %*s %*d %*d %s", subLine) == 1)
          {
          this->SetMeasuredFileName(subLine);
          vtkDebugMacro(<< this->GetMeasuredFileName());
          }
        else if (sscanf(line, " %*s %*d %s", subLine) == 1)
          {
          this->SetMeasuredFileName(subLine);
          vtkDebugMacro(<< this->GetMeasuredFileName());
          }
        else if (sscanf(line, " %*s %s", subLine) == 1)
          {
          this->SetMeasuredFileName(subLine);
          vtkDebugMacro(<< this->GetMeasuredFileName());
          }
        }
      else if (strncmp(subLine, "match:", 6) == 0)
        {
        sscanf(line, " %*s %s", subLine);
        this->SetMatchFileName(subLine);
        vtkDebugMacro(<< this->GetMatchFileName());
        }
      }
    }
  
  if (strncmp(line, "VARIABLE", 8) == 0)
    {
    // found the VARIABLE section
    vtkDebugMacro(<< "*** VARIABLE section");

    while(this->ReadNextDataLine(line) != 0 &&
          strncmp(line, "TIME", 4) != 0 &&
          strncmp(line, "FILE", 4) != 0)
      {
      if (strncmp(line, "constant", 8) == 0)
        {
        vtkDebugMacro(<< line);
        }
      else if (strncmp(line, "scalar", 6) == 0)
        {
        sscanf(line, " %*s %*s %s", subLine);
        if (strcmp(subLine, "node:") == 0)
          {
          vtkDebugMacro("scalar per node");
          this->VariableMode = VTK_SCALAR_PER_NODE;
          this->AddVariableType();
          if (sscanf(line, " %*s %*s %*s %*d %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfScalarsPerNode++;
            }
          else if (sscanf(line, " %*s %*s %*s %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfScalarsPerNode++;
            }
          else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfScalarsPerNode++;
            }
          this->NumberOfVariables++;
          }
        else if (strcmp(subLine, "element:") == 0)
          {
          vtkDebugMacro("scalar per element");
          this->VariableMode = VTK_SCALAR_PER_ELEMENT;
          this->AddVariableType();
          if (sscanf(line, " %*s %*s %*s %*d %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfScalarsPerElement++;
            }
          else if (sscanf(line, " %*s %*s %*s %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfScalarsPerElement++;
            }
          else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfScalarsPerElement++;
            }
          this->NumberOfVariables++;
          }
        else if (strcmp(subLine, "measured") == 0)
          {
          vtkDebugMacro("scalar per measured node");
          this->VariableMode = VTK_SCALAR_PER_MEASURED_NODE;
          this->AddVariableType();
          if (sscanf(line, " %*s %*s %*s %*s %*d %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfScalarsPerNode++;
            }
          else if (sscanf(line, " %*s %*s %*s %*s %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfScalarsPerNode++;
            }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfScalarsPerNode++;
            }
          this->NumberOfVariables++;
          }
        }
      else if (strncmp(line, "vector", 6) == 0)
        {
        sscanf(line, " %*s %*s %s", subLine);
        if (strcmp(subLine, "node:") == 0)
          {
          vtkDebugMacro("vector per node");
          this->VariableMode = VTK_VECTOR_PER_NODE;
          this->AddVariableType();
          if (sscanf(line, " %*s %*s %*s %*d %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfVectorsPerNode++;
            }
          else if (sscanf(line, " %*s %*s %*s %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfVectorsPerNode++;
            }
          else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfVectorsPerNode++;
            }
          }
        else if (strcmp(subLine, "element:") == 0)
          {
          vtkDebugMacro("vector per element");
          this->VariableMode = VTK_VECTOR_PER_ELEMENT;
          this->AddVariableType();
          if (sscanf(line, " %*s %*s %*s %*d %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfVectorsPerElement++;
            }
          else if (sscanf(line, " %*s %*s %*s %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfVectorsPerElement++;
            }
          else if (sscanf(line, " %*s %*s %*s %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfVectorsPerElement++;
            }
          }
        else if (strcmp(subLine, "measured") == 0)
          {
          vtkDebugMacro("vector per measured node");
          this->VariableMode = VTK_VECTOR_PER_MEASURED_NODE;
          this->AddVariableType();
          if (sscanf(line, " %*s %*s %*s %*s %*d %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfVectorsPerMeasuredNode++;
            }
          else if (sscanf(line, " %*s %*s %*s %*s %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfVectorsPerMeasuredNode++;
            }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfVectorsPerMeasuredNode++;
            }
          }
        this->NumberOfVariables++;
        }
      else if (strncmp(line, "tensor", 6) == 0)
        {
        sscanf(line, " %*s %*s %*s %s", subLine);
        if (strcmp(subLine, "node:") == 0)
          {
          vtkDebugMacro("tensor symm per node");
          this->VariableMode = VTK_TENSOR_SYMM_PER_NODE;
          this->AddVariableType();
          if (sscanf(line, " %*s %*s %*s %*s %*d %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfTensorsSymmPerNode++;
            }
          else if (sscanf(line, " %*s %*s %*s %*s %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfTensorsSymmPerNode++;
            }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfTensorsSymmPerNode++;
            }
          }
        else if (strcmp(subLine, "element:") == 0)
          {
          vtkDebugMacro("tensor symm per element");
          this->VariableMode = VTK_TENSOR_SYMM_PER_ELEMENT;
          this->AddVariableType();
          if (sscanf(line, " %*s %*s %*s %*s %*d %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfTensorsSymmPerElement++;
            }
          else if (sscanf(line, " %*s %*s %*s %*s %*d %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*d %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfTensorsSymmPerElement++;
            }
          else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
            {
            this->AddVariableDescription(subLine);
            sscanf(line, " %*s %*s %*s %*s %*s %s", subLine);
            this->AddVariableFileName(subLine);
            this->NumberOfTensorsSymmPerElement++;
            }
          }
        this->NumberOfVariables++;
        }
      else if (strncmp(line, "complex", 6) == 0)
        {
        sscanf(line, " %*s %s", subLine);
        if (strcmp(subLine, "scalar") == 0)
          {
          sscanf(line, " %*s %*s %*s %s", subLine);
          if (strcmp(subLine, "node:") == 0)
            {
            vtkDebugMacro("complex scalar per node");
            this->VariableMode = VTK_COMPLEX_SCALAR_PER_NODE;
            this->AddVariableType();
            if (sscanf(line, " %*s %*s %*s %*s %*d %*d %s %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine,
                     subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexScalarsPerNode++;
              }
            else if (sscanf(line, " %*s %*s %*s %*s %*d %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine,
                     subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexScalarsPerNode++;
              }
            else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexScalarsPerNode++;
              }
            }
          else if (strcmp(subLine, "element:") == 0)
            {
            vtkDebugMacro("complex scalar per element");
            this->VariableMode = VTK_COMPLEX_SCALAR_PER_ELEMENT;
            this->AddVariableType();
            if (sscanf(line, " %*s %*s %*s %*s %*d %*d %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine,
                     subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexScalarsPerElement++;
              }
            else if (sscanf(line, " %*s %*s %*s %*s %*d %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine,
                     subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexScalarsPerElement++;
              }
            else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexScalarsPerElement++;
              }
            }
          }
        else if (strcmp(subLine, "vector") == 0)
          {
          sscanf(line, " %*s %*s %*s %s", subLine);
          if (strcmp(subLine, "node:") == 0)
            {
            vtkDebugMacro("complex vector per node");
            this->VariableMode = VTK_COMPLEX_VECTOR_PER_NODE;
            this->AddVariableType();
            if (sscanf(line, " %*s %*s %*s %*s %*d %*d %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine,
                     subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexVectorsPerNode++;
              }
            else if (sscanf(line, " %*s %*s %*s %*s %*d %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine,
                     subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexVectorsPerNode++;
              }
            else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
              this->AddVariableFileName(subLine);
              this->NumberOfComplexVectorsPerNode++;
              }
            }
          else if (strcmp(subLine, "element:") == 0)
            {
            vtkDebugMacro("complex vector per element");
            this->VariableMode = VTK_COMPLEX_VECTOR_PER_ELEMENT;
            this->AddVariableType();
            if (sscanf(line, " %*s %*s %*s %*s %*d %*d %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s %s", subLine,
                     subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexVectorsPerElement++;
              }
            else if (sscanf(line, " %*s %*s %*s %*s %*d %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*d %*s %s %s", subLine,
                     subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexVectorsPerElement++;
              }
            else if (sscanf(line, " %*s %*s %*s %*s %s", subLine) == 1)
              {
              this->AddVariableDescription(subLine);
              sscanf(line, " %*s %*s %*s %*s %*s %s %s", subLine, subLine2);
              this->AddVariableFileName(subLine, subLine2);
              this->NumberOfComplexVectorsPerElement++;
              }
            }
          }
        this->NumberOfComplexVariables++;
        }
      else
        {
        vtkErrorMacro("invalid VARIABLE line: " << line);
        delete this->IS;
        this->IS = NULL;
        return 0;
        }
      }
    }
  
  if (strncmp(line, "TIME", 4) == 0)
    {
    // found TIME section
    vtkDebugMacro( << "*** TIME section; VTK cannot handle time currently.");
    delete this->IS;
    this->IS = NULL;
    return 0;
    }
  
  if (strncmp(line, "FILE", 4) == 0)
    {
    // found FILE section
    // There will not be a FILE section without a TIME section, so we should
    // not currently be able to get to this if statement.
    vtkDebugMacro( << "*** FILE section; VTK cannot handle time currently");
    delete this->IS;
    this->IS = NULL;
    return 0;
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadGeometryFile()
{
  char line[256], subLine[256];
  int partId;
  int lineRead;
  
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
  
  // Skip the 2 description lines.
  this->ReadNextDataLine(line);
  sscanf(line, " %*s %s", subLine);
  if (strcmp(subLine, "binary") == 0)
    {
    vtkErrorMacro("Reading binary files is not implemented yet.");
    return 0;
    }
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
  
  while (lineRead && strcmp(line, "part") == 0)
    {
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
          lineRead = this->CreateStructuredPointsOutput(partId, line);
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
      }
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadVariableFiles()
{
  // just point data right now (and not for measured data)
  int i;
  char description[256];
  
  for (i = 0; i < this->NumberOfVariables; i++)
    {
    if (strrchr(this->VariableFileNames[i], '*') != NULL)
      {
      vtkErrorMacro("VTK does not handle time.");
      return 0;
      }
    switch (this->VariableTypes[i])
      {
      case VTK_SCALAR_PER_NODE:
        this->ReadScalarsPerNode(this->VariableFileNames[i],
                                 this->VariableDescriptions[i]);
        break;
      case VTK_VECTOR_PER_NODE:
        this->ReadVectorsPerNode(this->VariableFileNames[i],
                                 this->VariableDescriptions[i]);
        break;
      case VTK_TENSOR_SYMM_PER_NODE:
        this->ReadTensorsPerNode(this->VariableFileNames[i],
                                 this->VariableDescriptions[i]);
        break;
      case VTK_SCALAR_PER_ELEMENT:
        this->ReadScalarsPerElement(this->VariableFileNames[i],
                                    this->VariableDescriptions[i]);
        break;
      case VTK_VECTOR_PER_ELEMENT:
        this->ReadVectorsPerElement(this->VariableFileNames[i],
                                    this->VariableDescriptions[i]);
        break;
      case VTK_TENSOR_SYMM_PER_ELEMENT:
        this->ReadTensorsPerElement(this->VariableFileNames[i],
                                    this->VariableDescriptions[i]);
        break;
      }
    }
  for (i = 0; i < this->NumberOfComplexVariables; i++)
    {
    switch (this->ComplexVariableTypes[i])
      {
      case VTK_COMPLEX_SCALAR_PER_NODE:
        strcpy(description, this->ComplexVariableDescriptions[i]);
        strcat(description, "_r");
        this->ReadScalarsPerNode(this->ComplexVariableFileNames[2*i],
                                 description);
        strcpy(description, this->ComplexVariableDescriptions[i]);
        strcat(description, "_i");
        this->ReadScalarsPerNode(this->ComplexVariableFileNames[2*i+1],
                                 description);
        break;
      case VTK_COMPLEX_VECTOR_PER_NODE:
        strcpy(description, this->ComplexVariableDescriptions[i]);
        strcat(description, "_r");
        this->ReadVectorsPerNode(this->ComplexVariableFileNames[2*i],
                                 description);
        strcpy(description, this->ComplexVariableDescriptions[i]);
        strcat(description, "_i");
        this->ReadVectorsPerNode(this->ComplexVariableFileNames[2*i+1],
                                 description);
        break;
      case VTK_COMPLEX_SCALAR_PER_ELEMENT:
        strcpy(description, this->ComplexVariableDescriptions[i]);
        strcat(description, "_r");
        this->ReadScalarsPerElement(this->ComplexVariableFileNames[2*i],
                                    description);
        strcpy(description, this->ComplexVariableDescriptions[i]);
        strcat(description, "_i");
        this->ReadScalarsPerElement(this->ComplexVariableFileNames[2*i+1],
                                    description);
        break;
      case VTK_COMPLEX_VECTOR_PER_ELEMENT:
        strcpy(description, this->ComplexVariableDescriptions[i]);
        strcat(description, "_r");
        this->ReadVectorsPerElement(this->ComplexVariableFileNames[2*i],
                                    description);
        strcpy(description, this->ComplexVariableDescriptions[i]);
        strcat(description, "_i");
        this->ReadVectorsPerElement(this->ComplexVariableFileNames[2*i+1],
                                    description);
        break;
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadScalarsPerNode(char* fileName, char* description)
{
  char line[256];
  int partId, numPts, i;
  vtkFloatArray *scalars;
  vtkFieldData *fieldData;
  
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

  this->ReadNextDataLine(line); // skip the description line
  
  while (this->ReadNextDataLine(line) &&
         strcmp(line, "part") == 0)
    {
    scalars = vtkFloatArray::New();
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    this->ReadNextDataLine(line); // "coordinates" or "block"
    numPts = this->GetOutput(partId)->GetNumberOfPoints();
    scalars->SetNumberOfTuples(numPts);
    scalars->SetNumberOfComponents(1);
    scalars->Allocate(numPts);
    for (i = 0; i < numPts; i++)
      {
      this->ReadNextDataLine(line);
      scalars->InsertComponent(i, 0, atof(line));
      }
    if (this->GetOutput(partId)->GetPointData()->GetFieldData() == NULL)
      {
      fieldData = vtkFieldData::New();
      fieldData->Allocate(1000);
      this->GetOutput(partId)->GetPointData()->SetFieldData(fieldData);
      fieldData->Delete();
      }
    this->GetOutput(partId)->GetPointData()->GetFieldData()->
      AddArray(scalars, description);
    scalars->Delete();
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadVectorsPerNode(char* fileName, char* description)
{
  char line[256]; 
  int partId, numPts, i, j;
  vtkFloatArray *vectors;
  vtkFieldData *fieldData;
 
  // Initialize
  //
  if (!this->GeometryFileName)
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

  this->ReadNextDataLine(line); // skip the description line

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
int vtkEnSightGoldReader::ReadTensorsPerNode(char* fileName, char* description)
{
  char line[256];
  int partId, numPts, i, j;
  vtkFloatArray *tensors;
  vtkFieldData *fieldData;
  
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
    }

  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadScalarsPerElement(char* fileName,
                                                char* description)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, idx;
  vtkFloatArray *scalars;
  vtkFieldData *fieldData;
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

  this->ReadNextDataLine(line); // skip the description line
  lineRead = this->ReadNextDataLine(line); // "part"
  
  while (lineRead && strcmp(line, "part") == 0)
    {
    scalars = vtkFloatArray::New();
    this->ReadNextDataLine(line);
    partId = atoi(line) - 1; // EnSight starts #ing with 1.
    numCells = this->GetOutput(partId)->GetNumberOfCells();
    this->ReadNextDataLine(line); // element type or "block"
    scalars->SetNumberOfTuples(numCells);
    scalars->SetNumberOfComponents(1);
    scalars->Allocate(numCells);
    
    // need to find out from CellIds how many cells we have of this element
    // type (and what their ids are) -- IF THIS IS NOT A BLOCK SECTION
    if (strcmp(line, "block") == 0)
      {
      for (i = 0; i < numCells; i++)
        {
        this->ReadNextDataLine(line);
        scalar = atof(line);
        scalars->InsertComponent(i, 0, scalar);
        }
      lineRead = this->ReadNextDataLine(line);
      }
    else 
      {
      while (lineRead && strcmp(line, "part") != 0)
        {
        if (strcmp(line, "point") == 0)
          {
          elementType = VTK_ENSIGHT_POINT;
          }
        else if (strcmp(line, "bar2") == 0)
          {
          elementType = VTK_ENSIGHT_BAR2;
          }
        else if (strcmp(line, "bar3") == 0)
          {
          elementType = VTK_ENSIGHT_BAR3;
          }
        else if (strcmp(line, "nsided") == 0)
          {
          elementType = VTK_ENSIGHT_NSIDED;
          }
        else if (strcmp(line, "tria3") == 0)
          {
          elementType = VTK_ENSIGHT_TRIA3;
          }
        else if (strcmp(line, "tria6") == 0)
          {
          elementType = VTK_ENSIGHT_TRIA6;
          }
        else if (strcmp(line, "quad4") == 0)
          {
          elementType = VTK_ENSIGHT_QUAD4;
          }
        else if (strcmp(line, "quad8") == 0)
          {
          elementType = VTK_ENSIGHT_QUAD8;
          }
        else if (strcmp(line, "tetra4") == 0)
          {
          elementType = VTK_ENSIGHT_TETRA4;
          }
        else if (strcmp(line, "tetra10") == 0)
          {
          elementType = VTK_ENSIGHT_TETRA10;
          }
        else if (strcmp(line, "pyramid5") == 0)
          {
          elementType = VTK_ENSIGHT_PYRAMID5;
          }
        else if (strcmp(line, "pyramid13") == 0)
          {
          elementType = VTK_ENSIGHT_PYRAMID13;
          }
        else if (strcmp(line, "hexa8") == 0)
          {
          elementType = VTK_ENSIGHT_HEXA8;
          }
        else if (strcmp(line, "hexa20") == 0)
          {
          elementType = VTK_ENSIGHT_HEXA20;
          }
        else if (strcmp(line, "penta6") == 0)
          {
          elementType = VTK_ENSIGHT_PENTA6;
          }
        else if (strcmp(line, "penta15") == 0)
          {
          elementType = VTK_ENSIGHT_PENTA15;
          }
        idx = this->UnstructuredPartIds->IsId(partId);
        numCellsPerElement = this->CellIds[idx][elementType]->GetNumberOfIds();
        for (i = 0; i < numCellsPerElement; i++)
          {
          this->ReadNextDataLine(line);
          scalar = atof(line);
          scalars->InsertComponent(this->CellIds[idx][elementType]->GetId(i),
                                   0, scalar);
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
      AddArray(scalars, description);
    scalars->Delete();
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightGoldReader::ReadVectorsPerElement(char* fileName,
                                                char* description)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray *vectors;
  vtkFieldData *fieldData;
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
        if (strcmp(line, "point") == 0)
          {
          elementType = VTK_ENSIGHT_POINT;
          }
        else if (strcmp(line, "bar2") == 0)
          {
          elementType = VTK_ENSIGHT_BAR2;
          }
        else if (strcmp(line, "bar3") == 0)
          {
          elementType = VTK_ENSIGHT_BAR3;
          }
        else if (strcmp(line, "nsided") == 0)
          {
          elementType = VTK_ENSIGHT_NSIDED;
          }
        else if (strcmp(line, "tria3") == 0)
          {
          elementType = VTK_ENSIGHT_TRIA3;
          }
        else if (strcmp(line, "tria6") == 0)
          {
          elementType = VTK_ENSIGHT_TRIA6;
          }
        else if (strcmp(line, "quad4") == 0)
          {
          elementType = VTK_ENSIGHT_QUAD4;
          }
        else if (strcmp(line, "quad8") == 0)
          {
          elementType = VTK_ENSIGHT_QUAD8;
          }
        else if (strcmp(line, "tetra4") == 0)
          {
          elementType = VTK_ENSIGHT_TETRA4;
          }
        else if (strcmp(line, "tetra10") == 0)
          {
          elementType = VTK_ENSIGHT_TETRA10;
          }
        else if (strcmp(line, "pyramid5") == 0)
          {
          elementType = VTK_ENSIGHT_PYRAMID5;
          }
        else if (strcmp(line, "pyramid13") == 0)
          {
          elementType = VTK_ENSIGHT_PYRAMID13;
          }
        else if (strcmp(line, "hexa8") == 0)
          {
          elementType = VTK_ENSIGHT_HEXA8;
          }
        else if (strcmp(line, "hexa20") == 0)
          {
          elementType = VTK_ENSIGHT_HEXA20;
          }
        else if (strcmp(line, "penta6") == 0)
          {
          elementType = VTK_ENSIGHT_PENTA6;
          }
        else if (strcmp(line, "penta15") == 0)
          {
          elementType = VTK_ENSIGHT_PENTA15;
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
int vtkEnSightGoldReader::ReadTensorsPerElement(char* fileName,
                                                char* description)
{
  char line[256];
  int partId, numCells, numCellsPerElement, i, j, idx;
  vtkFloatArray *tensors;
  vtkFieldData *fieldData;
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
        if (strcmp(line, "point") == 0)
          {
          elementType = VTK_ENSIGHT_POINT;
          }
        else if (strcmp(line, "bar2") == 0)
          {
          elementType = VTK_ENSIGHT_BAR2;
          }
        else if (strcmp(line, "bar3") == 0)
          {
          elementType = VTK_ENSIGHT_BAR3;
          }
        else if (strcmp(line, "nsided") == 0)
          {
          elementType = VTK_ENSIGHT_NSIDED;
          }
        else if (strcmp(line, "tria3") == 0)
          {
          elementType = VTK_ENSIGHT_TRIA3;
          }
        else if (strcmp(line, "tria6") == 0)
          {
          elementType = VTK_ENSIGHT_TRIA6;
          }
        else if (strcmp(line, "quad4") == 0)
          {
          elementType = VTK_ENSIGHT_QUAD4;
          }
        else if (strcmp(line, "quad8") == 0)
          {
          elementType = VTK_ENSIGHT_QUAD8;
          }
        else if (strcmp(line, "tetra4") == 0)
          {
          elementType = VTK_ENSIGHT_TETRA4;
          }
        else if (strcmp(line, "tetra10") == 0)
          {
          elementType = VTK_ENSIGHT_TETRA10;
          }
        else if (strcmp(line, "pyramid5") == 0)
          {
          elementType = VTK_ENSIGHT_PYRAMID5;
          }
        else if (strcmp(line, "pyramid13") == 0)
          {
          elementType = VTK_ENSIGHT_PYRAMID13;
          }
        else if (strcmp(line, "hexa8") == 0)
          {
          elementType = VTK_ENSIGHT_HEXA8;
          }
        else if (strcmp(line, "hexa20") == 0)
          {
          elementType = VTK_ENSIGHT_HEXA20;
          }
        else if (strcmp(line, "penta6") == 0)
          {
          elementType = VTK_ENSIGHT_PENTA6;
          }
        else if (strcmp(line, "penta15") == 0)
          {
          elementType = VTK_ENSIGHT_PENTA15;
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
int vtkEnSightGoldReader::CreateUnstructuredGridOutput(int partId,
                                                       char line[256])
{
  int lineRead = 1;
  char subLine[256];
  int i, j;
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
        points->InsertNextPoint(atoi(line), 0, 0);
        }
      for (i = 0; i < numPts; i++)
        {
        this->ReadNextDataLine(line);
        points->GetPoint(i, point);
        points->SetPoint(i, point[0], atoi(line), 0);
        }
      for (i = 0; i < numPts; i++)
        {
        this->ReadNextDataLine(line);
        points->GetPoint(i, point);
        points->SetPoint(i, point[0], point[1], atoi(line));
        }
      
      lineRead = this->ReadNextDataLine(line);
      sscanf(line, " %s", subLine);
      
      if (isdigit(subLine[0]))
        { // necessary if node ids were listed
        for (i = 0; i < numPts; i++)
          {
          points->GetPoint(i, point);
          points->SetPoint(i, point[1], point[2], atoi(line));
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
      
      nodeIds = new int[1];        
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
      
      nodeIds = new int[2];
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
      nodeIds = new int[2];
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
      sscanf(line, " %s", subLine);
      if (isdigit(subLine[0]))
        {
        // We still need to read in the node ids for each element.
        for (i = 0; i < numElements; i++)
          {
          numNodes = atoi(newLines[numElements+i]);
          nodeIds = new int[numNodes];
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
          nodeIds = new int[numNodes];
          strcpy(formatLine, "");
          strcpy(tempLine, "");
          for (j = 0; j < numNodes; j++)
            {
            strcat(formatLine, " %s");
            sscanf(newLines[numElements+i], formatLine,
                   nodeIds[numNodes-j]);
            strcat(tempLine, " %*s");
            strcpy(formatLine, tempLine);
            }
          cellId = ((vtkUnstructuredGrid*)this->GetOutput(partId))->
            InsertNextCell(VTK_POLYGON, numNodes, nodeIds);
          this->CellIds[idx][VTK_ENSIGHT_NSIDED]->InsertNextId(cellId);
          delete [] nodeIds;
          }
        }
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
      
      nodeIds = new int[4];
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
      
      nodeIds = new int[4];
      this->ReadNextDataLine(line);
      numElements = atoi(line);
      this->ReadNextDataLine(line);
      if (sscanf(line, " %d %d %d %d", &nodeIds[0], &nodeIds[1],
                 &nodeIds[2], nodeIds[3]) != 4)
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
      
      nodeIds = new int[5];
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
      
      nodeIds = new int[8];
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
      
      nodeIds = new int[6];
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
  vtkScalars *xCoords = vtkScalars::New();
  vtkScalars *yCoords = vtkScalars::New();
  vtkScalars *zCoords = vtkScalars::New();
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
  
  for (i = 0; i < dimensions[0]; i++)
    {
    this->ReadNextDataLine(line);
    xCoords->InsertNextScalar(atof(line));
    }
  for (i = 0; i < dimensions[1]; i++)
    {
    this->ReadNextDataLine(line);
    yCoords->InsertNextScalar(atof(line));
    }
  for (i = 0; i < dimensions[2]; i++)
    {
    this->ReadNextDataLine(line);
    zCoords->InsertNextScalar(atof(line));
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
int vtkEnSightGoldReader::CreateStructuredPointsOutput(int partId,
                                                       char line[256])
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
    vtkDebugMacro("creating new structured grid output");
    vtkStructuredPoints* spoints = vtkStructuredPoints::New();
    this->SetNthOutput(partId, spoints);
    spoints->Delete();
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
  ((vtkStructuredPoints*)this->GetOutput(partId))->SetDimensions(dimensions);
  ((vtkStructuredPoints*)this->GetOutput(partId))->
    SetWholeExtent(0, dimensions[0]-1, 0, dimensions[1]-1, 0, dimensions[2]-1);
  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &origin[0], &origin[1], &origin[2]);
  ((vtkStructuredPoints*)this->GetOutput(partId))->SetOrigin(origin[0],
                                                             origin[1],
                                                             origin[2]);
  this->ReadNextDataLine(line);
  sscanf(line, " %d %d %d", &delta[0], &delta[1], &delta[2]);
  ((vtkStructuredPoints*)this->GetOutput(partId))->SetSpacing(delta[0],
                                                              delta[1],
                                                              delta[2]);
  
  if (iblanked)
    {
    vtkWarningMacro("VTK does not handle blanking for structured points.");
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
void vtkEnSightGoldReader::AddVariableFileName(char* fileName1,
					       char* fileName2)
{
  int size;
  int i;
  
  if (this->VariableMode < 8)
    {
    size = this->NumberOfVariables;

    char** newFileNameList = new char *[size]; // temporary array
  
    // copy file names to temporary array
    for (i = 0; i < size; i++)
      {
      newFileNameList[i] = new char[strlen(this->VariableFileNames[i]) + 1];
      strcpy(newFileNameList[i], this->VariableFileNames[i]);
      delete [] this->VariableFileNames[i];
      }
    delete [] this->VariableFileNames;
    
    // make room for new file name
    this->VariableFileNames = new char *[size+1];
    
    // copy existing file names back to first array
    for (i = 0; i < size; i++)
      {
      this->VariableFileNames[i] = new char[strlen(newFileNameList[i]) + 1];
      strcpy(this->VariableFileNames[i], newFileNameList[i]);
      delete [] newFileNameList[i];
      }
    delete [] newFileNameList;
    
    // add new file name at end of first array
    this->VariableFileNames[size] = new char[strlen(fileName1) + 1];
    strcpy(this->VariableFileNames[size], fileName1);
    vtkDebugMacro( << "file name: " << this->VariableFileNames[size]);
    }
  else
    {
    size = this->NumberOfComplexVariables;
    
    char** newFileNameList = new char *[2 * size]; // temporary array
    
    // copy file names to temporary array
    for (i = 0; i < 2*size; i++)
      {
      newFileNameList[i] =
	new char[strlen(this->ComplexVariableFileNames[i]) + 1];
      strcpy(newFileNameList[i], this->ComplexVariableFileNames[i]);
      delete [] this->ComplexVariableFileNames[i];
      }
    delete [] this->ComplexVariableFileNames;

    // make room for new file name
    this->ComplexVariableFileNames = new char *[2*(size+1)];
    
    // copy existing file names back to first array
    for (i = 0; i < 2*size; i++)
      {
      this->ComplexVariableFileNames[i] =
	new char[strlen(newFileNameList[i]) + 1];
      strcpy(this->ComplexVariableFileNames[i], newFileNameList[i]);
      delete [] newFileNameList[i];
      }
    delete [] newFileNameList;
    
    // add new file name at end of first array
    this->ComplexVariableFileNames[2*size] = new char[strlen(fileName1) + 1];
    strcpy(this->ComplexVariableFileNames[2*size], fileName1);
    vtkDebugMacro("real file name: "
                  << this->ComplexVariableFileNames[2*size]);
    this->ComplexVariableFileNames[2*size+1] = new char[strlen(fileName2) + 1];
    strcpy(this->ComplexVariableFileNames[2*size+1], fileName2);
    vtkDebugMacro("imag. file name: "
                  << this->ComplexVariableFileNames[2*size+1]);
    }
}

//----------------------------------------------------------------------------
void vtkEnSightGoldReader::AddVariableDescription(char* description)
{
  int size;
  int i;
  
  if (this->VariableMode < 8)
    {
    size = this->NumberOfVariables;

    char ** newDescriptionList = new char *[size]; // temporary array    
    
    // copy descriptions to temporary array
    for (i = 0; i < size; i++)
      {
      newDescriptionList[i] =
	new char[strlen(this->VariableDescriptions[i]) + 1];
      strcpy(newDescriptionList[i], this->VariableDescriptions[i]);
      delete [] this->VariableDescriptions[i];
      }
    delete [] this->VariableDescriptions;
    
    // make room for new description
    this->VariableDescriptions = new char *[size+1];
    
    // copy existing descriptions back to first array
    for (i = 0; i < size; i++)
      {
      this->VariableDescriptions[i] =
	new char[strlen(newDescriptionList[i]) + 1];
      strcpy(this->VariableDescriptions[i], newDescriptionList[i]);
      delete [] newDescriptionList[i];
      }
    delete [] newDescriptionList;
    
    // add new description at end of first array
    this->VariableDescriptions[size] = new char[strlen(description) + 1];
    strcpy(this->VariableDescriptions[size], description);
    vtkDebugMacro("description: " << this->VariableDescriptions[size]);
    }
  else
    {
    size = this->NumberOfComplexVariables;
    
    char ** newDescriptionList = new char *[size]; // temporary array    
    
    // copy descriptions to temporary array
    for (i = 0; i < size; i++)
      {
      newDescriptionList[i] =
        new char[strlen(this->ComplexVariableDescriptions[i]) + 1];
      strcpy(newDescriptionList[i], this->ComplexVariableDescriptions[i]);
      delete [] this->ComplexVariableDescriptions[i];
      }
    delete [] this->ComplexVariableDescriptions;
    
    // make room for new description
    this->ComplexVariableDescriptions = new char *[size+1];
    
    // copy existing descriptions back to first array
    for (i = 0; i < size; i++)
      {
      this->ComplexVariableDescriptions[i] =
        new char[strlen(newDescriptionList[i]) + 1];
      strcpy(this->ComplexVariableDescriptions[i], newDescriptionList[i]);
      delete [] newDescriptionList[i];
      }
    delete [] newDescriptionList;
    
    // add new description at end of first array
    this->ComplexVariableDescriptions[size] =
      new char[strlen(description) + 1];
    strcpy(this->ComplexVariableDescriptions[size], description);
    vtkDebugMacro("description: "
                  << this->ComplexVariableDescriptions[size]);
    }
}

//----------------------------------------------------------------------------
void vtkEnSightGoldReader::AddVariableType()
{
  int size;
  int i;
  int *types;
  
  // Figure out what the size of the variable type array is.
  if (this->VariableMode < 8)
    {
    size = this->NumberOfVariables;
  
    types = new int[size];
    
    for (i = 0; i < size; i++)
      {
      types[i] = this->VariableTypes[i];
      }
    delete [] this->VariableTypes;
    
    this->VariableTypes = new int[size+1];
    for (i = 0; i < size; i++)
      {
      this->VariableTypes[i] = types[i];
      }
    delete [] types;
    this->VariableTypes[size] = this->VariableMode;
    vtkDebugMacro("variable type: " << this->VariableTypes[size]);
    }
  else
    {
    size = this->NumberOfComplexVariables;

    if (size > 0)
      {
      types = new int[size];
      for (i = 0; i < size; i++)
	{
	types[i] = this->ComplexVariableTypes[i];
	}
      delete [] this->ComplexVariableTypes;
      }
    
    this->ComplexVariableTypes = new int[size+1];
    for (i = 0; i < size; i++)
      {
      this->ComplexVariableTypes[i] = types[i];
      }
    
    if (size > 0)
      {
      delete [] types;
      }
    this->ComplexVariableTypes[size] = this->VariableMode;
    vtkDebugMacro("complex variable type: "
                  << this->ComplexVariableTypes[size]);
    }
}

// Internal function to read in a line up to 256 characters.
// Returns zero if there was an error.
int vtkEnSightGoldReader::ReadLine(char result[256])
{
  this->IS->getline(result,256);
  if (this->IS->eof()) 
    {
    return 0;
    }
  
  return 1;
}

// Internal function that skips blank lines and comment lines
// and reads the next line it finds (up to 256 characters).
// Returns 0 is there was an error.
int vtkEnSightGoldReader::ReadNextDataLine(char result[256])
{
  int value;
  char subline[256];
  
  value = this->ReadLine(result);
  sscanf(result, " %s", subline);
  
  while((strcmp(result, "") == 0 || subline[0] == '#') && value != 0)
    {
    value = this->ReadLine(result);
    sscanf(result, " %s", subline);
    }
  return value;
}

int vtkEnSightGoldReader::GetNumberOfVariables(int type)
{
  switch (type)
    {
    case VTK_SCALAR_PER_NODE:
      return this->GetNumberOfScalarsPerNode();
    case VTK_VECTOR_PER_NODE:
      return this->GetNumberOfVectorsPerNode();
    case VTK_TENSOR_SYMM_PER_NODE:
      return this->GetNumberOfTensorsSymmPerNode();
    case VTK_SCALAR_PER_ELEMENT:
      return this->GetNumberOfScalarsPerElement();
    case VTK_VECTOR_PER_ELEMENT:
      return this->GetNumberOfVectorsPerElement();
    case VTK_TENSOR_SYMM_PER_ELEMENT:
      return this->GetNumberOfTensorsSymmPerElement();
    case VTK_SCALAR_PER_MEASURED_NODE:
      return this->GetNumberOfScalarsPerMeasuredNode();
    case VTK_VECTOR_PER_MEASURED_NODE:
      return this->GetNumberOfVectorsPerMeasuredNode();
    case VTK_COMPLEX_SCALAR_PER_NODE:
      return this->GetNumberOfComplexScalarsPerNode();
    case VTK_COMPLEX_VECTOR_PER_NODE:
      return this->GetNumberOfComplexVectorsPerNode();
    case VTK_COMPLEX_SCALAR_PER_ELEMENT:
      return this->GetNumberOfComplexScalarsPerElement();
    case VTK_COMPLEX_VECTOR_PER_ELEMENT:
      return this->GetNumberOfComplexVectorsPerElement();
    default:
      vtkWarningMacro("unknow variable type");
      return -1;
    }
}

char* vtkEnSightGoldReader::GetDescription(int n)
{
  if (n < this->NumberOfVariables)
    {
    return this->VariableDescriptions[n];
    }
  return NULL;
}

char* vtkEnSightGoldReader::GetComplexDescription(int n)
{
  if (n < this->NumberOfComplexVariables)
    {
    return this->ComplexVariableDescriptions[n];
    }
  return NULL;
}

char* vtkEnSightGoldReader::GetDescription(int n, int type)
{
  int i, numMatches = 0;
  
  if (type < 8)
    {
    for (i = 0; i < this->NumberOfVariables; i++)
      {
      if (this->VariableTypes[i] == type)
        {
        if (numMatches == n)
          {
          return this->VariableDescriptions[i];
          }
        else
          {
          numMatches++;
          }
        }
      }
    }
  else
    {
    for (i = 0; i < this->NumberOfVariables; i++)
      {
      if (this->ComplexVariableTypes[i] == type)
        {
        if (numMatches == n)
          {
          return this->ComplexVariableDescriptions[i];
          }
        else
          {
          numMatches++;
          }
        }
      }
    }
  
  return NULL;
}

void vtkEnSightGoldReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetSource::PrintSelf(os,indent);

  os << indent << "CaseFileName: "
     << (this->CaseFileName ? this->CaseFileName : "(none)") << endl;
  os << indent << "FilePath: "
     << (this->FilePath ? this->FilePath : "(none)") << endl;
  os << indent << "NumberOfComplexScalarsPerNode: "
     << this->NumberOfComplexScalarsPerNode << endl;
  os << indent << "NumberOfVectorsPerElement :"
     << this->NumberOfVectorsPerElement << endl;
  os << indent << "NumberOfTensorsSymmPerElement: "
     << this->NumberOfTensorsSymmPerElement << endl;
  os << indent << "NumberOfComplexVectorsPerNode: "
     << this->NumberOfComplexVectorsPerNode << endl;
  os << indent << "NumberOfScalarsPerElement: "
     << this->NumberOfScalarsPerElement << endl;
  os << indent << "NumberOfComplexVectorsPerElement: "
     << this->NumberOfComplexVectorsPerElement << endl;
  os << indent << "NumberOfComplexScalarsPerElement: "
     << this->NumberOfComplexScalarsPerElement << endl;
  os << indent << "NumberOfTensorsSymmPerNode: "
     << this->NumberOfTensorsSymmPerNode << endl;
  os << indent << "NumberOfScalarsPerMeasuredNode: "
     << this->NumberOfScalarsPerMeasuredNode << endl;
  os << indent << "NumberOfVectorsPerMeasuredNode: "
     << this->NumberOfVectorsPerMeasuredNode << endl;
  os << indent << "NumberOfScalarsPerNode: "
     << this->NumberOfScalarsPerNode << endl;
  os << indent << "NumberOfVectorsPerNode: "
     << this->NumberOfVectorsPerNode << endl;
}
