/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetReader.cxx
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
#include "vtkDataSetReader.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridReader.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

vtkCxxRevisionMacro(vtkDataSetReader, "1.60");
vtkStandardNewMacro(vtkDataSetReader);

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
       (this->GetInputArray() == NULL && this->GetInputString() == NULL)))
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
  vtkDataObject *output;
  
  vtkDebugMacro(<<"Reading vtk dataset...");

  switch (this->ReadOutputType())
    {
    case VTK_POLY_DATA:
      {
      vtkPolyDataReader *preader = vtkPolyDataReader::New();
      preader->SetFileName(this->GetFileName());
      preader->SetInputArray(this->GetInputArray());
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
      return;
      }
    case VTK_STRUCTURED_POINTS:
      {
      vtkStructuredPointsReader *preader = vtkStructuredPointsReader::New();
      preader->SetFileName(this->GetFileName());
      preader->SetInputArray(this->GetInputArray());
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
      return;
      }
    case VTK_STRUCTURED_GRID:
      {
      vtkStructuredGridReader *preader = vtkStructuredGridReader::New();
      preader->SetFileName(this->GetFileName());
      preader->SetInputArray(this->GetInputArray());
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
      return;
      }
    case VTK_RECTILINEAR_GRID:
      {
      vtkRectilinearGridReader *preader = vtkRectilinearGridReader::New();
      preader->SetFileName(this->GetFileName());
      preader->SetInputArray(this->GetInputArray());
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
      return;
      }
    case VTK_UNSTRUCTURED_GRID:
      {
      vtkUnstructuredGridReader *preader = vtkUnstructuredGridReader::New();
      preader->SetFileName(this->GetFileName());
      preader->SetInputArray(this->GetInputArray());
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
      return;
      }
    default:
        vtkErrorMacro("Could not read file " << this->FileName);
    }
}


int vtkDataSetReader::ReadOutputType()
{
  char line[256];
  
  vtkDebugMacro(<<"Reading vtk dataset...");

  if (!this->OpenVTKFile() || !this->ReadHeader())
    {
    return -1;
    }

  // Determine dataset type
  //
  if (!this->ReadString(line))
    {
    vtkErrorMacro(<< "Premature EOF reading dataset keyword");
    return -1;
    }

  if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
    {
    // See if type is recognized.
    //
    if (!this->ReadString(line))
      {
      vtkErrorMacro(<< "Premature EOF reading type");
      this->CloseVTKFile ();
      return -1;
      }

    this->CloseVTKFile();
    if ( ! strncmp(this->LowerCase(line),"polydata",8) )
      {
      return VTK_POLY_DATA;
      }
    else if ( ! strncmp(line,"structured_points",17) )
      {
      return VTK_STRUCTURED_POINTS;
      }
    else if ( ! strncmp(line,"structured_grid",15) )
      {
      return VTK_STRUCTURED_GRID;
      }
    else if ( ! strncmp(line,"rectilinear_grid",16) )
      {
      return VTK_RECTILINEAR_GRID;
      }
    else if ( ! strncmp(line,"unstructured_grid",17) )
      {
      return VTK_UNSTRUCTURED_GRID;
      }
    else
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      return -1;
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

  return -1;
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
  this->Superclass::PrintSelf(os,indent);
}

vtkDataSet *vtkDataSetReader::GetOutput(int idx)
{
  return static_cast<vtkDataSet *>(this->vtkSource::GetOutput(idx)); 
}
