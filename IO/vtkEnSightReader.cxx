/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightReader.cxx
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
#include "vtkEnSightReader.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkFloatArray.h"
#include <ctype.h>

//----------------------------------------------------------------------------
vtkEnSightReader::vtkEnSightReader()
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
  
  this->NumberOfGeometryParts = 0;

  this->NumberOfMeasuredPoints = 0;
  this->MeasuredNodeIds = vtkIdList::New();
}

//----------------------------------------------------------------------------
vtkEnSightReader::~vtkEnSightReader()
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
  
    
  this->MeasuredNodeIds->Delete();
  this->MeasuredNodeIds = NULL;
}

//----------------------------------------------------------------------------
void vtkEnSightReader::Execute()
{
  if (!this->ReadCaseFile())
    {
    vtkErrorMacro("error reading case file");
    return;
    }
  if (this->GeometryFileName)
    {
    if (!this->ReadGeometryFile())
      {
      vtkErrorMacro("error reading geometry file");
      return;
      }
    }
  if (this->MeasuredFileName)
    {
    if (!this->ReadMeasuredGeometryFile())
      {
      vtkErrorMacro("error reading measured geometry file");
      return;
      }
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
void vtkEnSightReader::Update()
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
int vtkEnSightReader::ReadCaseFile()
{
  char line[256];
  char subLine[256], subLine2[256];
  int stringRead;
  
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
    
    stringRead = sscanf(line, " %*s %*s %s", subLine);
    if (stringRead == 1)
      {
      if (strcmp(subLine, "gold") == 0 &&
          strcmp(this->GetClassName(), "vtkEnSight6Reader") == 0)
        {
        // The class is vtkEnSight6Reader, but the case file says "gold".
        vtkErrorMacro("This is not an EnSight6 file.");
        delete this->IS;
        this->IS = NULL;
        return 0;
        }
      }
    else
      {
      if (strcmp(this->GetClassName(), "vtkEnSightGoldReader") == 0)
        {
        // The class is vtkEnSightGoldReader, but the case file does
        // not say "gold".
        vtkErrorMacro("This is not an EnSight Gold file.");
        delete this->IS;
        this->IS = NULL;
        return 0;
        }
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
      else if (strncmp(line, "measured:", 9) == 0)
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
      else if (strncmp(line, "match:", 6) == 0)
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
            if (sscanf(line, " %*s %*s %*s %*s %*d %*d %*s %s", subLine) == 1)
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
              this->AddVariableFileName(subLine, subLine2);
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
    vtkErrorMacro( << "*** TIME section; VTK cannot handle time currently.");
    delete this->IS;
    this->IS = NULL;
    return 0;
    }
  
  if (strncmp(line, "FILE", 4) == 0)
    {
    // found FILE section
    // There will not be a FILE section without a TIME section, so we should
    // not currently be able to get to this if statement.
    vtkErrorMacro( << "*** FILE section; VTK cannot handle time currently");
    delete this->IS;
    this->IS = NULL;
    return 0;
    }
  
  delete this->IS;
  this->IS = NULL;
  return 1;
}

//----------------------------------------------------------------------------
int vtkEnSightReader::ReadVariableFiles()
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
      case VTK_SCALAR_PER_MEASURED_NODE:
        this->ReadScalarsPerNode(this->VariableFileNames[i],
                                 this->VariableDescriptions[i], 1);
        break;
      case VTK_VECTOR_PER_NODE:
        this->ReadVectorsPerNode(this->VariableFileNames[i],
                                 this->VariableDescriptions[i]);
        break;
      case VTK_VECTOR_PER_MEASURED_NODE:
        this->ReadVectorsPerNode(this->VariableFileNames[i],
                                 this->VariableDescriptions[i], 1);
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
        this->ReadScalarsPerNode(this->ComplexVariableFileNames[2*i],
                                 this->ComplexVariableDescriptions[i], 0, 2);
        this->ReadScalarsPerNode(this->ComplexVariableFileNames[2*i+1],
                                 this->ComplexVariableDescriptions[i],
                                 0, 2, 1);
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
        this->ReadScalarsPerElement(this->ComplexVariableFileNames[2*i],
                                    this->ComplexVariableDescriptions[i], 2);
        this->ReadScalarsPerElement(this->ComplexVariableFileNames[2*i+1],
                                    this->ComplexVariableDescriptions[i], 2, 1);
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
void vtkEnSightReader::AddVariableFileName(char* fileName1,
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
void vtkEnSightReader::AddVariableDescription(char* description)
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
void vtkEnSightReader::AddVariableType()
{
  int size;
  int i;
  int *types = NULL;
  
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
int vtkEnSightReader::ReadLine(char result[256])
{
  this->IS->getline(result,256);
//  if (this->IS->eof()) 
  if (this->IS->fail())
    {
    return 0;
    }
  
  return 1;
}

// Internal function that skips blank lines and comment lines
// and reads the next line it finds (up to 256 characters).
// Returns 0 is there was an error.
int vtkEnSightReader::ReadNextDataLine(char result[256])
{
  int value, sublineFound;
  char subline[256];
  
  value = this->ReadLine(result);
  sublineFound = sscanf(result, " %s", subline);
  
  while((strcmp(result, "") == 0 || subline[0] == '#' || sublineFound < 1) &&
        value != 0)
    {
    value = this->ReadLine(result);
    sublineFound = sscanf(result, " %s", subline);
    }
  return value;
}

int vtkEnSightReader::GetNumberOfVariables(int type)
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

char* vtkEnSightReader::GetDescription(int n)
{
  if (n < this->NumberOfVariables)
    {
    return this->VariableDescriptions[n];
    }
  return NULL;
}

char* vtkEnSightReader::GetComplexDescription(int n)
{
  if (n < this->NumberOfComplexVariables)
    {
    return this->ComplexVariableDescriptions[n];
    }
  return NULL;
}

char* vtkEnSightReader::GetDescription(int n, int type)
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

int vtkEnSightReader::GetElementType(char* line)
{
  if (strcmp(line, "point") == 0)
    {
    return VTK_ENSIGHT_POINT;
    }
  else if (strcmp(line, "bar2") == 0)
    {
    return VTK_ENSIGHT_BAR2;
    }
  else if (strcmp(line, "bar3") == 0)
    {
    return VTK_ENSIGHT_BAR3;
    }
  else if (strcmp(line, "nsided") == 0)
    {
    return VTK_ENSIGHT_NSIDED;
    }
  else if (strcmp(line, "tria3") == 0)
    {
    return VTK_ENSIGHT_TRIA3;
    }
  else if (strcmp(line, "tria6") == 0)
    {
    return VTK_ENSIGHT_TRIA6;
    }
  else if (strcmp(line, "quad4") == 0)
    {
    return VTK_ENSIGHT_QUAD4;
    }
  else if (strcmp(line, "quad8") == 0)
    {
    return VTK_ENSIGHT_QUAD8;
    }
  else if (strcmp(line, "tetra4") == 0)
    {
    return VTK_ENSIGHT_TETRA4;
    }
  else if (strcmp(line, "tetra10") == 0)
    {
    return VTK_ENSIGHT_TETRA10;
    }
  else if (strcmp(line, "pyramid5") == 0)
    {
    return VTK_ENSIGHT_PYRAMID5;
    }
  else if (strcmp(line, "pyramid13") == 0)
    {
    return VTK_ENSIGHT_PYRAMID13;
    }
  else if (strcmp(line, "hexa8") == 0)
    {
    return VTK_ENSIGHT_HEXA8;
    }
  else if (strcmp(line, "hexa20") == 0)
    {
    return VTK_ENSIGHT_HEXA20;
    }
  else if (strcmp(line, "penta6") == 0)
    {
    return VTK_ENSIGHT_PENTA6;
    }
  else if (strcmp(line, "penta15") == 0)
    {
    return VTK_ENSIGHT_PENTA15;
    }
  else
    {
    return -1;
    }
}

void vtkEnSightReader::SetCaseFileName(char* fileName)
{
  char *endingSlash = NULL;
  char *path, *newFileName;
  int position, numChars;
  
  if ( this->CaseFileName && fileName && (!strcmp(this->CaseFileName, fileName)))
    {
    return;
    }
  if (this->CaseFileName)
    {
    delete [] this->CaseFileName;
    }
  if (fileName)
    {
    this->CaseFileName = new char[strlen(fileName)+1];
    strcpy(this->CaseFileName, fileName);
    }
   else
    {
    this->CaseFileName = NULL;
    }
  
  // strip off the path and save it as FilePath if it was included in the filename
  if ((endingSlash = strrchr(this->CaseFileName, '/')))
    {
    position = endingSlash - this->CaseFileName + 1;
    path = new char[position + 1];
    numChars = strlen(this->CaseFileName);
    newFileName = new char[numChars - position + 1];
    strcpy(path, "");
    strncat(path, this->CaseFileName, position);
    this->SetFilePath(path);
    strcpy(newFileName, this->CaseFileName + position);
    strcpy(this->CaseFileName, newFileName);
    delete [] path;
    delete [] newFileName;
    }
      
  this->Modified();
}

int vtkEnSightReader::GetVariableType(int n)
{
  if (n < this->NumberOfVariables)
    {
    return this->VariableTypes[n];
    }
  return -1;
}

int vtkEnSightReader::GetComplexVariableType(int n)
{
  if (n < this->NumberOfComplexVariables)
    {
    return this->ComplexVariableTypes[n];
    }
  return -1;
}

void vtkEnSightReader::PrintSelf(ostream& os, vtkIndent indent)
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
