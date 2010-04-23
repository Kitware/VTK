/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPStructuredGridWriter.h"

#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredGrid.h"
#include "vtkXMLStructuredGridWriter.h"

vtkStandardNewMacro(vtkXMLPStructuredGridWriter);

//----------------------------------------------------------------------------
vtkXMLPStructuredGridWriter::vtkXMLPStructuredGridWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPStructuredGridWriter::~vtkXMLPStructuredGridWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLPStructuredGridWriter::GetInput()
{
  return static_cast<vtkStructuredGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* vtkXMLPStructuredGridWriter::GetDataSetName()
{
  return "PStructuredGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLPStructuredGridWriter::GetDefaultFileExtension()
{
  return "pvts";
}

//----------------------------------------------------------------------------
vtkXMLStructuredDataWriter*
vtkXMLPStructuredGridWriter::CreateStructuredPieceWriter()
{  
  // Create the writer for the piece.
  vtkXMLStructuredGridWriter* pWriter = vtkXMLStructuredGridWriter::New();
  pWriter->SetInput(this->GetInput());
  return pWriter;
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridWriter::WritePData(vtkIndent indent)
{
  this->Superclass::WritePData(indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  vtkStructuredGrid* input = this->GetInput();  
  this->WritePPoints(input->GetPoints(), indent);
}

int vtkXMLPStructuredGridWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
  return 1;
}
