/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetReader.cxx
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
#include "vtkDataSetReader.h"
#include "vtkPolyDataReader.h"
#include "vtkStructuredPointsReader.h"
#include "vtkStructuredGridReader.h"
#include "vtkRectilinearGridReader.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkDataSetReader* vtkDataSetReader::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDataSetReader");
  if(ret)
    {
    return (vtkDataSetReader*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDataSetReader;
}

vtkDataSetReader::vtkDataSetReader()
{
}

vtkDataSetReader::~vtkDataSetReader()
{
}

vtkDataSet * vtkDataSetReader::GetOutput() 
{
  // check to see if an execute is necessary.
  if (this->Outputs && this->Outputs[0] && 
      this->Outputs[0]->GetUpdateTime() > this->GetMTime())
    {
    return (vtkDataSet *)(this->Outputs[0]);
    }
  
  // The filename might have changed (changing the output).
  // We need to re execute.
  if (this->GetFileName() == NULL && 
      (this->GetReadFromInputString() == 0 || 
       this->GetInputString() == NULL))
    {
    vtkWarningMacro(<< "FileName must be set");
    return (vtkDataSet *) NULL;
    }

  this->Execute();
  if (this->Outputs == NULL)
    {
    return NULL;
    }
  else
    {
    return (vtkDataSet *)this->Outputs[0];
    }
}

void vtkDataSetReader::Execute()
{
  char line[256];
  vtkDataObject *output;
  
  vtkDebugMacro(<<"Reading vtk dataset...");

  if (!this->OpenVTKFile() || !this->ReadHeader())
    {
    return;
    }

  // Determine dataset type
  //
  if (!this->ReadString(line))
    {
    vtkErrorMacro(<< "Premature EOF reading dataset keyword");
    return;
    }

  if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
    {
    // See if type is recognized.
    //
    if (!this->ReadString(line))
      {
      vtkErrorMacro(<< "Premature EOF reading type");
      this->CloseVTKFile ();
      return;
      }

    this->CloseVTKFile();
    if ( ! strncmp(this->LowerCase(line),"polydata",8) )
      {
      vtkPolyDataReader *preader = vtkPolyDataReader::New();
      preader->SetFileName(this->GetFileName());
      preader->SetInputString(this->GetInputString(),
			      this->GetInputStringLength());
      preader->SetReadFromInputString(this->GetReadFromInputString());
      preader->SetScalarsName(this->GetScalarsName());
      preader->SetVectorsName(this->GetVectorsName());
      preader->SetNormalsName(this->GetNormalsName());
      preader->SetTensorsName(this->GetTensorsName());
      preader->SetTCoordsName(this->GetTCoordsName());
      preader->SetLookupTableName(this->GetLookupTableName());
      preader->SetFieldDataName(this->GetFieldDataName());
      preader->Update();
      // Can we use the old output?
      output = this->Outputs ? this->Outputs[0] : NULL;
      if (output && strcmp(output->GetClassName(), "vtkPolyData") == 0)
        {
        output->ShallowCopy(preader->GetOutput());
        }
      else
        {
        this->SetNthOutput(0, preader->GetOutput());
        }

      preader->Delete();
      }

    else if ( ! strncmp(line,"structured_points",17) )
      {
      vtkStructuredPointsReader *preader = vtkStructuredPointsReader::New();
      preader->SetFileName(this->GetFileName());
      preader->SetInputString(this->GetInputString(),
			      this->GetInputStringLength());
      preader->SetReadFromInputString(this->GetReadFromInputString());
      preader->SetScalarsName(this->GetScalarsName());
      preader->SetVectorsName(this->GetVectorsName());
      preader->SetNormalsName(this->GetNormalsName());
      preader->SetTensorsName(this->GetTensorsName());
      preader->SetTCoordsName(this->GetTCoordsName());
      preader->SetLookupTableName(this->GetLookupTableName());
      preader->SetFieldDataName(this->GetFieldDataName());
      preader->Update();

      // Can we use the old output?
      output = this->Outputs ? this->Outputs[0] : NULL;
      if (output && strcmp(output->GetClassName(), "vtkStructuredPoints") == 0)
        {
        output->ShallowCopy(preader->GetOutput());
        }
      else
        {
        this->SetNthOutput(0, preader->GetOutput());
        }

      preader->Delete();
      }

    else if ( ! strncmp(line,"structured_grid",15) )
      {
      vtkStructuredGridReader *preader = vtkStructuredGridReader::New();
      preader->SetFileName(this->GetFileName());
      preader->SetInputString(this->GetInputString(),
			      this->GetInputStringLength());
      preader->SetReadFromInputString(this->GetReadFromInputString());
      preader->SetScalarsName(this->GetScalarsName());
      preader->SetVectorsName(this->GetVectorsName());
      preader->SetNormalsName(this->GetNormalsName());
      preader->SetTensorsName(this->GetTensorsName());
      preader->SetTCoordsName(this->GetTCoordsName());
      preader->SetLookupTableName(this->GetLookupTableName());
      preader->SetFieldDataName(this->GetFieldDataName());
      preader->Update();

      // Can we use the old output?
      output = this->Outputs ? this->Outputs[0] : NULL;
      if (output && strcmp(output->GetClassName(), "vtkStructuredGrid") == 0)
        {
        output->ShallowCopy(preader->GetOutput());
        }
      else
        {
        this->SetNthOutput(0, preader->GetOutput());
        }

      preader->Delete();
      }

    else if ( ! strncmp(line,"rectilinear_grid",16) )
      {
      vtkRectilinearGridReader *preader = vtkRectilinearGridReader::New();
      preader->SetFileName(this->GetFileName());
      preader->SetInputString(this->GetInputString(),
			      this->GetInputStringLength());
      preader->SetReadFromInputString(this->GetReadFromInputString());
      preader->SetScalarsName(this->GetScalarsName());
      preader->SetVectorsName(this->GetVectorsName());
      preader->SetNormalsName(this->GetNormalsName());
      preader->SetTensorsName(this->GetTensorsName());
      preader->SetTCoordsName(this->GetTCoordsName());
      preader->SetLookupTableName(this->GetLookupTableName());
      preader->SetFieldDataName(this->GetFieldDataName());
      preader->Update();

      // Can we use the old output?
      output = this->Outputs ? this->Outputs[0] : NULL;
      if (output && strcmp(output->GetClassName(), "vtkRectilinearGrid") == 0)
        {
        output->ShallowCopy(preader->GetOutput());
        }
      else
        {
        this->SetNthOutput(0, preader->GetOutput());
        }

      preader->Delete();
      }

    else if ( ! strncmp(line,"unstructured_grid",17) )
      {
      vtkUnstructuredGridReader *preader = vtkUnstructuredGridReader::New();
      preader->SetFileName(this->GetFileName());
      preader->SetInputString(this->GetInputString(),
			      this->GetInputStringLength());
      preader->SetReadFromInputString(this->GetReadFromInputString());
      preader->SetScalarsName(this->GetScalarsName());
      preader->SetVectorsName(this->GetVectorsName());
      preader->SetNormalsName(this->GetNormalsName());
      preader->SetTensorsName(this->GetTensorsName());
      preader->SetTCoordsName(this->GetTCoordsName());
      preader->SetLookupTableName(this->GetLookupTableName());
      preader->SetFieldDataName(this->GetFieldDataName());
      preader->Update();

      // Can we use the old output?
      output = this->Outputs ? this->Outputs[0] : NULL;
      if (output && strcmp(output->GetClassName(), "vtkUnstructuredGrid") == 0)
        {
        output->ShallowCopy(preader->GetOutput());
        }
      else
        {
        this->SetNthOutput(0, preader->GetOutput());
        }

      preader->Delete();
      }
    
    else
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
    }

  else if ( !strncmp(this->LowerCase(line),"field",(unsigned long)5) )
    {
    vtkErrorMacro(<<"This object can only read datasets, not fields");
    }
  
  else
    {
    vtkErrorMacro(<<"Expecting DATASET keyword, got " << line << " instead");
    }

  return;
}


vtkPolyData *vtkDataSetReader::GetPolyDataOutput()
{
  return vtkPolyData::SafeDownCast(this->GetOutput());
}

vtkStructuredPoints *vtkDataSetReader::GetStructuredPointsOutput() 
{
  return vtkStructuredPoints::SafeDownCast(this->GetOutput());
}

vtkStructuredGrid *vtkDataSetReader::GetStructuredGridOutput() 
{
  return vtkStructuredGrid::SafeDownCast(this->GetOutput());
}

vtkUnstructuredGrid *vtkDataSetReader::GetUnstructuredGridOutput() 
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

vtkRectilinearGrid *vtkDataSetReader::GetRectilinearGridOutput() 
{
  return vtkRectilinearGrid::SafeDownCast(this->GetOutput());
}


//----------------------------------------------------------------------------
void vtkDataSetReader::Update()
{
  if (this->GetOutput())
    {
    this->GetOutput()->Update();
    }
}

void vtkDataSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataReader::PrintSelf(os,indent);
}
