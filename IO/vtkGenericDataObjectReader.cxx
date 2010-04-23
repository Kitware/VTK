/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataObjectReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericDataObjectReader.h"
#include "vtkDirectedGraph.h"
#include "vtkGraph.h"
#include "vtkGraphReader.h"
#include "vtkImageData.h"
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
#include "vtkTable.h"
#include "vtkTableReader.h"
#include "vtkTree.h"
#include "vtkTreeReader.h"
#include "vtkUndirectedGraph.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

vtkStandardNewMacro(vtkGenericDataObjectReader);

template<typename ReaderT, typename DataT>
void ReadData(const char* DataClass, vtkDataReader* Owner, vtkTimeStamp& MTime, vtkDataObject* Output)
{
  ReaderT* const reader = ReaderT::New();
  
  reader->SetFileName(Owner->GetFileName());
  reader->SetInputArray(Owner->GetInputArray());
  reader->SetInputString(Owner->GetInputString(),
                          Owner->GetInputStringLength());
  reader->SetReadFromInputString(Owner->GetReadFromInputString());
  reader->SetScalarsName(Owner->GetScalarsName());
  reader->SetVectorsName(Owner->GetVectorsName());
  reader->SetNormalsName(Owner->GetNormalsName());
  reader->SetTensorsName(Owner->GetTensorsName());
  reader->SetTCoordsName(Owner->GetTCoordsName());
  reader->SetLookupTableName(Owner->GetLookupTableName());
  reader->SetFieldDataName(Owner->GetFieldDataName());
  reader->SetReadAllScalars(Owner->GetReadAllScalars());
  reader->SetReadAllVectors(Owner->GetReadAllVectors());
  reader->SetReadAllNormals(Owner->GetReadAllNormals());
  reader->SetReadAllTensors(Owner->GetReadAllTensors());
  reader->SetReadAllColorScalars(Owner->GetReadAllColorScalars());
  reader->SetReadAllTCoords(Owner->GetReadAllTCoords());
  reader->SetReadAllFields(Owner->GetReadAllFields());
  reader->Update();
  
  // Can we use the old output?
  if(!(Output && strcmp(Output->GetClassName(), DataClass) == 0))
    {
    // Hack to make sure that the object is not modified
    // with SetNthOutput. Otherwise, extra executions occur.
    const vtkTimeStamp mtime = MTime;
    Output = DataT::New();
    Owner->GetExecutive()->SetOutputData(0, Output);
    Output->Delete();
    MTime = mtime;
    }
  Output->ShallowCopy(reader->GetOutput());
  Output->GetPipelineInformation()->CopyEntry(
    reader->GetOutput()->GetPipelineInformation(),
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  reader->Delete();
}

vtkGenericDataObjectReader::vtkGenericDataObjectReader()
{
}

vtkGenericDataObjectReader::~vtkGenericDataObjectReader()
{
}

int vtkGenericDataObjectReader::RequestDataObject(
  vtkInformation* /*information*/,
  vtkInformationVector** /*inputVector*/, 
  vtkInformationVector* outputVector)
{
  if(this->GetFileName() == NULL &&
      (this->GetReadFromInputString() == 0 ||
       (this->GetInputArray() == NULL && this->GetInputString() == NULL)))
    {
    vtkWarningMacro(<< "FileName must be set");
    return 0;
    }

  int outputType = this->ReadOutputType();

  vtkInformation* const info = outputVector->GetInformationObject(0);
  vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

  if(output && (output->GetDataObjectType() == outputType))
    {
    return 1;
    }

  if(!output || output->GetDataObjectType() != outputType) 
    {
    switch (outputType)
      {
      case VTK_DIRECTED_GRAPH:
        output = vtkDirectedGraph::New();
        break;
      case VTK_UNDIRECTED_GRAPH:
        output = vtkUndirectedGraph::New();
        break;
      case VTK_IMAGE_DATA:
        output = vtkImageData::New();
        break;
      case VTK_POLY_DATA:
        output = vtkPolyData::New();
        break;
      case VTK_RECTILINEAR_GRID:
        output = vtkRectilinearGrid::New();
        break;
      case VTK_STRUCTURED_GRID:
        output = vtkStructuredGrid::New();
        break;
      case VTK_STRUCTURED_POINTS:
        output = vtkStructuredPoints::New();
        break;
      case VTK_TABLE:
        output = vtkTable::New();
        break;
      case VTK_TREE:
        output = vtkTree::New();
        break;
      case VTK_UNSTRUCTURED_GRID:
        output = vtkUnstructuredGrid::New();
        break;
      default:
        return 0;
      }
    
    output->SetPipelineInformation(info);
    output->Delete();
    }

  return 1;
}

int vtkGenericDataObjectReader::RequestInformation(
  vtkInformation* /*information*/,
  vtkInformationVector** /*inputVector*/,
  vtkInformationVector* outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  if(this->GetFileName() == NULL &&
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
    case VTK_UNDIRECTED_GRAPH:
    case VTK_DIRECTED_GRAPH:
      reader = vtkGraphReader::New();
      break;
    case VTK_IMAGE_DATA:
      reader = vtkStructuredPointsReader::New();
      break;
    case VTK_POLY_DATA:
      reader = vtkPolyDataReader::New();
      break;
    case VTK_RECTILINEAR_GRID:
      reader = vtkRectilinearGridReader::New();
      break;
    case VTK_STRUCTURED_GRID:
      reader = vtkStructuredGridReader::New();
      break;
    case VTK_STRUCTURED_POINTS:
      reader = vtkStructuredPointsReader::New();
      break;
    case VTK_TABLE:
      reader = vtkTableReader::New();
      break;
    case VTK_TREE:
      reader = vtkTreeReader::New();
      break;
    case VTK_UNSTRUCTURED_GRID:
      reader = vtkUnstructuredGridReader::New();
      break;
    default:
      reader = NULL;
    }

  if(reader)
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

int vtkGenericDataObjectReader::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  
  vtkDebugMacro(<<"Reading vtk dataset...");

  switch (this->ReadOutputType())
    {
    case VTK_DIRECTED_GRAPH:
      {
      ReadData<vtkGraphReader, vtkDirectedGraph>("vtkDirectedGraph", this, this->MTime, output);
      return 1;
      }
    case VTK_UNDIRECTED_GRAPH:
      {
      ReadData<vtkGraphReader, vtkUndirectedGraph>("vtkUndirectedGraph", this, this->MTime, output);
      return 1;
      }
    case VTK_IMAGE_DATA:
      {
      ReadData<vtkStructuredPointsReader, vtkImageData>("vtkImageData", this, this->MTime, output);
      return 1;
      }
    case VTK_POLY_DATA:
      {
      ReadData<vtkPolyDataReader, vtkPolyData>("vtkPolyData", this, this->MTime, output);
      return 1;
      }
    case VTK_RECTILINEAR_GRID:
      {
      ReadData<vtkRectilinearGridReader, vtkRectilinearGrid>("vtkRectilinearGrid", this, this->MTime, output);
      return 1;
      }
    case VTK_STRUCTURED_GRID:
      {
      ReadData<vtkStructuredGridReader, vtkStructuredGrid>("vtkStructuredGrid", this, this->MTime, output);
      return 1;
      }
    case VTK_STRUCTURED_POINTS:
      {
      ReadData<vtkStructuredPointsReader, vtkStructuredPoints>("vtkStructuredPoints", this, this->MTime, output);
      return 1;
      }
    case VTK_TABLE:
      {
      ReadData<vtkTableReader, vtkTable>("vtkTable", this, this->MTime, output);
      return 1;
      }
    case VTK_TREE:
      {
      ReadData<vtkTreeReader, vtkTree>("vtkTree", this, this->MTime, output);
      return 1;
      }
    case VTK_UNSTRUCTURED_GRID:
      {
      ReadData<vtkUnstructuredGridReader, vtkUnstructuredGrid>("vtkUnstructuredGrid", this, this->MTime, output);
      return 1;
      }
    default:
        vtkErrorMacro("Could not read file " << this->FileName);
    }
  return 0;
}

int vtkGenericDataObjectReader::ReadOutputType()
{
  char line[256];
  
  vtkDebugMacro(<<"Reading vtk data object...");

  if(!this->OpenVTKFile() || !this->ReadHeader())
    {
    return -1;
    }

  // Determine dataset type
  //
  if(!this->ReadString(line))
    {
    vtkDebugMacro(<< "Premature EOF reading dataset keyword");
    return -1;
    }

  if(!strncmp(this->LowerCase(line),"dataset",static_cast<unsigned long>(7)))
    {
    // See iftype is recognized.
    //
    if(!this->ReadString(line))
      {
      vtkDebugMacro(<< "Premature EOF reading type");
      this->CloseVTKFile ();
      return -1;
      }

    this->CloseVTKFile();
    
    if(!strncmp(this->LowerCase(line), "directed_graph", 5))
      {
      return VTK_DIRECTED_GRAPH;
      }
    if(!strncmp(this->LowerCase(line), "undirected_graph", 5))
      {
      return VTK_UNDIRECTED_GRAPH;
      }
    if(!strncmp(this->LowerCase(line), "polydata",8))
      {
      return VTK_POLY_DATA;
      }
    if(!strncmp(this->LowerCase(line), "rectilinear_grid",16))
      {
      return VTK_RECTILINEAR_GRID;
      }
    if(!strncmp(this->LowerCase(line), "structured_grid",15))
      {
      return VTK_STRUCTURED_GRID;
      }
    if(!strncmp(this->LowerCase(line), "structured_points",17))
      {
      return VTK_STRUCTURED_POINTS;
      }
    if(!strncmp(this->LowerCase(line), "table", 5))
      {
      return VTK_TABLE;
      }
    if(!strncmp(this->LowerCase(line), "tree", 4))
      {
      return VTK_TREE;
      }
    if(!strncmp(this->LowerCase(line), "unstructured_grid",17))
      {
      return VTK_UNSTRUCTURED_GRID;
      }

    vtkDebugMacro(<< "Cannot read dataset type: " << line);
    return -1;
    }
  else if(!strncmp(this->LowerCase(line),"field",static_cast<unsigned long>(5)))
    {
    vtkDebugMacro(<<"This object can only read data objects, not fields");
    }
  else
    {
    vtkDebugMacro(<<"Expecting DATASET keyword, got " << line << " instead");
    }

  return -1;
}

vtkGraph *vtkGenericDataObjectReader::GetGraphOutput()
{
  return vtkGraph::SafeDownCast(this->GetOutput());
}

vtkPolyData *vtkGenericDataObjectReader::GetPolyDataOutput()
{
  return vtkPolyData::SafeDownCast(this->GetOutput());
}

vtkRectilinearGrid *vtkGenericDataObjectReader::GetRectilinearGridOutput() 
{
  return vtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

vtkStructuredGrid *vtkGenericDataObjectReader::GetStructuredGridOutput() 
{
  return vtkStructuredGrid::SafeDownCast(this->GetOutput());
}

vtkStructuredPoints *vtkGenericDataObjectReader::GetStructuredPointsOutput() 
{
  return vtkStructuredPoints::SafeDownCast(this->GetOutput());
}

vtkTable *vtkGenericDataObjectReader::GetTableOutput()
{
  return vtkTable::SafeDownCast(this->GetOutput());
}

vtkTree *vtkGenericDataObjectReader::GetTreeOutput()
{
  return vtkTree::SafeDownCast(this->GetOutput());
}

vtkUnstructuredGrid *vtkGenericDataObjectReader::GetUnstructuredGridOutput() 
{
  return vtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

void vtkGenericDataObjectReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

vtkDataObject *vtkGenericDataObjectReader::GetOutput()
{
  return this->GetOutputDataObject(0);
}

vtkDataObject *vtkGenericDataObjectReader::GetOutput(int idx)
{
  return this->GetOutputDataObject(idx);
}

int vtkGenericDataObjectReader::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

int vtkGenericDataObjectReader::ProcessRequest(vtkInformation* request,
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
