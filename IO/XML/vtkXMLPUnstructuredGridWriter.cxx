/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPUnstructuredGridWriter.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridWriter.h"

vtkStandardNewMacro(vtkXMLPUnstructuredGridWriter);

//----------------------------------------------------------------------------
vtkXMLPUnstructuredGridWriter::vtkXMLPUnstructuredGridWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPUnstructuredGridWriter::~vtkXMLPUnstructuredGridWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLPUnstructuredGridWriter::GetInput()
{
  return static_cast<vtkUnstructuredGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* vtkXMLPUnstructuredGridWriter::GetDataSetName()
{
  return "PUnstructuredGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLPUnstructuredGridWriter::GetDefaultFileExtension()
{
  return "pvtu";
}

//----------------------------------------------------------------------------
vtkXMLUnstructuredDataWriter*
vtkXMLPUnstructuredGridWriter::CreateUnstructuredPieceWriter()
{
  // Create the writer for the piece.
  vtkXMLUnstructuredGridWriter* pWriter = vtkXMLUnstructuredGridWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//----------------------------------------------------------------------------
int vtkXMLPUnstructuredGridWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}
