/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPRectilinearGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPRectilinearGridWriter.h"

#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkXMLRectilinearGridWriter.h"

vtkStandardNewMacro(vtkXMLPRectilinearGridWriter);

//----------------------------------------------------------------------------
vtkXMLPRectilinearGridWriter::vtkXMLPRectilinearGridWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPRectilinearGridWriter::~vtkXMLPRectilinearGridWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLPRectilinearGridWriter::GetInput()
{
  return static_cast<vtkRectilinearGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* vtkXMLPRectilinearGridWriter::GetDataSetName()
{
  return "PRectilinearGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLPRectilinearGridWriter::GetDefaultFileExtension()
{
  return "pvtr";
}

//----------------------------------------------------------------------------
vtkXMLStructuredDataWriter*
vtkXMLPRectilinearGridWriter::CreateStructuredPieceWriter()
{  
  // Create the writer for the piece.
  vtkXMLRectilinearGridWriter* pWriter = vtkXMLRectilinearGridWriter::New();
  pWriter->SetInput(this->GetInput());
  return pWriter;
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridWriter::WritePData(vtkIndent indent)
{
  this->Superclass::WritePData(indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  vtkRectilinearGrid* input = this->GetInput();
  this->WritePCoordinates(input->GetXCoordinates(), input->GetYCoordinates(),
                          input->GetZCoordinates(), indent);
}

int vtkXMLPRectilinearGridWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}
