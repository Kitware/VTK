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

vtkCxxRevisionMacro(vtkDataSetWriter, "1.38");
vtkStandardNewMacro(vtkDataSetWriter);

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkDataSetWriter::SetInput(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkDataSetWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  return static_cast<vtkDataSet*>(this->Inputs[0]);
}

void vtkDataSetWriter::WriteData()
{
  int type;
  vtkDataWriter *writer;
  vtkDataSet *input = this->GetInput();
  
  vtkDebugMacro(<<"Writing vtk dataset...");

  type = input->GetDataObjectType();
  if ( type == VTK_POLY_DATA )
    {
    vtkPolyDataWriter *pwriter = vtkPolyDataWriter::New();
    pwriter->SetInput(static_cast<vtkPolyData*>(input));
    writer = pwriter;
    }

  else if ( type == VTK_STRUCTURED_POINTS || type == VTK_IMAGE_DATA)
    {
    vtkStructuredPointsWriter *spwriter = vtkStructuredPointsWriter::New();
    spwriter->SetInput(static_cast<vtkImageData*>(input));
    writer = spwriter;
    }

  else if ( type == VTK_STRUCTURED_GRID )
    {
    vtkStructuredGridWriter *sgwriter = vtkStructuredGridWriter::New();
    sgwriter->SetInput(static_cast<vtkStructuredGrid*>(input));
    writer = sgwriter;
    }

  else if ( type == VTK_UNSTRUCTURED_GRID )
    {
    vtkUnstructuredGridWriter *ugwriter = vtkUnstructuredGridWriter::New();
    ugwriter->SetInput(static_cast<vtkUnstructuredGrid*>(input));
    writer = ugwriter;
    }

  else if ( type == VTK_RECTILINEAR_GRID )
    {
    vtkRectilinearGridWriter *rgwriter = vtkRectilinearGridWriter::New();
    rgwriter->SetInput(static_cast<vtkRectilinearGrid*>(input));
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
    if (this->OutputString)
      {
      delete [] this->OutputString;
      }
    this->OutputStringLength = writer->GetOutputStringLength();
    // should fill something here.
    this->OutputStringAllocatedLength = this->OutputStringLength;
    this->OutputString = writer->RegisterAndGetOutputString();
    }
  writer->Delete();
}

void vtkDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
