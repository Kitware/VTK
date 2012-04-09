/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetReader.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridReader.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

vtkStandardNewMacro(vtkDataSetReader);

vtkDataSetReader::vtkDataSetReader()
{
}

vtkDataSetReader::~vtkDataSetReader()
{
}

int vtkDataSetReader::RequestDataObject(
  vtkInformation *,
  vtkInformationVector** vtkNotUsed(inputVector) ,
  vtkInformationVector* outputVector)
{
  if (this->GetFileName() == NULL &&
      (this->GetReadFromInputString() == 0 ||
       (this->GetInputArray() == NULL && this->GetInputString() == NULL)))
    {
    vtkWarningMacro(<< "FileName must be set");
    return 0;
    }

  int outputType = this->ReadOutputType();

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  if (output && (output->GetDataObjectType() == outputType))
    {
    return 1;
    }

  if (!output || output->GetDataObjectType() != outputType)
    {
    switch (outputType)
      {
      case VTK_POLY_DATA:
        output = vtkPolyData::New();
        break;
      case VTK_STRUCTURED_POINTS:
        output = vtkStructuredPoints::New();
        break;
      case VTK_STRUCTURED_GRID:
        output = vtkStructuredGrid::New();
        break;
      case VTK_RECTILINEAR_GRID:
        output = vtkRectilinearGrid::New();
        break;
      case VTK_UNSTRUCTURED_GRID:
        output = vtkUnstructuredGrid::New();
        break;
      default:
        return 0;
      }

    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    }

  return 1;
}

int vtkDataSetReader::RequestInformation(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  if (this->GetFileName() == NULL &&
      (this->GetReadFromInputString() == 0 ||
       (this->GetInputArray() == NULL && this->GetInputString() == NULL)))
    {
    vtkWarningMacro(<< "FileName must be set");
    return 0;
    }

  vtkDataReader *reader = 0;
  int retVal;
  switch (this->ReadOutputType())
    {
    case VTK_POLY_DATA:
      reader = vtkPolyDataReader::New();
      break;
    case VTK_STRUCTURED_POINTS:
      reader = vtkStructuredPointsReader::New();
      break;
    case VTK_STRUCTURED_GRID:
      reader = vtkStructuredGridReader::New();
      break;
    case VTK_RECTILINEAR_GRID:
      reader = vtkRectilinearGridReader::New();
      break;
    case VTK_UNSTRUCTURED_GRID:
      reader = vtkUnstructuredGridReader::New();
      break;
    default:
      reader = NULL;
    }

  if (reader)
    {
    reader->SetFileName(this->GetFileName());
    reader->SetReadFromInputString(this->GetReadFromInputString());
    reader->SetInputArray(this->GetInputArray());
    reader->SetInputString(this->GetInputString());
    retVal = reader->ReadMetaData(outInfo);
    reader->Delete();
    return retVal;
    }
  return 1;
}

