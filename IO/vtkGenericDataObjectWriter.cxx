/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataObjectWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericDataObjectWriter.h"

#include "vtkDataObject.h"
#include "vtkErrorCode.h"
#include "vtkGraph.h"
#include "vtkGraphWriter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataWriter.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridWriter.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridWriter.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsWriter.h"
#include "vtkTable.h"
#include "vtkTableWriter.h"
#include "vtkTree.h"
#include "vtkTreeWriter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridWriter.h"

vtkStandardNewMacro(vtkGenericDataObjectWriter);

template<typename WriterT, typename DataT>
vtkDataWriter* CreateWriter(vtkDataObject* Data)
{
  WriterT* const writer = WriterT::New();
  writer->SetInput(static_cast<DataT*>(Data));
  return writer;
}

vtkGenericDataObjectWriter::vtkGenericDataObjectWriter()
{
}

vtkGenericDataObjectWriter::~vtkGenericDataObjectWriter()
{
}

void vtkGenericDataObjectWriter::WriteData()
{
  vtkDebugMacro(<<"Writing vtk data object ...");

  vtkDataWriter* writer = 0;

  vtkDataObject* const input = this->GetInput();
  switch(input->GetDataObjectType())
    {
    case VTK_COMPOSITE_DATA_SET:
      vtkErrorMacro(<< "Cannot write composite data set");
      return;
    case VTK_DATA_OBJECT:
      vtkErrorMacro(<< "Cannot write data object");
      return;
    case VTK_DATA_SET:
      vtkErrorMacro(<< "Cannot write data set");
      return;
    case VTK_GENERIC_DATA_SET:
      vtkErrorMacro(<< "Cannot write generic data set");
      return;
    case VTK_DIRECTED_GRAPH:
    case VTK_UNDIRECTED_GRAPH:
      writer = CreateWriter<vtkGraphWriter, vtkGraph>(input);
      break;
    case VTK_HIERARCHICAL_BOX_DATA_SET:
      vtkErrorMacro(<< "Cannot write hierarchical box data set");
      return;
    case VTK_HIERARCHICAL_DATA_SET:
      vtkErrorMacro(<< "Cannot write hierarchical data set");
      return;
    case VTK_HYPER_OCTREE:
      vtkErrorMacro(<< "Cannot write hyper octree");
      return;
    case VTK_IMAGE_DATA:
      writer = CreateWriter<vtkStructuredPointsWriter, vtkImageData>(input);
      break;
    case VTK_MULTIBLOCK_DATA_SET:
      vtkErrorMacro(<< "Cannot write multiblock data set");
      return;
    case VTK_MULTIGROUP_DATA_SET:
      vtkErrorMacro(<< "Cannot write multigroup data set");
      return;
    case VTK_PIECEWISE_FUNCTION:
      vtkErrorMacro(<< "Cannot write piecewise function");
      return;
    case VTK_POINT_SET:
      vtkErrorMacro(<< "Cannot write point set");
      return;
    case VTK_POLY_DATA:
      writer = CreateWriter<vtkPolyDataWriter, vtkPolyData>(input);
      break;
    case VTK_RECTILINEAR_GRID:
      writer = CreateWriter<vtkRectilinearGridWriter, vtkRectilinearGrid>(input);
      break;
    case VTK_STRUCTURED_GRID:
      writer = CreateWriter<vtkStructuredGridWriter, vtkStructuredGrid>(input);
      break;
    case VTK_STRUCTURED_POINTS:
      writer = CreateWriter<vtkStructuredPointsWriter, vtkStructuredPoints>(input);
      break;
    case VTK_TABLE:
      writer = CreateWriter<vtkTableWriter, vtkTable>(input);
      break;
    case VTK_TREE:
      writer = CreateWriter<vtkTreeWriter, vtkTree>(input);
      break;
    case VTK_TEMPORAL_DATA_SET:
      vtkErrorMacro(<< "Cannot write temporal data set");
      return;
    case VTK_UNIFORM_GRID:
      vtkErrorMacro(<< "Cannot write uniform grid");
      return;
    case VTK_UNSTRUCTURED_GRID:
      writer = CreateWriter<vtkUnstructuredGridWriter, vtkUnstructuredGrid>(input);
      break;
    }

  if(!writer)
    {
    vtkErrorMacro(<< "null data object writer");
    return;
    }

  writer->SetFileName(this->FileName);
  writer->SetScalarsName(this->ScalarsName);
  writer->SetVectorsName(this->VectorsName);
  writer->SetNormalsName(this->NormalsName);
  writer->SetTensorsName(this->TensorsName);
  writer->SetTCoordsName(this->TCoordsName);
  writer->SetHeader(this->Header);
  writer->SetLookupTableName(this->LookupTableName);
  writer->SetFieldDataName(this->FieldDataName);
  writer->SetFileType(this->FileType);
  writer->SetDebug(this->Debug);
  writer->SetWriteToOutputString(this->WriteToOutputString);
  writer->Write();
  if (writer->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
    {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    }
  if (this->WriteToOutputString)
    {
    if (this->OutputString)
      {
      delete [] this->OutputString;
      }
    this->OutputStringLength = writer->GetOutputStringLength();
    this->OutputString = writer->RegisterAndGetOutputString();
    }
  writer->Delete();
}

int vtkGenericDataObjectWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

void vtkGenericDataObjectWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
