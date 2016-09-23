/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetWriter.h"

#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataWriter.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridWriter.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridWriter.h"
#include "vtkStructuredPointsWriter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridWriter.h"
#include "vtkAlgorithmOutput.h"

vtkStandardNewMacro(vtkDataSetWriter);

void vtkDataSetWriter::WriteData()
{
  int type;
  vtkDataWriter *writer;
  vtkAlgorithmOutput *input = this->GetInputConnection(0, 0);

  vtkDebugMacro(<<"Writing vtk dataset...");

  type = this->GetInput()->GetDataObjectType();
  if ( type == VTK_POLY_DATA )
  {
    vtkPolyDataWriter *pwriter = vtkPolyDataWriter::New();
    pwriter->SetInputConnection(input);
    writer = pwriter;
  }

  else if ( type == VTK_STRUCTURED_POINTS || type == VTK_IMAGE_DATA || type == VTK_UNIFORM_GRID)
  {
    vtkStructuredPointsWriter *spwriter = vtkStructuredPointsWriter::New();
    spwriter->SetInputConnection(input);
    writer = spwriter;
  }

  else if ( type == VTK_STRUCTURED_GRID )
  {
    vtkStructuredGridWriter *sgwriter = vtkStructuredGridWriter::New();
    sgwriter->SetInputConnection(input);
    writer = sgwriter;
  }

  else if ( type == VTK_UNSTRUCTURED_GRID )
  {
    vtkUnstructuredGridWriter *ugwriter = vtkUnstructuredGridWriter::New();
    ugwriter->SetInputConnection(input);
    writer = ugwriter;
  }

  else if ( type == VTK_RECTILINEAR_GRID )
  {
    vtkRectilinearGridWriter *rgwriter = vtkRectilinearGridWriter::New();
    rgwriter->SetInputConnection(input);
    writer = rgwriter;
  }

  else
  {
    vtkErrorMacro(<< "Cannot write dataset type: " << type);
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
    delete [] this->OutputString;
    this->OutputStringLength = writer->GetOutputStringLength();
    this->OutputString = writer->RegisterAndGetOutputString();
  }
  writer->Delete();
}

int vtkDataSetWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

vtkDataSet* vtkDataSetWriter::GetInput()
{
  return vtkDataSet::SafeDownCast(this->Superclass::GetInput());
}

vtkDataSet* vtkDataSetWriter::GetInput(int port)
{
  return vtkDataSet::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