int vtkDataSetReader::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());

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
      preader->SetReadAllScalars(this->GetReadAllScalars());
      preader->SetReadAllVectors(this->GetReadAllVectors());
      preader->SetReadAllNormals(this->GetReadAllNormals());
      preader->SetReadAllTensors(this->GetReadAllTensors());
      preader->SetReadAllColorScalars(this->GetReadAllColorScalars());
      preader->SetReadAllTCoords(this->GetReadAllTCoords());
      preader->SetReadAllFields(this->GetReadAllFields());
      preader->Update();
      // Can we use the old output?
      if(!(output && strcmp(output->GetClassName(), "vtkPolyData") == 0))
        {
        // Hack to make sure that the object is not modified
        // with SetNthOutput. Otherwise, extra executions occur.
        vtkTimeStamp ts = this->MTime;
        output = vtkPolyData::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->MTime = ts;
        }
      output->ShallowCopy(preader->GetOutput());
      preader->Delete();
      return 1;
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
      preader->SetReadAllScalars(this->GetReadAllScalars());
      preader->SetReadAllVectors(this->GetReadAllVectors());
      preader->SetReadAllNormals(this->GetReadAllNormals());
      preader->SetReadAllTensors(this->GetReadAllTensors());
      preader->SetReadAllColorScalars(this->GetReadAllColorScalars());
      preader->SetReadAllTCoords(this->GetReadAllTCoords());
      preader->SetReadAllFields(this->GetReadAllFields());
      preader->Update();
      output->ShallowCopy(preader->GetOutput());
      preader->Delete();
      return 1;
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
      preader->SetReadAllScalars(this->GetReadAllScalars());
      preader->SetReadAllVectors(this->GetReadAllVectors());
      preader->SetReadAllNormals(this->GetReadAllNormals());
      preader->SetReadAllTensors(this->GetReadAllTensors());
      preader->SetReadAllColorScalars(this->GetReadAllColorScalars());
      preader->SetReadAllTCoords(this->GetReadAllTCoords());
      preader->SetReadAllFields(this->GetReadAllFields());
      preader->Update();
      // Can we use the old output?
      if(!(output && strcmp(output->GetClassName(), "vtkStructuredGrid") == 0))
        {
        // Hack to make sure that the object is not modified
        // with SetNthOutput. Otherwise, extra executions occur.
        vtkTimeStamp ts = this->MTime;
        output = vtkStructuredGrid::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->MTime = ts;
        }
      output->ShallowCopy(preader->GetOutput());
      preader->Delete();
      return 1;
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
      preader->SetReadAllScalars(this->GetReadAllScalars());
      preader->SetReadAllVectors(this->GetReadAllVectors());
      preader->SetReadAllNormals(this->GetReadAllNormals());
      preader->SetReadAllTensors(this->GetReadAllTensors());
      preader->SetReadAllColorScalars(this->GetReadAllColorScalars());
      preader->SetReadAllTCoords(this->GetReadAllTCoords());
      preader->SetReadAllFields(this->GetReadAllFields());
      preader->Update();
      // Can we use the old output?
      if(!(output && strcmp(output->GetClassName(), "vtkRectilinearGrid") == 0))
        {
        // Hack to make sure that the object is not modified
        // with SetNthOutput. Otherwise, extra executions occur.
        vtkTimeStamp ts = this->MTime;
        output = vtkRectilinearGrid::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->MTime = ts;
        }
      output->ShallowCopy(preader->GetOutput());
      preader->Delete();
      return 1;
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
      preader->SetReadAllScalars(this->GetReadAllScalars());
      preader->SetReadAllVectors(this->GetReadAllVectors());
      preader->SetReadAllNormals(this->GetReadAllNormals());
      preader->SetReadAllTensors(this->GetReadAllTensors());
      preader->SetReadAllColorScalars(this->GetReadAllColorScalars());
      preader->SetReadAllTCoords(this->GetReadAllTCoords());
      preader->SetReadAllFields(this->GetReadAllFields());
      preader->Update();
      // Can we use the old output?
      if(!(output && strcmp(output->GetClassName(), "vtkUnstructuredGrid") == 0))
        {
        // Hack to make sure that the object is not modified
        // with SetNthOutput. Otherwise, extra executions occur.
        vtkTimeStamp ts = this->MTime;
        output = vtkUnstructuredGrid::New();
        this->GetExecutive()->SetOutputData(0, output);
        output->Delete();
        this->MTime = ts;
        }
      output->ShallowCopy(preader->GetOutput());
      preader->Delete();
      return 1;
      }
    default:
        vtkErrorMacro("Could not read file " << this->FileName);
    }
  return 0;
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
    vtkDebugMacro(<< "Premature EOF reading dataset keyword");
    return -1;
    }

  if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
    {
    // See if type is recognized.
    //
    if (!this->ReadString(line))
      {
      vtkDebugMacro(<< "Premature EOF reading type");
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
      vtkDebugMacro(<< "Cannot read dataset type: " << line);
      return -1;
      }
    }
  else if ( !strncmp(this->LowerCase(line),"field",(unsigned long)5) )
    {
    vtkDebugMacro(<<"This object can only read datasets, not fields");
    }
  else
    {
    vtkDebugMacro(<<"Expecting DATASET keyword, got " << line << " instead");
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

void vtkDataSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

vtkDataSet *vtkDataSetReader::GetOutput(int idx)
{
  return vtkDataSet::SafeDownCast(this->GetOutputDataObject(idx));
}

vtkDataSet *vtkDataSetReader::GetOutput()
{
  return vtkDataSet::SafeDownCast(this->GetOutputDataObject(0));
}

int vtkDataSetReader::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;
}

int vtkDataSetReader::ProcessRequest(vtkInformation* request,
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
